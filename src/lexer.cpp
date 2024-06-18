#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

namespace parse
{

    bool operator==(const Token &lhs, const Token &rhs)
    {
        using namespace token_type;

        if (lhs.index() != rhs.index())
        {
            return false;
        }
        if (lhs.Is<Char>())
        {
            return lhs.As<Char>().value == rhs.As<Char>().value;
        }
        if (lhs.Is<Number>())
        {
            return lhs.As<Number>().value == rhs.As<Number>().value;
        }
        if (lhs.Is<String>())
        {
            return lhs.As<String>().value == rhs.As<String>().value;
        }
        if (lhs.Is<Id>())
        {
            return lhs.As<Id>().value == rhs.As<Id>().value;
        }
        return true;
    }

    bool operator!=(const Token &lhs, const Token &rhs)
    {
        return !(lhs == rhs);
    }

    std::ostream &operator<<(std::ostream &os, const Token &rhs)
    {
        using namespace token_type;

#define VALUED_OUTPUT(type)         \
    if (auto p = rhs.TryAs<type>()) \
        return os << #type << '{' << p->value << '}';

        VALUED_OUTPUT(Number);
        VALUED_OUTPUT(Id);
        VALUED_OUTPUT(String);
        VALUED_OUTPUT(Char);

#undef VALUED_OUTPUT

#define UNVALUED_OUTPUT(type) \
    if (rhs.Is<type>())       \
        return os << #type;

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

    std::string Unescape(const std::string &str)
    {
        std::string result;
        result.reserve(str.size());
        for (size_t i = 0; i < str.size(); ++i)
        {
            if (str[i] == '\\' && i + 1 < str.size())
            {
                switch (str[++i])
                {
                case '\"':
                    result += '\"';
                    break;
                case '\'':
                    result += '\'';
                    break;
                case 't':
                    result += '\t';
                    break;
                case 'n':
                    result += '\n';
                    break;
                case '\\':
                    result += '\\';
                    break;
                default:
                    result += '\\';
                    result += str[i];
                    break;
                }
            }
            else
            {
                result += str[i];
            }
        }
        return result;
    }

    //=============================Tokenazer=====================================
    class Tokenazer_Base
    {
    public:
        static uint32_t GetIndentLevel()
        {
            return Tokenazer_Base::intend_level_;
        }

        static void EraseIndent()
        {
            Tokenazer_Base::intend_level_ = 0;
        }

    protected:
        Tokenazer_Base(std::string_view buff) : buff_(buff) {}
        virtual void PushToOutput(Token &&) = 0;
        virtual ~Tokenazer_Base() = default;

        void HandleCode()
        {
            if (buff_[0] == '#')
            {
                HandleComment();
                return;
            }

            auto tokens = HandleIntend();
            if (!tokens.empty())
            {
                for (auto &token : tokens)
                {
                    PushToOutput(std::move(token));
                }
            }

            while (!buff_.empty())
            {
                if (buff_[0] == '#')
                {
                    HandleComment();
                    continue;
                }

                if (std::isspace(buff_[0]))
                {
                    auto space_count = buff_.find_first_not_of(" \n\t");
                    buff_.remove_prefix(space_count == std::string::npos ? buff_.size() : space_count);
                }
                else if (std::isalpha(buff_[0]) || buff_[0] == '_')
                {
                    PushToOutput(HandleWord());
                }
                else if (std::isdigit(buff_[0]))
                {
                    PushToOutput(HandleNumber());
                }
                else if (buff_[0] == '"' || buff_[0] == '\'')
                {
                    PushToOutput(HandleString());
                }
                else if (std::ispunct(buff_[0]))
                {
                    PushToOutput(HandleOperator());
                }
            }
            PushToOutput(Token{token_type::Newline{}});
        }

    private:
        std::vector<Token> HandleIntend()
        {
            std::vector<Token> tokens;
            uint32_t curr_inten_lvl = 0;

            auto space_count = buff_.find_first_not_of(" \n\t");
            if (space_count == std::string::npos)
            {
                return tokens;
            }

            curr_inten_lvl = space_count / 2;
            buff_.remove_prefix(space_count);

            if (curr_inten_lvl > intend_level_)
            {
                auto diff = curr_inten_lvl - intend_level_;
                tokens.resize(diff, Token{token_type::Indent{}});
            }
            else if (curr_inten_lvl < intend_level_)
            {
                auto diff = intend_level_ - curr_inten_lvl;
                tokens.resize(diff, Token{token_type::Dedent{}});
            }

            intend_level_ = curr_inten_lvl;
            return tokens;
        }

        Token HandleWord()
        {
            Token token;
            size_t end_pos = 0;

            while (buff_.size() > end_pos && (std::isalnum(buff_[end_pos]) || buff_[end_pos] == '_'))
            {
                ++end_pos;
            }

            std::string word(buff_.substr(0, end_pos));
            buff_.remove_prefix(end_pos);

            auto it = keywords_.find(word);
            if (it != keywords_.end())
            {
                token = it->second;
            }
            else
            {
                token = token_type::Id{std::move(word)};
            }

            return token;
        }

        Token HandleNumber()
        {
            Token token;

            size_t end_pos = 0;
            while (buff_.size() > end_pos && std::isdigit(buff_[end_pos]))
            {
                ++end_pos;
            }

            token = token_type::Number{std::stoi(std::string(buff_.substr(0, end_pos)))};
            buff_.remove_prefix(end_pos);

            return token;
        }

        Token HandleString()
        {
            Token token;

            char sep = buff_[0];
            buff_.remove_prefix(1);

            size_t end_pos = 0;
            bool has_escape = false;
            while (buff_.size() > 0 && buff_[end_pos] != sep)
            {
                if (buff_[end_pos] == '\\')
                {
                    has_escape = true;
                    ++end_pos;
                }
                ++end_pos;
            }

            token = token_type::String{
                has_escape ? Unescape(std::string(buff_.substr(0, end_pos))) : std::string(buff_.substr(0, end_pos))};

            buff_.remove_prefix(++end_pos);

            return token;
        }

        Token HandleOperator()
        {
            Token token;

            if (std::ispunct(buff_[1]))
            {
                std::string op{buff_.substr(0, 2)};
                auto it = operators_.find(op);
                if (it != operators_.end())
                {
                    token = it->second;
                    buff_.remove_prefix(2);
                    return token;
                }
            }

            token = operators_.at(std::string{buff_[0]});
            buff_.remove_prefix(1);

            return token;
        }

        void HandleComment()
        {
            buff_.remove_prefix(buff_.size());
        }

        bool is_key_sign(char c) const
        {
            return key_sign_.find(c) != key_sign_.end();
        }

        bool is_operator_sign(const std::string &buff) const
        {
            return operators_.find(buff) != operators_.end();
        }

        std::string_view buff_;
        static uint32_t intend_level_;
        static const std::unordered_map<std::string, Token> keywords_;
        static const std::unordered_set<char> key_sign_;
        static const std::unordered_map<std::string, Token> operators_;
    }; // end of class Tokenazer_Base
    uint32_t Tokenazer_Base::intend_level_ = 0;
    const std::unordered_map<std::string, Token> Tokenazer_Base::keywords_ =
        {
            {"class", token_type::Class{}},
            {"return", token_type::Return{}},
            {"if", token_type::If{}},
            {"else", token_type::Else{}},
            {"def", token_type::Def{}},
            {"print", token_type::Print{}},
            {"and", token_type::And{}},
            {"or", token_type::Or{}},
            {"not", token_type::Not{}},
            {"None", token_type::None{}},
            {"True", token_type::True{}},
            {"False", token_type::False{}},
    };
    const std::unordered_map<std::string, Token> Tokenazer_Base::operators_ =
        {
            {"==", token_type::Eq{}},
            {"!=", token_type::NotEq{}},
            {"<=", token_type::LessOrEq{}},
            {">=", token_type::GreaterOrEq{}},
            {"+", token_type::Char{'+'}},
            {"-", token_type::Char{'-'}},
            {"*", token_type::Char{'*'}},
            {"/", token_type::Char{'/'}},
            {".", token_type::Char{'.'}},
            {",", token_type::Char{','}},
            {"(", token_type::Char{'('}},
            {")", token_type::Char{')'}},
            {"<", token_type::Char{'<'}},
            {">", token_type::Char{'>'}},
            {"=", token_type::Char{'='}},
            {":", token_type::Char{':'}},
    };

    template <typename T>
    concept T_has_put_to_output = requires(T &obj) {
        { obj.push_back(std::declval<Token>()) };
    };

    template <typename T>
        requires T_has_put_to_output<T>
    class Tokenazer final : private parse::Tokenazer_Base
    {
    public:
        Tokenazer(std::string_view buff, T &output)
            : parse::Tokenazer_Base(buff), output_(output) {}
        void PushToOutput(Token &&token) override
        {
            output_.push_back(std::forward<Token>(token));
        }

        void HandleCode()
        {
            parse::Tokenazer_Base::HandleCode();
        }

    private:
        T &output_;
    }; // end of class Tokenazer
    //================================Tokenazer=====================================

    Lexer::Lexer(std::istream &input)
    {
        std::string buff;
        while (std::getline(input, buff))
        {
            if (buff.empty())
                continue;

            Tokenazer tokinazer(buff, tokens_);
            tokinazer.HandleCode();
        }

        tokens_.resize(
            tokens_.size() + Tokenazer_Base::GetIndentLevel(),
            Token{token_type::Dedent{}});
        Tokenazer_Base::EraseIndent();

        tokens_.emplace_back(token_type::Eof{});
        current_token_ = tokens_.begin();
    }

    const Token &Lexer::CurrentToken() const
    {
        return *current_token_;
    }

    Token Lexer::NextToken()
    {
        if (current_token_ != (--tokens_.end()))
        {
            ++current_token_;
        }

        return *current_token_;
    }
} // namespace parse