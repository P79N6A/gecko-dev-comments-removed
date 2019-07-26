




#ifndef __nsWhitespaceTokenizer_h
#define __nsWhitespaceTokenizer_h

#include "mozilla/RangedPtr.h"
#include "nsDependentSubstring.h"

class nsWhitespaceTokenizer
{
public:
    nsWhitespaceTokenizer(const nsSubstring& aSource)
        : mIter(aSource.Data(), aSource.Length()),
          mEnd(aSource.Data() + aSource.Length(), aSource.Data(),
               aSource.Length())
    {
        while (mIter < mEnd && isWhitespace(*mIter)) {
            ++mIter;
        }
    }

    


    bool hasMoreTokens()
    {
        return mIter < mEnd;
    }

    


    const nsDependentSubstring nextToken()
    {
        const mozilla::RangedPtr<const PRUnichar> tokenStart = mIter;
        while (mIter < mEnd && !isWhitespace(*mIter)) {
            ++mIter;
        }
        const mozilla::RangedPtr<const PRUnichar> tokenEnd = mIter;
        while (mIter < mEnd && isWhitespace(*mIter)) {
            ++mIter;
        }
        return Substring(tokenStart.get(), tokenEnd.get());
    }

private:
    mozilla::RangedPtr<const PRUnichar> mIter;
    const mozilla::RangedPtr<const PRUnichar> mEnd;

    bool isWhitespace(PRUnichar aChar)
    {
        return aChar <= ' ' &&
               (aChar == ' ' || aChar == '\n' ||
                aChar == '\r'|| aChar == '\t');
    }
};

class nsCWhitespaceTokenizer
{
public:
    nsCWhitespaceTokenizer(const nsCSubstring& aSource)
        : mIter(aSource.Data(), aSource.Length()),
          mEnd(aSource.Data() + aSource.Length(), aSource.Data(),
               aSource.Length())
    {
        while (mIter < mEnd && isWhitespace(*mIter)) {
            ++mIter;
        }
    }

    


    bool hasMoreTokens()
    {
        return mIter < mEnd;
    }

    


    const nsDependentCSubstring nextToken()
    {
        const mozilla::RangedPtr<const char> tokenStart = mIter;
        while (mIter < mEnd && !isWhitespace(*mIter)) {
            ++mIter;
        }
        const mozilla::RangedPtr<const char> tokenEnd = mIter;
        while (mIter < mEnd && isWhitespace(*mIter)) {
            ++mIter;
        }
        return Substring(tokenStart.get(), tokenEnd.get());
    }

private:
    mozilla::RangedPtr<const char> mIter;
    const mozilla::RangedPtr<const char> mEnd;

    bool isWhitespace(char aChar)
    {
        return aChar <= ' ' &&
               (aChar == ' ' || aChar == '\n' ||
                aChar == '\r'|| aChar == '\t');
    }
};

#endif 
