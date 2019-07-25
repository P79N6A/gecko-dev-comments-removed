





#include "Lexer.h"

#include <cassert>

#include "Input.h"

namespace pp
{

Lexer::Lexer() : mHandle(0), mLeadingSpace(false)
{
}

Lexer::~Lexer()
{
    destroyLexer();
}

bool Lexer::init(int count, const char* const string[], const int length[])
{
    assert((count >=0) && (string != NULL));
    if ((count < 0) || (string == NULL))
        return false;

    mInput.reset(new Input(count, string, length));
    return initLexer();
}

}  

