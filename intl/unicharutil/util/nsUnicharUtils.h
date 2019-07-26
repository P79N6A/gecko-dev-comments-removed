




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

uint32_t ToLowerCase(uint32_t);
uint32_t ToUpperCase(uint32_t);
uint32_t ToTitleCase(uint32_t);

void ToLowerCase(const char16_t*, char16_t*, uint32_t);
void ToUpperCase(const char16_t*, char16_t*, uint32_t);

inline bool IsUpperCase(uint32_t c) {
  return ToLowerCase(c) != c;
}

inline bool IsLowerCase(uint32_t c) {
  return ToUpperCase(c) != c;
}

#ifdef MOZILLA_INTERNAL_API

class nsCaseInsensitiveStringComparator : public nsStringComparator
{
public:
  virtual int32_t operator() (const char16_t*,
                              const char16_t*,
                              uint32_t,
                              uint32_t) const;
};

class nsCaseInsensitiveUTF8StringComparator : public nsCStringComparator
{
public:
  virtual int32_t operator() (const char*,
                              const char*,
                              uint32_t,
                              uint32_t) const;
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
  virtual int operator() (const char16_t*,
                          const char16_t*,
                          uint32_t,
                          uint32_t) const;
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

int32_t
CaseInsensitiveCompare(const char16_t *a, const char16_t *b, uint32_t len);

int32_t
CaseInsensitiveCompare(const char* aLeft, const char* aRight,
                       uint32_t aLeftBytes, uint32_t aRightBytes);




















bool
CaseInsensitiveUTF8CharsEqual(const char* aLeft, const char* aRight,
                              const char* aLeftEnd, const char* aRightEnd,
                              const char** aLeftNext, const char** aRightNext,
                              bool* aErr);

namespace mozilla {









uint32_t
HashUTF8AsUTF16(const char* aUTF8, uint32_t aLength, bool* aErr);

} 

#endif  
