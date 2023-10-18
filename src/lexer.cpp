#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <iostream>
#include <optional>

using namespace std;

namespace parse {

bool operator==(const Token& lhs, const Token& rhs) {
    using namespace token_type;

    if (lhs.index() != rhs.index()) {
        return false;
    }
    if (lhs.Is<Char>()) {
        return lhs.As<Char>().value == rhs.As<Char>().value;
    }
    if (lhs.Is<Number>()) {
        return lhs.As<Number>().value == rhs.As<Number>().value;
    }
    if (lhs.Is<String>()) {
        return lhs.As<String>().value == rhs.As<String>().value;
    }
    if (lhs.Is<Id>()) {
        return lhs.As<Id>().value == rhs.As<Id>().value;
    }
    return true;
}

bool operator!=(const Token& lhs, const Token& rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const Token& rhs) {
    using namespace token_type;

#define VALUED_OUTPUT(type) \
    if (auto p = rhs.TryAs<type>()) return os << #type << '{' << p->value << '}';

    VALUED_OUTPUT(Number);
    VALUED_OUTPUT(Id);
    VALUED_OUTPUT(String);
    VALUED_OUTPUT(Char);

#undef VALUED_OUTPUT

#define UNVALUED_OUTPUT(type) \
    if (rhs.Is<type>()) return os << #type;

    UNVALUED_OUTPUT(Class);
    UNVALUED_OUTPUT(Return);
    UNVALUED_OUTPUT(If);
    UNVALUED_OUTPUT(Else);
    UNVALUED_OUTPUT(Def);
    UNVALUED_OUTPUT(Newline);
    UNVALUED_OUTPUT(Print);
    UNVALUED_OUTPUT(Indent);
    UNVALUED_OUTPUT(Dedent);
    UNVALUED_OUTPUT(And);
    UNVALUED_OUTPUT(Or);
    UNVALUED_OUTPUT(Not);
    UNVALUED_OUTPUT(Eq);
    UNVALUED_OUTPUT(NotEq);
    UNVALUED_OUTPUT(LessOrEq);
    UNVALUED_OUTPUT(GreaterOrEq);
    UNVALUED_OUTPUT(None);
    UNVALUED_OUTPUT(True);
    UNVALUED_OUTPUT(False);
    UNVALUED_OUTPUT(Eof);

#undef UNVALUED_OUTPUT

    return os << "Unknown token :("sv;
}

// Контексты start ----------------------------------------------------------------------------------------

class Lexer::TokenizerEof final: public Lexer::Tokenizer_interface
{
public:
    Token GetToken() override 
    {
        return Token{token_type::Eof{}};
    }

    size_t GetTokenWordEnd() override
    {
        return std::string::npos;
    }

private:
    void Parse(std::string_view /*line*/) override {}
};

class Lexer::TokenizerIntend final: public Lexer::Tokenizer_interface
{
public:

    TokenizerIntend(std::list<Token>& tokens, std::string_view line, bool is_newline = false): tokens_(tokens), is_newline_(is_newline)
    {
        Parse(line);
    }

    void Parse(std::string_view line) override
    {
        pos_intend_end_ = std::min(line.find_first_not_of(' ', 0), line.size());
        
        if (is_newline_)
        {
            auto intend_count = (pos_intend_end_ + 1) / 2;

            if (intend_stack_.empty() || intend_stack_.top() < intend_count)
            {
                intend_stack_.push(intend_count);
                for (int i = 0; i < intend_count; i++)
                {
                    tokens_.emplace_back(token_type::Indent{});
                }
            }
            else
            {
                intend_count = intend_stack_.top() - intend_count;
                intend_stack_.pop();

                for (int i = 0; i < intend_count; i++)
                {
                    tokens_.emplace_back(token_type::Dedent{});
                }
            }
        }
        
    }

    size_t GetTokenWordEnd() override
    {
        return pos_intend_end_;
    }

private:

    Token GetToken() override { return Token{token_type::Indent{}}; }

    std::list<Token>& tokens_;
    size_t pos_intend_end_ = 0;
    static std::stack<uint32_t, std::list<uint32_t>> intend_stack_;
    bool is_newline_;
};
std::stack<uint32_t, std::list<uint32_t>>  Lexer::TokenizerIntend::intend_stack_{};

class Lexer::TokenizerNewline final: public Lexer::Tokenizer_interface
{
public:
    Token GetToken() override
    {
        return Token{token_type::Newline{}};
    }

    size_t GetTokenWordEnd() override
    {
        return 1;
    }

private:
    void Parse(std::string_view /*line*/) override {}
};

class Lexer::TokenizerSomeWord final: public Lexer::Tokenizer_interface
{
public:

    TokenizerSomeWord(std::string_view line)
    {
        Parse(line);
    }

