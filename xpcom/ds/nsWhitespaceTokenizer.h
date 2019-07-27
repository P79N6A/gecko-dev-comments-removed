




#ifndef __nsWhitespaceTokenizer_h
#define __nsWhitespaceTokenizer_h

#include "mozilla/RangedPtr.h"
#include "nsDependentSubstring.h"
#include "nsCRT.h"

template<typename SubstringType,
         typename DependentSubstringType,
         bool IsWhitespace(char16_t)>
class nsTWhitespaceTokenizer
{
  typedef typename SubstringType::char_type CharType;

public:
    nsTWhitespaceTokenizer(const SubstringType& aSource)
        : mIter(aSource.Data(), aSource.Length()),
          mEnd(aSource.Data() + aSource.Length(), aSource.Data(),
               aSource.Length()),
          mWhitespaceBeforeFirstToken(false),
          mWhitespaceAfterCurrentToken(false)
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
  : public nsTWhitespaceTokenizer<nsSubstring, nsDependentSubstring,
                                  IsWhitespace>
{
public:
  nsWhitespaceTokenizerTemplate(const nsSubstring& aSource)
    : nsTWhitespaceTokenizer<nsSubstring, nsDependentSubstring,
                             IsWhitespace>(aSource)
  {
  }
};

class nsWhitespaceTokenizer
  : public nsWhitespaceTokenizerTemplate<>
{
public:
  nsWhitespaceTokenizer(const nsSubstring& aSource)
    : nsWhitespaceTokenizerTemplate<>(aSource)
  {
  }
};

template<bool IsWhitespace(char16_t) = NS_IsAsciiWhitespace>
class nsCWhitespaceTokenizerTemplate
  : public nsTWhitespaceTokenizer<nsCSubstring, nsDependentCSubstring,
                                  IsWhitespace>
{
public:
  nsCWhitespaceTokenizerTemplate(const nsCSubstring& aSource)
    : nsTWhitespaceTokenizer<nsCSubstring, nsDependentCSubstring,
                             IsWhitespace>(aSource)
  {
  }
};

class nsCWhitespaceTokenizer
  : public nsCWhitespaceTokenizerTemplate<>
{
public:
  nsCWhitespaceTokenizer(const nsCSubstring& aSource)
    : nsCWhitespaceTokenizerTemplate<>(aSource)
  {
  }
};

#endif
