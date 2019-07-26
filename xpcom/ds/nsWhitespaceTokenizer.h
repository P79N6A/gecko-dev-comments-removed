




#ifndef __nsWhitespaceTokenizer_h
#define __nsWhitespaceTokenizer_h

#include "mozilla/RangedPtr.h"
#include "nsDependentSubstring.h"
#include "nsCRT.h"

template<bool IsWhitespace(PRUnichar) = NS_IsAsciiWhitespace>
class nsWhitespaceTokenizerTemplate
{
public:
    nsWhitespaceTokenizerTemplate(const nsSubstring& aSource)
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

    


    const nsDependentSubstring nextToken()
    {
        const mozilla::RangedPtr<const PRUnichar> tokenStart = mIter;
        while (mIter < mEnd && !IsWhitespace(*mIter)) {
            ++mIter;
        }
        const mozilla::RangedPtr<const PRUnichar> tokenEnd = mIter;
        mWhitespaceAfterCurrentToken = false;
        while (mIter < mEnd && IsWhitespace(*mIter)) {
            mWhitespaceAfterCurrentToken = true;
            ++mIter;
        }
        return Substring(tokenStart.get(), tokenEnd.get());
    }

private:
    mozilla::RangedPtr<const PRUnichar> mIter;
    const mozilla::RangedPtr<const PRUnichar> mEnd;
    bool mWhitespaceBeforeFirstToken;
    bool mWhitespaceAfterCurrentToken;
};

class nsWhitespaceTokenizer: public nsWhitespaceTokenizerTemplate<>
{
public:
    nsWhitespaceTokenizer(const nsSubstring& aSource)
      : nsWhitespaceTokenizerTemplate<>(aSource)
    {
    }
};

template<bool IsWhitespace(PRUnichar) = NS_IsAsciiWhitespace>
class nsCWhitespaceTokenizerTemplate
{
public:
    nsCWhitespaceTokenizerTemplate(const nsCSubstring& aSource)
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

    


    const nsDependentCSubstring nextToken()
    {
        const mozilla::RangedPtr<const char> tokenStart = mIter;
        while (mIter < mEnd && !IsWhitespace(*mIter)) {
            ++mIter;
        }
        const mozilla::RangedPtr<const char> tokenEnd = mIter;
        mWhitespaceAfterCurrentToken = false;
        while (mIter < mEnd && IsWhitespace(*mIter)) {
            mWhitespaceAfterCurrentToken = true;
            ++mIter;
        }
        return Substring(tokenStart.get(), tokenEnd.get());
    }

private:
    mozilla::RangedPtr<const char> mIter;
    const mozilla::RangedPtr<const char> mEnd;
    bool mWhitespaceBeforeFirstToken;
    bool mWhitespaceAfterCurrentToken;
};

class nsCWhitespaceTokenizer: public nsCWhitespaceTokenizerTemplate<>
{
public:
    nsCWhitespaceTokenizer(const nsCSubstring& aSource)
      : nsCWhitespaceTokenizerTemplate<>(aSource)
    {
    }
};

#endif
