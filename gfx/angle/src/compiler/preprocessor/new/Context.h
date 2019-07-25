





#ifndef COMPILER_PREPROCESSOR_CONTEXT_H_
#define COMPILER_PREPROCESSOR_CONTEXT_H_

#include "Input.h"
#include "Macro.h"
#include "Token.h"

namespace pp
{

struct Context
{
    Context(int count, const char* const string[], const int length[],
            TokenVector* output);

    Input input;
    TokenVector* output;

    void* lexer;  
    MacroSet macros;  
};

}  
#endif  

