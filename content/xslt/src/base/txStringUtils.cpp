






































#include "txStringUtils.h"
#include "nsDependentString.h"

int
txCaseInsensitiveStringComparator::operator()(const char_type* lhs,
                                              const char_type* rhs,
                                              PRUint32 aLength ) const
{
  PRUnichar thisChar, otherChar;
  PRUint32 compLoop = 0;
  while (compLoop < aLength) {
    thisChar = lhs[compLoop];
    if ((thisChar >= 'A') && (thisChar <= 'Z')) {
      thisChar += 32;
    }
    otherChar = rhs[compLoop];
    if ((otherChar >= 'A') && (otherChar <= 'Z')) {
      otherChar += 32;
    }
    if (thisChar != otherChar) {
      return thisChar - otherChar;
    }
    ++compLoop;
  }
  return 0;

}

int
txCaseInsensitiveStringComparator::operator()(char_type lhs,
                                              char_type rhs) const
{
  if (lhs >= 'A' && lhs <= 'Z') {
    lhs += 32;
  }
  if (rhs >= 'A' && rhs <= 'Z') {
    rhs += 32;
  }
  return lhs - rhs;
} 




class ConvertToLowerCase
{
public:
  typedef PRUnichar value_type;

  PRUint32 write( const PRUnichar* aSource, PRUint32 aSourceLength)
  {
    PRUnichar* cp = const_cast<PRUnichar*>(aSource);
    const PRUnichar* end = aSource + aSourceLength;
    while (cp != end) {
      PRUnichar ch = *cp;
      if ((ch >= 'A') && (ch <= 'Z'))
        *cp = ch + ('a' - 'A');
      ++cp;
    }
    return aSourceLength;
  }
};

void TX_ToLowerCase(nsAString& aString)
{
  nsAString::iterator fromBegin, fromEnd;
  ConvertToLowerCase converter;
  copy_string(aString.BeginWriting(fromBegin), aString.EndWriting(fromEnd),
              converter);
}




class CopyToLowerCase
{
public:
  typedef PRUnichar value_type;

  CopyToLowerCase(nsAString::iterator& aDestIter) : mIter(aDestIter)
  {
  }

  PRUint32 write(const PRUnichar* aSource, PRUint32 aSourceLength)
  {
    PRUint32 len = PR_MIN(PRUint32(mIter.size_forward()), aSourceLength);
    PRUnichar* cp = mIter.get();
    const PRUnichar* end = aSource + len;
    while (aSource != end) {
      PRUnichar ch = *aSource;
      if ((ch >= 'A') && (ch <= 'Z'))
        *cp = ch + ('a' - 'A');
      else
        *cp = ch;
      ++aSource;
      ++cp;
    }
    mIter.advance(len);
    return len;
  }

protected:
  nsAString::iterator& mIter;
};

void TX_ToLowerCase(const nsAString& aSource, nsAString& aDest)
{
  nsAString::const_iterator fromBegin, fromEnd;
  nsAString::iterator toBegin;
  if (!EnsureStringLength(aDest, aSource.Length()))
    return; 
  CopyToLowerCase converter(aDest.BeginWriting(toBegin));
  copy_string(aSource.BeginReading(fromBegin), aSource.EndReading(fromEnd),
              converter);
}
