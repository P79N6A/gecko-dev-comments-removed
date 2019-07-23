





































#ifndef __nsCommaSeparatedTokenizer_h
#define __nsCommaSeparatedTokenizer_h

#include "nsDependentSubstring.h"
















class nsCommaSeparatedTokenizer
{
public:
    nsCommaSeparatedTokenizer(const nsSubstring& aSource)
    {
        aSource.BeginReading(mIter);
        aSource.EndReading(mEnd);

        while (mIter != mEnd && isWhitespace(*mIter)) {
            ++mIter;
        }
    }

    


    PRBool hasMoreTokens()
    {
        NS_ASSERTION(mIter == mEnd || !isWhitespace(*mIter),
                     "Should be at beginning of token if there is one");

        return mIter != mEnd;
    }

    PRBool lastTokenEndedWithComma()
    {
        return mLastTokenEndedWithComma;
    }

    


    const nsDependentSubstring nextToken()
    {
        nsSubstring::const_char_iterator end = mIter, begin = mIter;

        NS_ASSERTION(mIter == mEnd || !isWhitespace(*mIter),
                     "Should be at beginning of token if there is one");

        
        while (mIter != mEnd && *mIter != ',') {
          while (mIter != mEnd && !isWhitespace(*mIter) && *mIter != ',') {
              ++mIter;
          }
          end = mIter;

          while (mIter != mEnd && isWhitespace(*mIter)) {
              ++mIter;
          }
        }
        mLastTokenEndedWithComma = mIter != mEnd;

        
        if (mLastTokenEndedWithComma) {
            NS_ASSERTION(*mIter == ',', "Ended loop too soon");
            ++mIter;

            while (mIter != mEnd && isWhitespace(*mIter)) {
                ++mIter;
            }
        }
        
        return Substring(begin, end);
    }

private:
    nsSubstring::const_char_iterator mIter, mEnd;
    PRPackedBool mLastTokenEndedWithComma;

    PRBool isWhitespace(PRUnichar aChar)
    {
        return aChar <= ' ' &&
               (aChar == ' ' || aChar == '\n' ||
                aChar == '\r'|| aChar == '\t');
    }
};

class nsCCommaSeparatedTokenizer
{
public:
    nsCCommaSeparatedTokenizer(const nsCSubstring& aSource)
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
        nsCSubstring::const_char_iterator end = mIter, begin = mIter;

        
        while (mIter != mEnd && *mIter != ',') {
          while (mIter != mEnd && !isWhitespace(*mIter) && *mIter != ',') {
              ++mIter;
          }
          end = mIter;

          while (mIter != mEnd && isWhitespace(*mIter)) {
              ++mIter;
          }
        }
        
        
        if (mIter != mEnd) {
            NS_ASSERTION(*mIter == ',', "Ended loop too soon");
            ++mIter;

            while (mIter != mEnd && isWhitespace(*mIter)) {
                ++mIter;
            }
        }
        
        return Substring(begin, end);
    }

private:
    nsCSubstring::const_char_iterator mIter, mEnd;

    PRBool isWhitespace(unsigned char aChar)
    {
        return aChar <= ' ' &&
               (aChar == ' ' || aChar == '\n' ||
                aChar == '\r'|| aChar == '\t');
    }
};

#endif 
