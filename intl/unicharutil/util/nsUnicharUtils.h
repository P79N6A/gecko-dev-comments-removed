




#ifndef nsUnicharUtils_h__
#define nsUnicharUtils_h__

#include "nsStringGlue.h"



#define IS_CJ_CHAR(u) \
  ((0x2e80u <= (u) && (u) <= 0x312fu) || \
   (0x3190u <= (u) && (u) <= 0xabffu) || \
   (0xf900u <= (u) && (u) <= 0xfaffu) || \
   (0xff00u <= (u) && (u) <= 0xffefu) )

void ToLowerCase(nsAString&);
void ToUpperCase(nsAString&);

void ToLowerCase(const nsAString& aSource, nsAString& aDest);
void ToUpperCase(const nsAString& aSource, nsAString& aDest);

PRUint32 ToLowerCase(PRUint32);
PRUint32 ToUpperCase(PRUint32);
PRUint32 ToTitleCase(PRUint32);

void ToLowerCase(const PRUnichar*, PRUnichar*, PRUint32);
void ToUpperCase(const PRUnichar*, PRUnichar*, PRUint32);

inline bool IsUpperCase(PRUint32 c) {
  return ToLowerCase(c) != c;
}

inline bool IsLowerCase(PRUint32 c) {
  return ToUpperCase(c) != c;
}

#ifdef MOZILLA_INTERNAL_API

class nsCaseInsensitiveStringComparator : public nsStringComparator
{
public:
  virtual PRInt32 operator() (const PRUnichar*,
                              const PRUnichar*,
                              PRUint32,
                              PRUint32) const;
};

class nsCaseInsensitiveUTF8StringComparator : public nsCStringComparator
{
public:
  virtual PRInt32 operator() (const char*,
                              const char*,
                              PRUint32,
                              PRUint32) const;
};

class nsCaseInsensitiveStringArrayComparator
{
public:
  template<class A, class B>
  bool Equals(const A& a, const B& b) const {
    return a.Equals(b, nsCaseInsensitiveStringComparator());
  }
};

class nsASCIICaseInsensitiveStringComparator : public nsStringComparator
{
public:
  nsASCIICaseInsensitiveStringComparator() {}
  virtual int operator() (const PRUnichar*,
                          const PRUnichar*,
                          PRUint32,
                          PRUint32) const;
};

inline bool
CaseInsensitiveFindInReadable(const nsAString& aPattern,
                              nsAString::const_iterator& aSearchStart,
                              nsAString::const_iterator& aSearchEnd)
{
  return FindInReadable(aPattern, aSearchStart, aSearchEnd,
                        nsCaseInsensitiveStringComparator());
}

inline bool
CaseInsensitiveFindInReadable(const nsAString& aPattern,
                              const nsAString& aHay)
{
  nsAString::const_iterator searchBegin, searchEnd;
  return FindInReadable(aPattern, aHay.BeginReading(searchBegin),
                        aHay.EndReading(searchEnd),
                        nsCaseInsensitiveStringComparator());
}

#endif 

PRInt32
CaseInsensitiveCompare(const PRUnichar *a, const PRUnichar *b, PRUint32 len);

PRInt32
CaseInsensitiveCompare(const char* aLeft, const char* aRight,
                       PRUint32 aLeftBytes, PRUint32 aRightBytes);




















bool
CaseInsensitiveUTF8CharsEqual(const char* aLeft, const char* aRight,
                              const char* aLeftEnd, const char* aRightEnd,
                              const char** aLeftNext, const char** aRightNext,
                              bool* aErr);

namespace mozilla {









PRUint32
HashUTF8AsUTF16(const char* aUTF8, PRUint32 aLength, bool* aErr);

} 

#endif  
