#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <iostream>

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

Lexer::Lexer(std::istream& input)
{
    const size_t BUFF_SIZE = 1024;
    char line[BUFF_SIZE];

    stack<uint32_t, std::list<uint32_t>> intend_stack;
    
    bool is_newline = true;
    input.read(line, BUFF_SIZE);
    do {
        BufferPareser(line, intend_stack, is_newline);
        is_newline = false;
    
    } while (input.read(line, BUFF_SIZE));
}

void Lexer::BufferPareser(std::string_view line, std::stack<uint32_t, std::list<uint32_t>>& intend_stack, bool is_newline = false)
{ // можно эту функцию вызывать рекурсивно и передавать в нее char *, постоянно смещать его, так и парсить, хотя, лучше наверно string_view, так будет контроль конца строки
    if (line.empty() || line[0] == EOF)
    {
        return;
    }

    switch (line[0])
    {
    case ' ': // реализовать обработку ошибок кол-во отступов // еще нужно реализовать уменьшение отступа
        {
            auto pos_intend_end = line.find_first_not_of(' ', 0);
        
            if (is_newline) // нужно чтобы не учитывались единичные пробелы в начале строки, хотя, может лучше с этим работать в AST
            {
                auto intend_count = (pos_intend_end + 1) / 2;

                intend_stack.push(intend_count);

                for (int i = 0; i < intend_count; i++)
                {
                    tokens.emplace_back(token_type::Indent{});
                }
            }

            BufferPareser({line.data() + pos_intend_end}, intend_stack);
            break;
        }


    case '\n':
        {
            tokens.emplace_back(token_type::Newline{});
            BufferPareser({line.data() + 1}, intend_stack, true);
            break;
        }


    case '#':
        break;

    default:
        {
            size_t pos_end_word = 0;

            for (size_t i = 0; i < line.size(); i++)
            {
                if (std::isalpha(line[i]) || std::isdigit(line[i]))
                {
                    ++pos_end_word;
                }
                else
                {
                    break;
                }
            }

            auto token = keywords.find(line.substr(0, pos_end_word));
            if (token != keywords.end())
            {
                tokens.emplace_back(token->second);
            }
            else
            {
                tokens.emplace_back(token_type::Id{std::string{line.substr(0, pos_end_word)}});
            }

            BufferPareser({line.data() + pos_end_word}, intend_stack);
            break;
        }
    }

}

const Token& Lexer::CurrentToken() const {
    // Заглушка. to do
    // вернуть значение "вершины"
    throw std::logic_error("Not implemented"s);
}

Token Lexer::NextToken() {
    // Заглушка. to do
    // инкрементировать вершину
    throw std::logic_error("Not implemented"s);
}

}  // namespace parse