    void Parse(std::string_view line) override
    {
        pos_end_word_ = std::min(line.find_first_of(" \n", 0), line.size());
        std::string_view subline = line.substr(0, pos_end_word_);

        if (key_signs_.find(subline[0]) != key_signs_.end())
        {
            out_ = token_type::Char{subline[0]};
            return;
        }

        auto token = keywords_.find(subline);
        if (token != keywords_.end())
        {
            out_ = std::move(token->second);
        }
        else
        {
            out_ = token_type::Id{std::string{subline}};
        }
    }

    Token GetToken() override
    {
        return std::move(out_);
    }

    size_t GetTokenWordEnd() override
    {
        return pos_end_word_;
    }

private:
    size_t pos_end_word_ = 0;
    Token out_;
    static const std::unordered_map<std::string_view, Token> keywords_;
    static const std::unordered_set<char> key_signs_;
};
const std::unordered_map<std::string_view, Token> Lexer::TokenizerSomeWord::keywords_
    {
        {"class", token_type::Class{}},
        {"return", token_type::Return{}},
        {"if", token_type::If{}},
        {"else", token_type::Else{}},
        {"def", token_type::Def{}},
        // {"/n"s, token_type::Newline{}},
        {"print", token_type::Print{}},
        // {"indent"s, token_type::Indent{}},
        // {"dedent"s, token_type::Dedent{}},
        // {"EOF"s, token_type::Eof{}},
        {"and", token_type::And{}},
        {"or", token_type::Or{}},
        {"not", token_type::Not{}},
        {"==", token_type::Eq{}},
        {"!=", token_type::NotEq{}},
        {"<=", token_type::LessOrEq{}},
        {">=", token_type::GreaterOrEq{}},
        {"None", token_type::None{}},
        {"True", token_type::True{}},
        {"False", token_type::False{}},
    };
const std::unordered_set<char> Lexer::TokenizerSomeWord::key_signs_{'=', '.', ',', '(', '+', '<', '=', ')'};

class Lexer::TokenizerNumber final: public Lexer::Tokenizer_interface
{
public:

    TokenizerNumber(std::string_view line)
    {
        Parse(line);
    }

    void Parse(std::string_view line) override
    {
        pos_end_ = std::min(line.find_first_of(" \n", 0), line.size());

        number_ = token_type::Number{StringConvertToDigit(line.substr(0, pos_end_))};
    }

    Token GetToken() override
    {
        return std::move(number_);
    }

    size_t GetTokenWordEnd() override
    {
        return pos_end_;
    }

private:

    int StringConvertToDigit(std::string_view str)
    {
        int out = 0;

        if (str.empty())
        {
            return out;
        }

        const std::from_chars_result res = std::from_chars(str.data(), str.data() + str.size(), out);

        
        if (res.ec != std::errc() || res.ptr != str.data() + str.size())
        {
            throw std::runtime_error("Can't convert string to digit");
        }

        return out;
    }

    size_t pos_end_ = 0;
    Token number_;
};

// Контексты end----------------------------------------------------------------------------------------

Lexer::Lexer(std::istream& input)
{
    const size_t BUFF_SIZE = 1024;
    char line[BUFF_SIZE];
    
    bool is_newline = true;
    input.read(line, BUFF_SIZE);
    do {
        BufferPareser(line, is_newline);
        is_newline = false;
    
    } while (input.read(line, BUFF_SIZE));

    // if (input.eof())
    // {
    //     tokens.emplace_back(token_type::Eof{});
    // }

    tokens_.emplace_back(TokenizerEof{}.GetToken());
    curr_token_ = tokens_.begin();
}

void Lexer::BufferPareser(std::string_view line, bool is_newline = false)
{
    if (line.empty() || line[0] == EOF)
    {
        return;
    }

    if (line[0] == ' ')
    {
        auto tokenize = TokenizerIntend(tokens_, line, is_newline);

        BufferPareser({line.data() + tokenize.GetTokenWordEnd()});
    }
    else if (line[0] == '\n')
    {
        tokens_.emplace_back(TokenizerNewline{}.GetToken()); 

        BufferPareser({line.data() + 1}, true);
    }
    else if (line[0] == '#')
    {
    }
    else if (std::isdigit(line[0]))
    {
        auto tokenize = TokenizerNumber(line);
        tokens_.emplace_back(tokenize.GetToken());

        BufferPareser({line.data() + tokenize.GetTokenWordEnd()});
    }
    else
    {
        auto tokenize = TokenizerSomeWord(line);
        tokens_.emplace_back(tokenize.GetToken());
        
        BufferPareser({line.data() + tokenize.GetTokenWordEnd()});
    }
}

const Token& Lexer::CurrentToken() const 
{
    return (*curr_token_);
}

Token Lexer::NextToken() 
{
    if (*curr_token_ != Token(token_type::Eof{}))
    {
        ++curr_token_;
    }

    return *(curr_token_);
}

void Lexer::PrintTokens()
{
    for (const auto& token : tokens_)
    {
        std::cout << token << std::endl;
    }
}

}  // namespace parse