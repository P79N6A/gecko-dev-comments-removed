





#ifndef COMPILER_PREPROCESSOR_LEXER_H_
#define COMPILER_PREPROCESSOR_LEXER_H_

namespace pp
{

struct Token;

class Lexer
{
  public:
    virtual ~Lexer();

    virtual void lex(Token* token) = 0;
};

}  
#endif  

