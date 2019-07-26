




#ifndef __nsCharSeparatedTokenizer_h
#define __nsCharSeparatedTokenizer_h

#include "mozilla/RangedPtr.h"

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
                                     uint32_t  aFlags = 0)
        : mFirstTokenBeganWithWhitespace(false),
          mLastTokenEndedWithWhitespace(false),
          mLastTokenEndedWithSeparator(false),
          mSeparatorChar(aSeparatorChar),
          mFlags(aFlags),
          mIter(aSource.Data(), aSource.Length()),
          mEnd(aSource.Data() + aSource.Length(), aSource.Data(),
               aSource.Length())
    {
        
        while (mIter < mEnd && IsWhitespace(*mIter)) {
            mFirstTokenBeganWithWhitespace = true;
            ++mIter;
        }
    }

    


    bool hasMoreTokens()
    {
        NS_ASSERTION(mIter == mEnd || !IsWhitespace(*mIter),
                     "Should be at beginning of token if there is one");

        return mIter < mEnd;
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
        mozilla::RangedPtr<const PRUnichar> tokenStart = mIter, tokenEnd = mIter;

        NS_ASSERTION(mIter == mEnd || !IsWhitespace(*mIter),
                     "Should be at beginning of token if there is one");

        
        
        while (mIter < mEnd && *mIter != mSeparatorChar) {
          
          while (mIter < mEnd &&
                 !IsWhitespace(*mIter) && *mIter != mSeparatorChar) {
              ++mIter;
          }
          tokenEnd = mIter;

          
          mLastTokenEndedWithWhitespace = false;
          while (mIter < mEnd && IsWhitespace(*mIter)) {
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
                     (mLastTokenEndedWithSeparator == (mIter < mEnd)),
                     "If we require a separator and haven't hit the end of "
                     "our string, then we shouldn't have left the loop "
                     "unless we hit a separator");

        
        if (mLastTokenEndedWithSeparator) {
            ++mIter;

            while (mIter < mEnd && IsWhitespace(*mIter)) {
                ++mIter;
            }
        }

        return Substring(tokenStart.get(), tokenEnd.get());
    }

private:
    mozilla::RangedPtr<const PRUnichar> mIter;
    const mozilla::RangedPtr<const PRUnichar> mEnd;
    bool mFirstTokenBeganWithWhitespace;
    bool mLastTokenEndedWithWhitespace;
    bool mLastTokenEndedWithSeparator;
    PRUnichar mSeparatorChar;
    uint32_t  mFlags;
};

class nsCharSeparatedTokenizer: public nsCharSeparatedTokenizerTemplate<>
{
public:
    nsCharSeparatedTokenizer(const nsSubstring& aSource,
                             PRUnichar aSeparatorChar,
                             uint32_t  aFlags = 0)
      : nsCharSeparatedTokenizerTemplate<>(aSource, aSeparatorChar, aFlags)
    {
    }
};

class nsCCharSeparatedTokenizer
{
public:
    nsCCharSeparatedTokenizer(const nsCSubstring& aSource,
                              char aSeparatorChar)
        : mSeparatorChar(aSeparatorChar),
          mIter(aSource.Data(), aSource.Length()),
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
        mozilla::RangedPtr<const char> tokenStart = mIter,tokenEnd = mIter;

        
        while (mIter < mEnd && *mIter != mSeparatorChar) {
          while (mIter < mEnd &&
                 !isWhitespace(*mIter) && *mIter != mSeparatorChar) {
              ++mIter;
          }
          tokenEnd = mIter;

          while (mIter < mEnd && isWhitespace(*mIter)) {
              ++mIter;
          }
        }

        
        if (mIter < mEnd) {
            NS_ASSERTION(*mIter == mSeparatorChar, "Ended loop too soon");
            ++mIter;

            while (mIter < mEnd && isWhitespace(*mIter)) {
                ++mIter;
            }
        }

        return Substring(tokenStart.get(), tokenEnd.get());
    }

private:
    mozilla::RangedPtr<const char> mIter;
    const mozilla::RangedPtr<const char> mEnd;
    char mSeparatorChar;

    bool isWhitespace(unsigned char aChar)
    {
        return aChar <= ' ' &&
               (aChar == ' ' || aChar == '\n' ||
                aChar == '\r'|| aChar == '\t');
    }
};

#endif
