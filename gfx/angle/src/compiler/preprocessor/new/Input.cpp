





#include "Input.h"

#include <cassert>
#include <cstdio>

namespace pp
{

Input::Input(int count, const char* const string[], const int length[])
    : mCount(count),
      mString(string),
      mLength(length),
      mIndex(-1),
      mSize(0),
      mError(kErrorNone),
      mState(kStateInitial)
{
    assert(mCount >= 0);
    switchToNextString();
}

bool Input::eof() const
{
    assert(mIndex <= mCount);
    return mIndex == mCount;
}

int Input::read(char* buf, int bufSize)
{
    int nread = 0;
    int startIndex = mIndex;
    
    while ((mIndex == startIndex) && (nread < bufSize))
    {
        int c = getChar();
        if (c == EOF)
        {
            if (mState == kStateBlockComment)
                mError = kErrorUnexpectedEOF;
            break;
        }

        switch (mState)
        {
          case kStateInitial:
            if (c == '/')
            {
                
                switch (peekChar())
                {
                  case '/':
                    getChar();  
                    mState = kStateLineComment;
                    break;
                  case '*':
                    getChar();  
                    mState = kStateBlockComment;
                    break;
                  default:
                    
                    buf[nread++] = c;
                    break;
                }
            } else
            {
                buf[nread++] = c;
            }
            break;

          case kStateLineComment:
            if (c == '\n')
            {
                buf[nread++] = c;
                mState = kStateInitial;
            }
            break;

          case kStateBlockComment:
            if (c == '*' && (peekChar() == '/'))
            {
                getChar();   
                buf[nread++] = ' ';  
                mState = kStateInitial;
            } else if (c == '\n')
            {
                
                buf[nread++] = c;
            }
            break;

          default:
            assert(false);
            break;
        }
    }

    return nread;
}

int Input::getChar()
{
    if (eof()) return EOF;

    const char* str = mString[mIndex];
    int c = str[mSize++];

    
    int length = stringLength(mIndex);
    
    assert(length != 0);
    if (((length < 0) && (str[mSize] == '\0')) ||
        ((length > 0) && (mSize == length)))
        switchToNextString();

    return c;
}

int Input::peekChar()
{
    
    int index = mIndex;
    int size = mSize;
    int c = getChar();

    
    mIndex = index;
    mSize = size;
    return c;
}

void Input::switchToNextString()
{
    assert(mIndex < mCount);

    mSize = 0;
    do
    {
        ++mIndex;
    } while (!eof() && isStringEmpty(mIndex));
}

bool Input::isStringEmpty(int index)
{
    assert(index < mCount);

    const char* str = mString[mIndex];
    int length = stringLength(mIndex);
    return (length == 0) || ((length < 0) && (str[0] == '\0'));
}

int Input::stringLength(int index)
{
    assert(index < mCount);
    return mLength ? mLength[index] : -1;
}

}  

