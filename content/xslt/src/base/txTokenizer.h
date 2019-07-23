





































#ifndef txTokenizer_h___
#define txTokenizer_h___

#include "nsDependentSubstring.h"
#include "txXMLUtils.h"

class txTokenizer
{
public:
    


    txTokenizer(const nsSubstring& aSource)
    {
        aSource.BeginReading(mIter);
        aSource.EndReading(mEnd);

        while (mIter != mEnd && XMLUtils::isWhitespace(*mIter)) {
            ++mIter;
        }
    }

    


    PRBool hasMoreTokens()
    {
        return (mIter != mEnd);
    }

    


    const nsDependentSubstring nextToken()
    {
        nsAFlatString::const_char_iterator begin = mIter;
        while (mIter != mEnd && !XMLUtils::isWhitespace(*mIter)) {
            ++mIter;
        }
        nsAFlatString::const_char_iterator end = mIter;
        while (mIter != mEnd && XMLUtils::isWhitespace(*mIter)) {
            ++mIter;
        }
        return Substring(begin, end);
    }

private:
    nsSubstring::const_char_iterator mIter, mEnd;
};

#endif 

