





































#ifndef mozilla_CharTokenizer_h
#define mozilla_CharTokenizer_h

#include "nsDependentSubstring.h"

namespace mozilla {

template<PRUnichar delimiter>
class CharTokenizer
{
public:
    CharTokenizer(const nsSubstring& aSource)
    {
      aSource.BeginReading(mIter);
      aSource.EndReading(mEnd);
    }

    


    bool hasMoreTokens()
    {
      return mIter != mEnd;
    }

    


    const nsDependentSubstring nextToken()
    {
      nsSubstring::const_char_iterator begin = mIter;
      while (mIter != mEnd && (*mIter) != delimiter) {
        ++mIter;
      }
      nsSubstring::const_char_iterator end = mIter;
      if (mIter != mEnd) {
        ++mIter;
      }

      return Substring(begin, end);
    }

private:
    nsSubstring::const_char_iterator mIter, mEnd;
};

template<char delimiter>
class CCharTokenizer
{
public:
    CCharTokenizer(const nsCSubstring& aSource)
    {
      aSource.BeginReading(mIter);
      aSource.EndReading(mEnd);
    }

    


    bool hasMoreTokens()
    {
      return mIter != mEnd;
    }

    


    const nsDependentCSubstring nextToken()
    {
      nsCSubstring::const_char_iterator begin = mIter;
      while (mIter != mEnd && (*mIter) != delimiter) {
        ++mIter;
      }
      nsCSubstring::const_char_iterator end = mIter;
      if (mIter != mEnd) {
        ++mIter;
      }

      return Substring(begin, end);
    }

private:
    nsCSubstring::const_char_iterator mIter, mEnd;
};

} 

#endif 
