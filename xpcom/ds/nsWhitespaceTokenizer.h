





































#ifndef __nsWhitespaceTokenizer_h
#define __nsWhitespaceTokenizer_h

#include "nsDependentSubstring.h"

class nsWhitespaceTokenizer
{
public:
    nsWhitespaceTokenizer(const nsSubstring& aSource)
    {
        aSource.BeginReading(mIter);
        aSource.EndReading(mEnd);

        while (mIter != mEnd && isWhitespace(*mIter)) {
            ++mIter;
        }
    }

    


    PRBool hasMoreTokens()
    {
        return mIter != mEnd;
    }

    


    const nsDependentSubstring nextToken()
    {
        nsSubstring::const_char_iterator begin = mIter;
        while (mIter != mEnd && !isWhitespace(*mIter)) {
            ++mIter;
        }
        nsSubstring::const_char_iterator end = mIter;
        while (mIter != mEnd && isWhitespace(*mIter)) {
            ++mIter;
        }
        return Substring(begin, end);
    }

private:
    nsSubstring::const_char_iterator mIter, mEnd;

    PRBool isWhitespace(PRUnichar aChar)
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
    {
        aSource.BeginReading(mIter);
        aSource.EndReading(mEnd);

        while (mIter != mEnd && isWhitespace(*mIter)) {
            ++mIter;
        }
    }

    


    PRBool hasMoreTokens()
    {
        return mIter != mEnd;
    }

    


    const nsDependentCSubstring nextToken()
    {
        nsCSubstring::const_char_iterator begin = mIter;
        while (mIter != mEnd && !isWhitespace(*mIter)) {
            ++mIter;
        }
        nsCSubstring::const_char_iterator end = mIter;
        while (mIter != mEnd && isWhitespace(*mIter)) {
            ++mIter;
        }
        return Substring(begin, end);
    }

private:
    nsCSubstring::const_char_iterator mIter, mEnd;

    PRBool isWhitespace(char aChar)
    {
        return aChar <= ' ' &&
               (aChar == ' ' || aChar == '\n' ||
                aChar == '\r'|| aChar == '\t');
    }
};

#endif 
