





#ifndef COMPILER_PREPROCESSOR_MACRO_H_
#define COMPILER_PREPROCESSOR_MACRO_H_

#include <map>
#include <string>
#include <vector>

namespace pp
{

struct Token;

struct Macro
{
    enum Type
    {
        kTypeObj,
        kTypeFunc
    };
    typedef std::vector<std::string> Parameters;
    typedef std::vector<Token> Replacements;

    Macro() : predefined(false), disabled(false), type(kTypeObj) { }
    bool equals(const Macro& other) const;

    bool predefined;
    mutable bool disabled;

    Type type;
    std::string name;
    Parameters parameters;
    Replacements replacements;
};

typedef std::map<std::string, Macro> MacroSet;

}  
#endif  
