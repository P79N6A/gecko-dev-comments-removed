




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
        : mIter(aSource.Data(), aSource.Length()),
          mEnd(aSource.Data() + aSource.Length(), aSource.Data(),
               aSource.Length()),
          mSeparatorChar(aSeparatorChar),
          mWhitespaceBeforeFirstToken(false),
          mWhitespaceAfterCurrentToken(false),
          mSeparatorAfterCurrentToken(false),
          mSeparatorOptional(aFlags & SEPARATOR_OPTIONAL)
    {
        
        while (mIter < mEnd && IsWhitespace(*mIter)) {
            mWhitespaceBeforeFirstToken = true;
            ++mIter;
        }
    }

    


    bool hasMoreTokens() const
    {
        MOZ_ASSERT(mIter == mEnd || !IsWhitespace(*mIter),
                   "Should be at beginning of token if there is one");

        return mIter < mEnd;
    }

    


    bool whitespaceBeforeFirstToken() const
    {
        return mWhitespaceBeforeFirstToken;
    }

    




    bool separatorAfterCurrentToken() const
    {
        return mSeparatorAfterCurrentToken;
    }

    


    bool whitespaceAfterCurrentToken() const
    {
        return mWhitespaceAfterCurrentToken;
    }

    


    const nsDependentSubstring nextToken()
    {
        mozilla::RangedPtr<const PRUnichar> tokenStart = mIter, tokenEnd = mIter;

        MOZ_ASSERT(mIter == mEnd || !IsWhitespace(*mIter),
                   "Should be at beginning of token if there is one");

        
        
        while (mIter < mEnd && *mIter != mSeparatorChar) {
          
          while (mIter < mEnd &&
                 !IsWhitespace(*mIter) && *mIter != mSeparatorChar) {
              ++mIter;
          }
          tokenEnd = mIter;

          
          mWhitespaceAfterCurrentToken = false;
          while (mIter < mEnd && IsWhitespace(*mIter)) {
              mWhitespaceAfterCurrentToken = true;
              ++mIter;
          }
          if (mSeparatorOptional) {
            
            
            break;
          } 
        }

        mSeparatorAfterCurrentToken = (mIter != mEnd &&
                                       *mIter == mSeparatorChar);
        MOZ_ASSERT(mSeparatorOptional ||
                   (mSeparatorAfterCurrentToken == (mIter < mEnd)),
                   "If we require a separator and haven't hit the end of "
                   "our string, then we shouldn't have left the loop "
                   "unless we hit a separator");

        
        if (mSeparatorAfterCurrentToken) {
            ++mIter;

            while (mIter < mEnd && IsWhitespace(*mIter)) {
                mWhitespaceAfterCurrentToken = true;
                ++mIter;
            }
        }

        return Substring(tokenStart.get(), tokenEnd.get());
    }

private:
    mozilla::RangedPtr<const PRUnichar> mIter;
    const mozilla::RangedPtr<const PRUnichar> mEnd;
    PRUnichar mSeparatorChar;
    bool mWhitespaceBeforeFirstToken;
    bool mWhitespaceAfterCurrentToken;
    bool mSeparatorAfterCurrentToken;
    bool mSeparatorOptional;
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

template<bool IsWhitespace(PRUnichar) = NS_IsAsciiWhitespace>
class nsCCharSeparatedTokenizerTemplate
{
public:
    
    
    enum {
        SEPARATOR_OPTIONAL = 1
    };

    nsCCharSeparatedTokenizerTemplate(const nsCSubstring& aSource,
                                      char aSeparatorChar,
                                      uint32_t  aFlags = 0)
        : mIter(aSource.Data(), aSource.Length()),
          mEnd(aSource.Data() + aSource.Length(), aSource.Data(),
               aSource.Length()),
          mSeparatorChar(aSeparatorChar),
          mWhitespaceBeforeFirstToken(false),
          mWhitespaceAfterCurrentToken(false),
          mSeparatorAfterCurrentToken(false),
          mSeparatorOptional(aFlags & SEPARATOR_OPTIONAL)
    {
        
        while (mIter < mEnd && IsWhitespace(*mIter)) {
            mWhitespaceBeforeFirstToken = true;
            ++mIter;
        }
    }

    


    bool hasMoreTokens() const
    {
        MOZ_ASSERT(mIter == mEnd || !IsWhitespace(*mIter),
                   "Should be at beginning of token if there is one");

        return mIter < mEnd;
    }

    


    bool whitespaceBeforeFirstToken() const
    {
        return mWhitespaceBeforeFirstToken;
    }

    




    bool separatorAfterCurrentToken() const
    {
        return mSeparatorAfterCurrentToken;
    }

    


    bool whitespaceAfterCurrentToken() const
    {
        return mWhitespaceAfterCurrentToken;
    }

    


    const nsDependentCSubstring nextToken()
    {
        mozilla::RangedPtr<const char> tokenStart = mIter, tokenEnd = mIter;

        MOZ_ASSERT(mIter == mEnd || !IsWhitespace(*mIter),
                   "Should be at beginning of token if there is one");

        
        
        while (mIter < mEnd && *mIter != mSeparatorChar) {
          
          while (mIter < mEnd &&
                 !IsWhitespace(*mIter) && *mIter != mSeparatorChar) {
              ++mIter;
          }
          tokenEnd = mIter;

          
          mWhitespaceAfterCurrentToken = false;
          while (mIter < mEnd && IsWhitespace(*mIter)) {
              mWhitespaceAfterCurrentToken = true;
              ++mIter;
          }
          if (mSeparatorOptional) {
            
            
            break;
          } 
        }

        mSeparatorAfterCurrentToken = (mIter != mEnd &&
                                       *mIter == mSeparatorChar);
        MOZ_ASSERT(mSeparatorOptional ||
                   (mSeparatorAfterCurrentToken == (mIter < mEnd)),
                   "If we require a separator and haven't hit the end of "
                   "our string, then we shouldn't have left the loop "
                   "unless we hit a separator");

        
        if (mSeparatorAfterCurrentToken) {
            ++mIter;

            while (mIter < mEnd && IsWhitespace(*mIter)) {
                mWhitespaceAfterCurrentToken = true;
                ++mIter;
            }
        }

        return Substring(tokenStart.get(), tokenEnd.get());
    }

private:
    mozilla::RangedPtr<const char> mIter;
    const mozilla::RangedPtr<const char> mEnd;
    char mSeparatorChar;
    bool mWhitespaceBeforeFirstToken;
    bool mWhitespaceAfterCurrentToken;
    bool mSeparatorAfterCurrentToken;
    bool mSeparatorOptional;
};

class nsCCharSeparatedTokenizer: public nsCCharSeparatedTokenizerTemplate<>
{
public:
    nsCCharSeparatedTokenizer(const nsCSubstring& aSource,
                              char aSeparatorChar,
                              uint32_t aFlags = 0)
      : nsCCharSeparatedTokenizerTemplate<>(aSource, aSeparatorChar, aFlags)
    {
    }
};

#endif
