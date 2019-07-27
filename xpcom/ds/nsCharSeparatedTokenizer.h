





#ifndef __nsCharSeparatedTokenizer_h
#define __nsCharSeparatedTokenizer_h

#include "mozilla/RangedPtr.h"

#include "nsDependentSubstring.h"
#include "nsCRT.h"



















template<typename DependentSubstringType, bool IsWhitespace(char16_t)>
class nsTCharSeparatedTokenizer
{
  typedef typename DependentSubstringType::char_type CharType;
  typedef typename DependentSubstringType::substring_type SubstringType;

public:
  
  
  enum
  {
    SEPARATOR_OPTIONAL = 1
  };

  nsTCharSeparatedTokenizer(const SubstringType& aSource,
                            CharType aSeparatorChar,
                            uint32_t aFlags = 0)
    : mIter(aSource.Data(), aSource.Length())
    , mEnd(aSource.Data() + aSource.Length(), aSource.Data(),
           aSource.Length())
    , mSeparatorChar(aSeparatorChar)
    , mWhitespaceBeforeFirstToken(false)
    , mWhitespaceAfterCurrentToken(false)
    , mSeparatorAfterCurrentToken(false)
    , mSeparatorOptional(aFlags & SEPARATOR_OPTIONAL)
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

  


  const DependentSubstringType nextToken()
  {
    mozilla::RangedPtr<const CharType> tokenStart = mIter;
    mozilla::RangedPtr<const CharType> tokenEnd = mIter;

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
  mozilla::RangedPtr<const CharType> mIter;
  const mozilla::RangedPtr<const CharType> mEnd;
  CharType mSeparatorChar;
  bool mWhitespaceBeforeFirstToken;
  bool mWhitespaceAfterCurrentToken;
  bool mSeparatorAfterCurrentToken;
  bool mSeparatorOptional;
};

template<bool IsWhitespace(char16_t) = NS_IsAsciiWhitespace>
class nsCharSeparatedTokenizerTemplate
  : public nsTCharSeparatedTokenizer<nsDependentSubstring, IsWhitespace>
{
public:
  nsCharSeparatedTokenizerTemplate(const nsSubstring& aSource,
                                   char16_t aSeparatorChar,
                                   uint32_t aFlags = 0)
    : nsTCharSeparatedTokenizer<nsDependentSubstring,
                                IsWhitespace>(aSource, aSeparatorChar, aFlags)
  {
  }
};

typedef nsCharSeparatedTokenizerTemplate<> nsCharSeparatedTokenizer;

template<bool IsWhitespace(char16_t) = NS_IsAsciiWhitespace>
class nsCCharSeparatedTokenizerTemplate
  : public nsTCharSeparatedTokenizer<nsDependentCSubstring, IsWhitespace>
{
public:
  nsCCharSeparatedTokenizerTemplate(const nsCSubstring& aSource,
                                    char aSeparatorChar,
                                    uint32_t aFlags = 0)
    : nsTCharSeparatedTokenizer<nsDependentCSubstring,
                                IsWhitespace>(aSource, aSeparatorChar, aFlags)
  {
  }
};

typedef nsCCharSeparatedTokenizerTemplate<> nsCCharSeparatedTokenizer;

#endif 
