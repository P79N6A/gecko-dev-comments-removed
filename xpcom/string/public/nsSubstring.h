





































#ifndef nsSubstring_h___
#define nsSubstring_h___

#ifndef nsAString_h___
#include "nsAString.h"
#endif

#define kNotFound -1



#ifndef NS_DISABLE_LITERAL_TEMPLATE
#  if (defined(_MSC_VER) && (_MSC_VER < 1310)) || (defined(__SUNPRO_CC) && (__SUNPRO_CC < 0x560)) || (defined(__HP_aCC) && (__HP_aCC <= 012100))
#    define NS_DISABLE_LITERAL_TEMPLATE
#  endif
#endif 

#include <string.h>

  
#include "string-template-def-unichar.h"
#include "nsTSubstring.h"
#include "string-template-undef.h"


  
#include "string-template-def-char.h"
#include "nsTSubstring.h"
#include "string-template-undef.h"


#ifndef nsSubstringTuple_h___
#include "nsSubstringTuple.h"
#endif

#endif 
