






#ifndef nsAString_h___
#define nsAString_h___

#include "nsStringFwd.h"
#include "nsStringIterator.h"

#include <string.h>
#include <stdarg.h>

#include "mozilla/fallible.h"

#define kNotFound -1


#include "string-template-def-unichar.h"
#include "nsTSubstring.h"
#include "string-template-undef.h"


#include "string-template-def-char.h"
#include "nsTSubstring.h"
#include "string-template-undef.h"






class nsCaseInsensitiveCStringComparator
  : public nsCStringComparator
{
public:
  nsCaseInsensitiveCStringComparator()
  {
  }
  typedef char char_type;

  virtual int operator()(const char_type*, const char_type*,
                         uint32_t, uint32_t) const;
};

class nsCaseInsensitiveCStringArrayComparator
{
public:
  template<class A, class B>
  bool Equals(const A& aStrA, const B& aStrB) const
  {
    return aStrA.Equals(aStrB, nsCaseInsensitiveCStringComparator());
  }
};


#ifndef nsSubstringTuple_h___
#include "nsSubstringTuple.h"
#endif

#endif 
