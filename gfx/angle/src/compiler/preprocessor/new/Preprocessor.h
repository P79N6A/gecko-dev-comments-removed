





#ifndef COMPILER_PREPROCESSOR_PREPROCESSOR_H_
#define COMPILER_PREPROCESSOR_PREPROCESSOR_H_

#include "common/angleutils.h"
#include "Token.h"

namespace pp
{

struct Context;

class Preprocessor
{
  public:
    Preprocessor() { }

    bool process(int count, const char* const string[], const int length[]);

    TokenIterator begin() const { return mTokens.begin(); }
    TokenIterator end() const { return mTokens.end(); }

  private:
    DISALLOW_COPY_AND_ASSIGN(Preprocessor);

    static bool initLexer(Context* context);
    static void destroyLexer(Context* context);
    static bool parse(Context* context);

    void clearResults();

    TokenVector mTokens;  
};

}  
#endif  

