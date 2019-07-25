





#include "Preprocessor.h"

namespace pp
{

bool Preprocessor::init(int count,
                        const char* const string[],
                        const int length[])
{
    return mLexer.init(count, string, length);
}

int Preprocessor::lex(Token* token)
{
    return mLexer.lex(token);
}

}  

