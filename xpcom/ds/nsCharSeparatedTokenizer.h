





































#ifndef __nsCharSeparatedTokenizer_h
#define __nsCharSeparatedTokenizer_h

#include "nsDependentSubstring.h"
#include "nsCRT.h"



















template<PRBool IsWhitespace(PRUnichar) = NS_IsAsciiWhitespace>
class nsCharSeparatedTokenizerTemplate
{
public:
    
    
    enum {
        SEPARATOR_OPTIONAL = 1
    };

    nsCharSeparatedTokenizerTemplate(const nsSubstring& aSource,
                                     PRUnichar aSeparatorChar,
                                     PRUint32  aFlags = 0)
        : mLastTokenEndedWithSeparator(PR_FALSE),
          mSeparatorChar(aSeparatorChar),
          mFlags(aFlags)
    {
        aSource.BeginReading(mIter);
        aSource.EndReading(mEnd);

        
        while (mIter != mEnd && IsWhitespace(*mIter)) {
            ++mIter;
        }
    }

    


    PRBool hasMoreTokens()
    {
        NS_ASSERTION(mIter == mEnd || !IsWhitespace(*mIter),
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

        NS_ASSERTION(mIter == mEnd || !IsWhitespace(*mIter),
                     "Should be at beginning of token if there is one");

        
        
        while (mIter != mEnd && *mIter != mSeparatorChar) {
          
          while (mIter != mEnd &&
                 !IsWhitespace(*mIter) && *mIter != mSeparatorChar) {
              ++mIter;
          }
          end = mIter;

          
          while (mIter != mEnd && IsWhitespace(*mIter)) {
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

            while (mIter != mEnd && IsWhitespace(*mIter)) {
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
};

class nsCharSeparatedTokenizer: public nsCharSeparatedTokenizerTemplate<>
{
public:
    nsCharSeparatedTokenizer(const nsSubstring& aSource,
                             PRUnichar aSeparatorChar,
                             PRUint32  aFlags = 0)
      : nsCharSeparatedTokenizerTemplate<>(aSource, aSeparatorChar, aFlags)
    {
    }
};

class nsCCharSeparatedTokenizer
{
public:
    nsCCharSeparatedTokenizer(const nsCSubstring& aSource,
                              char aSeparatorChar)
        : mSeparatorChar(aSeparatorChar)
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

        
        while (mIter != mEnd && *mIter != mSeparatorChar) {
          while (mIter != mEnd &&
                 !isWhitespace(*mIter) && *mIter != mSeparatorChar) {
              ++mIter;
          }
          end = mIter;

          while (mIter != mEnd && isWhitespace(*mIter)) {
              ++mIter;
          }
        }

        
        if (mIter != mEnd) {
            NS_ASSERTION(*mIter == mSeparatorChar, "Ended loop too soon");
            ++mIter;

            while (mIter != mEnd && isWhitespace(*mIter)) {
                ++mIter;
            }
        }

        return Substring(begin, end);
    }

private:
    nsCSubstring::const_char_iterator mIter, mEnd;
    char mSeparatorChar;

    PRBool isWhitespace(unsigned char aChar)
    {
        return aChar <= ' ' &&
               (aChar == ' ' || aChar == '\n' ||
                aChar == '\r'|| aChar == '\t');
    }
};

#endif
