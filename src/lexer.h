#pragma once

#include <iosfwd>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <stack>

namespace parse {

namespace token_type {
struct Number {  // Лексема «число»
    int value;   // число
};

struct Id {             // Лексема «идентификатор»
    std::string value;  // Имя идентификатора
};

struct Char {    // Лексема «символ»
    char value;  // код символа
};

struct String {  // Лексема «строковая константа»
    std::string value;
};

struct Class {};    // Лексема «class»
struct Return {};   // Лексема «return»
struct If {};       // Лексема «if»
struct Else {};     // Лексема «else»
struct Def {};      // Лексема «def»
struct Newline {};  // Лексема «конец строки»
struct Print {};    // Лексема «print»
struct Indent {};  // Лексема «увеличение отступа», соответствует двум пробелам
struct Dedent {};  // Лексема «уменьшение отступа»
struct Eof {};     // Лексема «конец файла»
struct And {};     // Лексема «and»
struct Or {};      // Лексема «or»
struct Not {};     // Лексема «not»
struct Eq {};      // Лексема «==»
struct NotEq {};   // Лексема «!=»
struct LessOrEq {};     // Лексема «<=»
struct GreaterOrEq {};  // Лексема «>=»
struct None {};         // Лексема «None»
struct True {};         // Лексема «True»
struct False {};        // Лексема «False»
}  // namespace token_type

using TokenBase
    = std::variant<token_type::Number, token_type::Id, token_type::Char, token_type::String,
                   token_type::Class, token_type::Return, token_type::If, token_type::Else,
                   token_type::Def, token_type::Newline, token_type::Print, token_type::Indent,
                   token_type::Dedent, token_type::And, token_type::Or, token_type::Not,
                   token_type::Eq, token_type::NotEq, token_type::LessOrEq, token_type::GreaterOrEq,
                   token_type::None, token_type::True, token_type::False, token_type::Eof>;

struct Token : TokenBase {
    using TokenBase::TokenBase;

    template <typename T>
    [[nodiscard]] bool Is() const {
        return std::holds_alternative<T>(*this);
    }

    template <typename T>
    [[nodiscard]] const T& As() const {
        return std::get<T>(*this);
    }

    template <typename T>
    [[nodiscard]] const T* TryAs() const {
        return std::get_if<T>(this);
    }
};

bool operator==(const Token& lhs, const Token& rhs);
bool operator!=(const Token& lhs, const Token& rhs);

std::ostream& operator<<(std::ostream& os, const Token& rhs);

class LexerError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class Lexer {
public:
    explicit Lexer(std::istream& input);

    // Возвращает ссылку на текущий токен или token_type::Eof, если поток токенов закончился
    [[nodiscard]] const Token& CurrentToken() const;

    // Возвращает следующий токен, либо token_type::Eof, если поток токенов закончился
    Token NextToken();

    // Если текущий токен имеет тип T, метод возвращает ссылку на него.
    // В противном случае метод выбрасывает исключение LexerError
    template <typename T>
    const T& Expect() const 
    {
        if (curr_token_->Is<T>())
        {
            return curr_token_->As<T>();
        }
        else
        {
            throw LexerError("Is not required type"s);
        }
    }

    // Метод проверяет, что текущий токен имеет тип T, а сам токен содержит значение value.
    // В противном случае метод выбрасывает исключение LexerError
    template <typename T, typename U>
    void Expect(const U& value) const 
    {
        if (!curr_token_->Is<T>())
        {
            throw LexerError("Is not required type"s);
        }

        auto value = curr_token_->TryAs<T>();

        if(value)
        {
            if (value->value != value)
            {
                throw LexerError("Is not required value"s);
            }
        }
        else 
        {
            throw LexerError("Is not required type"s);
        }
    }

    // Если следующий токен имеет тип T, метод возвращает ссылку на него.
    // В противном случае метод выбрасывает исключение LexerError
    template <typename T>
    const T& ExpectNext() 
    {
        auto iter_buff = curr_token_;
        if (iter_buff != tokens_.end())
        {
            ++iter_buff;
        }
        else
        {
            throw LexerError("Next token not found"s);
        }

        if (iter_buff->Is<T>())
        {
            return iter_buff->As<T>();
        }
        else
        {
            throw LexerError("Is not required type"s);
        }
    }

    // Метод проверяет, что следующий токен имеет тип T, а сам токен содержит значение value.
    // В противном случае метод выбрасывает исключение LexerError
    template <typename T, typename U>
    void ExpectNext(const U& value) 
    {
        auto iter_buff = curr_token_;
        if (*iter_buff != Token(token_type::Eof))
        {
            ++iter_buff;
        }
        else
        {
            throw LexerError("Next token not found"s);
        }
        
        if (!iter_buff->Is<T>())
        {
            throw LexerError("Is not required type"s);
        }

        auto value = iter_buff->TryAs<T>();

        if(value)
        {
            if (value->value != value)
            {
                throw LexerError("Is not required value"s);
            }
        }
        else 
        {
            throw LexerError("Is not required type"s);
        }
    }

    void PrintTokens();

private:

    void BufferPareser(std::string_view line, bool is_newline);

    class Tokenizer_interface
    {
    public:
        virtual ~Tokenizer_interface() = default;

        virtual void Parse(std::string_view /*line*/) = 0;
        virtual Token GetToken() = 0;
        virtual size_t GetTokenWordEnd() = 0;
    };

    class TokenizerEof;
    class TokenizerIntend;
    class TokenizerNewline;
    class TokenizerSomeWord;

    std::list<Token> tokens_;
    std::list<Token>::iterator curr_token_ = tokens_.end();
};

}  // namespace parse