





#ifndef COMPILER_PREPROCESSOR_PREPROCESSOR_H_
#define COMPILER_PREPROCESSOR_PREPROCESSOR_H_

#include "pp_utils.h"

namespace pp
{

class Diagnostics;
class DirectiveHandler;
struct PreprocessorImpl;
struct Token;

class Preprocessor
{
  public:
    Preprocessor(Diagnostics* diagnostics, DirectiveHandler* directiveHandler);
    ~Preprocessor();

    
    
    
    
    
    
    
    
    
    bool init(int count, const char* const string[], const int length[]);
    
    void predefineMacro(const char* name, int value);

    void lex(Token* token);

  private:
    PP_DISALLOW_COPY_AND_ASSIGN(Preprocessor);

    PreprocessorImpl* mImpl;
};

}  
#endif  

