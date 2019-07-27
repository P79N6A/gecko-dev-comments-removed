





#ifndef __nsWhitespaceTokenizer_h
#define __nsWhitespaceTokenizer_h

#include "mozilla/RangedPtr.h"
#include "nsDependentSubstring.h"
#include "nsCRT.h"

template<typename DependentSubstringType, bool IsWhitespace(char16_t)>
class nsTWhitespaceTokenizer
{
  typedef typename DependentSubstringType::char_type CharType;
  typedef typename DependentSubstringType::substring_type SubstringType;

public:
  explicit nsTWhitespaceTokenizer(const SubstringType& aSource)
    : mIter(aSource.Data(), aSource.Length())
    , mEnd(aSource.Data() + aSource.Length(), aSource.Data(),
           aSource.Length())
    , mWhitespaceBeforeFirstToken(false)
    , mWhitespaceAfterCurrentToken(false)
  {
    while (mIter < mEnd && IsWhitespace(*mIter)) {
      mWhitespaceBeforeFirstToken = true;
      ++mIter;
    }
  }

  


  bool hasMoreTokens() const
  {
    return mIter < mEnd;
  }

  


  bool whitespaceBeforeFirstToken() const
  {
    return mWhitespaceBeforeFirstToken;
  }

  



  bool whitespaceAfterCurrentToken() const
  {
    return mWhitespaceAfterCurrentToken;
  }

  


  const DependentSubstringType nextToken()
  {
    const mozilla::RangedPtr<const CharType> tokenStart = mIter;
    while (mIter < mEnd && !IsWhitespace(*mIter)) {
      ++mIter;
    }
    const mozilla::RangedPtr<const CharType> tokenEnd = mIter;
    mWhitespaceAfterCurrentToken = false;
    while (mIter < mEnd && IsWhitespace(*mIter)) {
      mWhitespaceAfterCurrentToken = true;
      ++mIter;
    }
    return Substring(tokenStart.get(), tokenEnd.get());
  }

private:
  mozilla::RangedPtr<const CharType> mIter;
  const mozilla::RangedPtr<const CharType> mEnd;
  bool mWhitespaceBeforeFirstToken;
  bool mWhitespaceAfterCurrentToken;
};

template<bool IsWhitespace(char16_t) = NS_IsAsciiWhitespace>
class nsWhitespaceTokenizerTemplate
  : public nsTWhitespaceTokenizer<nsDependentSubstring, IsWhitespace>
{
public:
  explicit nsWhitespaceTokenizerTemplate(const nsSubstring& aSource)
    : nsTWhitespaceTokenizer<nsDependentSubstring, IsWhitespace>(aSource)
  {
  }
};

typedef nsWhitespaceTokenizerTemplate<> nsWhitespaceTokenizer;

template<bool IsWhitespace(char16_t) = NS_IsAsciiWhitespace>
class nsCWhitespaceTokenizerTemplate
  : public nsTWhitespaceTokenizer<nsDependentCSubstring, IsWhitespace>
{
public:
  explicit nsCWhitespaceTokenizerTemplate(const nsCSubstring& aSource)
    : nsTWhitespaceTokenizer<nsDependentCSubstring, IsWhitespace>(aSource)
  {
  }
};

typedef nsCWhitespaceTokenizerTemplate<> nsCWhitespaceTokenizer;

#endif 
