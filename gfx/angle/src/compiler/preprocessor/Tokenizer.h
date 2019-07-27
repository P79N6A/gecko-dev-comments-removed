





#ifndef COMPILER_PREPROCESSOR_TOKENIZER_H_
#define COMPILER_PREPROCESSOR_TOKENIZER_H_

#include "Input.h"
#include "Lexer.h"
#include "pp_utils.h"

namespace pp
{

class Diagnostics;

class Tokenizer : public Lexer
{
  public:
    struct Context
    {
        Diagnostics *diagnostics;

        Input input;
        
        
        
        Input::Location scanLoc;

        bool leadingSpace;
        bool lineStart;
    };

    Tokenizer(Diagnostics *diagnostics);
    ~Tokenizer();

    bool init(size_t count, const char * const string[], const int length[]);

    void setFileNumber(int file);
    void setLineNumber(int line);
    void setMaxTokenSize(size_t maxTokenSize);

    virtual void lex(Token *token);

  private:
    PP_DISALLOW_COPY_AND_ASSIGN(Tokenizer);
    bool initScanner();
    void destroyScanner();

    void *mHandle;  
    Context mContext;  
    size_t mMaxTokenSize; 
};

}  
#endif  

