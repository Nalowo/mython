#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <unordered_map>
#include <unordered_set>

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

    template <typename T, size_t N>
    constexpr size_t array_size(const T (&)[N]) noexcept
    {
        return N;
    }

    static void ClearBuff(char *buff, size_t buff_size)
    {
        for (size_t i = 0; i < buff_size; ++i)
        {
            buff[i] = '\0';
        }
    }

    class Tokinazer_Base;
    template <typename T>
    class Tokinazer final;

    Lexer::Lexer(std::istream &input)
    {
        char buff[1024];
        while (input.good())
        {
            ClearBuff(buff, array_size(buff));
            input.read(buff, array_size(buff) - 1);
            Tokinazer tokinazer(buff, tokens_);
            tokinazer.HandleCode();
        }
    }

    const Token &Lexer::CurrentToken() const
    {
        // Заглушка. Реализуйте метод самостоятельно
        throw std::logic_error("Not implemented"s);
    }

    Token Lexer::NextToken()
    {
        // Заглушка. Реализуйте метод самостоятельно
        throw std::logic_error("Not implemented"s);
    }

    class Tokinazer_Base
    {
    protected:
        Tokinazer_Base(std::string_view buff) : buff_(buff) {}
        virtual void PushToOutput(Token &&) = 0;
        virtual ~Tokinazer_Base() = default;

        void HandleCode()
        {
            while (buff_.size() > 0)
            {
                if (std::isspace(buff_[0]))
                {
                    if (buff_[0] == '\n')
                    {
                        PushToOutput(Token{token_type::Newline{}});
                        buff_.remove_prefix(1);
                        PushToOutput(HandleIntend());
                    }
                }
                else if (std::isalpha(buff_[0]) || buff_[0] == '_')
                {
                    PushToOutput(HandleWord());
                }
                // нужно дальше реализовать обработку 
            }
        }

    private:
        Token HandleIntend()
        {
            Token token;
            uint32_t curr_inten_lvl = 0;

            while (buff_.size() > 0 && buff_[0] == ' ')
            {
                ++curr_inten_lvl;
                buff_.remove_prefix(1);
            }

            if (curr_inten_lvl > intend_level_)
            {
                token = Token{token_type::Indent{}};
            }
            else if (curr_inten_lvl < intend_level_)
            {
                token = Token{token_type::Dedent{}};
            }

            intend_level_ = curr_inten_lvl;
            return token;
        }

        Token HandleWord()
        {
            Token token;
            size_t end_pos = 0;

            while (buff_.size() > 0 && (std::isalnum(buff_[0]) || buff_[0] == '_'))
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
                token = Token{token_type::Id{word}};
            }

            return token;
        }

        std::string_view buff_;
        static uint32_t intend_level_;
        static const std::unordered_map<std::string, Token> keywords_;
        static const std::unordered_set<char> key_sign_;
    }; // end of class Tokinazer_Base
    uint32_t Tokinazer_Base::intend_level_ = 0;

    const std::unordered_map<std::string, Token> Tokinazer_Base::keywords_ =
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
            {"==", token_type::Eq{}},
            {"!=", token_type::NotEq{}},
            {"<=", token_type::LessOrEq{}},
            {">=", token_type::GreaterOrEq{}},
            {"None", token_type::None{}},
            {"True", token_type::True{}},
            {"False", token_type::False{}},
    };
    const std::unordered_set<char> Tokinazer_Base::key_sign_{'=', '*', '.', ',', '(', '+', '<', ')', '-'};

    template <typename T>
    class Tokinazer final : private Tokinazer_Base
    {
    public:
        Tokinazer(std::string_view buff, T &output) : Tokinazer_Base(buff), output_(output) {}
        void PushToOutput(Token &&token) override
        {
            output_.push_back(std::forward<Token>(token));
        }

        void HandleCode()
        {
            Tokinazer_Base::HandleCode();
        }

    private:
        T &output_;
    }; // end of class Tokinazer

} // namespace parse