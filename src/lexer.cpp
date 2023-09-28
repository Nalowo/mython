#include "lexer.h"

#include <algorithm>
#include <charconv>

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
    string line;
    line.reserve(1024);

    stack<uint32_t, std::list<uint32_t>> intend_stack;
    
    while (input.read(line.data(), line.capacity())) 
    {
        
    }
}

void BufferPareser(const std::string& line, std::stack<uint32_t, std::list<uint32_t>>& intend_stack)
{ // можно эту функцию вызывать рекурсивно и передавать в нее char *, постоянно смещать его, так и парсить, хотя, лучше наверно string_view, так будет контроль конца строки
    if (line.empty())
    {
        return;
    }

    switch (line[0])
    {
    case ' ':
        
        break;
    case '#':
        
        break;
    default:
        break;
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