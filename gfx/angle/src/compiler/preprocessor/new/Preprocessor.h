





#ifndef COMPILER_PREPROCESSOR_PREPROCESSOR_H_
#define COMPILER_PREPROCESSOR_PREPROCESSOR_H_

#include "common/angleutils.h"
#include "Token.h"

namespace pp
{

class Context;

class Preprocessor
{
  public:
    Preprocessor();
    ~Preprocessor();

    bool init();

    bool process(int count, const char* const string[], const int length[]);
    TokenIterator begin() const { return mTokens.begin(); }
    TokenIterator end() const { return mTokens.end(); }

  private:
    DISALLOW_COPY_AND_ASSIGN(Preprocessor);

    
    void reset();

    Context* mContext;
    TokenVector mTokens;  
};

}  
#endif  

