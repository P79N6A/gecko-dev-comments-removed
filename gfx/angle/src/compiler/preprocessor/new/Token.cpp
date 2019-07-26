





#include "Token.h"

namespace pp
{

std::ostream& operator<<(std::ostream& out, const Token& token)
{
    if (token.hasLeadingSpace())
        out << " ";

    out << token.value;
    return out;
}

}  
