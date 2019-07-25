





































#ifndef __nsCharSeparatedTokenizer_h
#define __nsCharSeparatedTokenizer_h

#include "nsDependentSubstring.h"
#include "nsCRT.h"



















template<bool IsWhitespace(PRUnichar) = NS_IsAsciiWhitespace>
class nsCharSeparatedTokenizerTemplate
{
public:
    
    
    enum {
        SEPARATOR_OPTIONAL = 1
    };

    nsCharSeparatedTokenizerTemplate(const nsSubstring& aSource,
                                     PRUnichar aSeparatorChar,
                                     PRUint32  aFlags = 0)
        : mFirstTokenBeganWithWhitespace(false),
          mLastTokenEndedWithWhitespace(false),
          mLastTokenEndedWithSeparator(false),
          mSeparatorChar(aSeparatorChar),
          mFlags(aFlags)
    {
        aSource.BeginReading(mIter);
        aSource.EndReading(mEnd);

        
        while (mIter != mEnd && IsWhitespace(*mIter)) {
            mFirstTokenBeganWithWhitespace = true;
            ++mIter;
        }
    }

    


    bool hasMoreTokens()
    {
        NS_ASSERTION(mIter == mEnd || !IsWhitespace(*mIter),
                     "Should be at beginning of token if there is one");

        return mIter != mEnd;
    }

    bool firstTokenBeganWithWhitespace() const
    {
        return mFirstTokenBeganWithWhitespace;
    }

    bool lastTokenEndedWithSeparator() const
    {
        return mLastTokenEndedWithSeparator;
    }

    bool lastTokenEndedWithWhitespace() const
    {
        return mLastTokenEndedWithWhitespace;
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

          
          mLastTokenEndedWithWhitespace = false;
          while (mIter != mEnd && IsWhitespace(*mIter)) {
              mLastTokenEndedWithWhitespace = true;
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
    bool mFirstTokenBeganWithWhitespace;
    bool mLastTokenEndedWithWhitespace;
    bool mLastTokenEndedWithSeparator;
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

    


    bool hasMoreTokens()
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

    bool isWhitespace(unsigned char aChar)
    {
        return aChar <= ' ' &&
               (aChar == ' ' || aChar == '\n' ||
                aChar == '\r'|| aChar == '\t');
    }
};

#endif
