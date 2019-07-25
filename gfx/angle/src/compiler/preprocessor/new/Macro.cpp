





#include "Macro.h"

#include "Token.h"

namespace pp
{

bool Macro::equals(const Macro& other) const
{
    return (type == other.type) &&
           (name == other.name) &&
           (parameters == other.parameters) &&
           (replacements == other.replacements);
}

}  

