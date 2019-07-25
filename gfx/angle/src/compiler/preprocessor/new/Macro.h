





#ifndef COMPILER_PREPROCESSOR_MACRO_H_
#define COMPILER_PREPROCESSOR_MACRO_H_

#include <map>
#include <string>

#include "Token.h"

namespace pp
{

struct Macro
{
    enum Type
    {
        kTypeObj,
        kTypeFunc
    };
    Type type;
    std::string identifier;
    TokenVector parameters;
    TokenVector replacements;
};
typedef std::map<std::string, Macro> MacroSet;

}  
#endif COMPILER_PREPROCESSOR_MACRO_H_

