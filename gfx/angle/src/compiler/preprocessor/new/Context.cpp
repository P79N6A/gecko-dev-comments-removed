





#include "Context.h"

namespace pp
{

Context::Context(int count, const char* const string[], const int length[],
                 TokenVector* output)
    : input(count, string, length),
      output(output),
      lexer(NULL)
{
}

}  

