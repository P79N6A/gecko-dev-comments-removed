





































#ifndef __nsCharSeparatedTokenizer_h
#define __nsCharSeparatedTokenizer_h

#include "nsDependentSubstring.h"
















class nsCharSeparatedTokenizer
{
public:
    
    
    enum {
        SEPARATOR_OPTIONAL = 1
    };

    nsCharSeparatedTokenizer(const nsSubstring& aSource,
                             PRUnichar aSeparatorChar,
                             PRUint32  aFlags = 0)
        : mLastTokenEndedWithSeparator(PR_FALSE),
          mSeparatorChar(aSeparatorChar),
          mFlags(aFlags)
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

    PRBool lastTokenEndedWithSeparator()
    {
        return mLastTokenEndedWithSeparator;
    }

    


    const nsDependentSubstring nextToken()
    {
        nsSubstring::const_char_iterator end = mIter, begin = mIter;

        NS_ASSERTION(mIter == mEnd || !isWhitespace(*mIter),
                     "Should be at beginning of token if there is one");

        
        
        while (mIter != mEnd && *mIter != mSeparatorChar) {
          
          while (mIter != mEnd &&
                 !isWhitespace(*mIter) && *mIter != mSeparatorChar) {
              ++mIter;
          }
          end = mIter;

          
          while (mIter != mEnd && isWhitespace(*mIter)) {
              ++mIter;
          }
          if (mFlags & SEPARATOR_OPTIONAL) {
            
            
            break;
          } 
        }

        mLastTokenEndedWithSeparator = (mIter != mEnd &&
                                        *mIter == mSeparatorChar);
        NS_ASSERTION((mFlags & SEPARATOR_OPTIONAL) ||
                     (mLastTokenEndedWithSeparator == (mIter != mEnd)),
                     "If we require a separator and haven't hit the end of "
                     "our string, then we shouldn't have left the loop "
                     "unless we hit a separator");

        
        if (mLastTokenEndedWithSeparator) {
            ++mIter;

            while (mIter != mEnd && isWhitespace(*mIter)) {
                ++mIter;
            }
        }
        
        return Substring(begin, end);
    }

private:
    nsSubstring::const_char_iterator mIter, mEnd;
    PRPackedBool mLastTokenEndedWithSeparator;
    PRUnichar mSeparatorChar;
    PRUint32  mFlags;

    PRBool isWhitespace(PRUnichar aChar)
    {
        return aChar <= ' ' &&
               (aChar == ' ' || aChar == '\n' ||
                aChar == '\r'|| aChar == '\t');
    }
};

class nsCommaSeparatedTokenizer : public nsCharSeparatedTokenizer
{
public:
    nsCommaSeparatedTokenizer(const nsSubstring& aSource)
        : nsCharSeparatedTokenizer(aSource, ',') {}
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
