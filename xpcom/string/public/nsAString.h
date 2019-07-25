





































#ifndef nsAString_h___
#define nsAString_h___

#ifndef nsStringFwd_h___
#include "nsStringFwd.h"
#endif

#ifndef nsStringIterator_h___
#include "nsStringIterator.h"
#endif



#ifndef NS_DISABLE_LITERAL_TEMPLATE
#  if (defined(_MSC_VER) && (_MSC_VER < 1310)) || (defined(__SUNPRO_CC) && (__SUNPRO_CC < 0x560)) || (defined(__HP_aCC) && (__HP_aCC <= 012100))
#    define NS_DISABLE_LITERAL_TEMPLATE
#  endif
#endif 

#include <string.h>

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
      nsCaseInsensitiveCStringComparator() {}
      typedef char char_type;

      virtual int operator()( const char_type*, const char_type*, PRUint32, PRUint32 ) const;
  };

class nsCaseInsensitiveCStringArrayComparator
  {
    public:
      template<class A, class B>
      bool Equals(const A& a, const B& b) const {
        return a.Equals(b, nsCaseInsensitiveCStringComparator());
      }
  };

  
#ifndef nsSubstringTuple_h___
#include "nsSubstringTuple.h"
#endif

#endif 
