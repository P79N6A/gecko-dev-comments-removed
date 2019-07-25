





































#include <ctype.h>
#include "nsAString.h"
#include "plstr.h"


  
#include "string-template-def-unichar.h"
#include "nsTStringComparator.cpp"
#include "string-template-undef.h"

  
#include "string-template-def-char.h"
#include "nsTStringComparator.cpp"
#include "string-template-undef.h"


int
nsCaseInsensitiveCStringComparator::operator()( const char_type* lhs,
                                                const char_type* rhs,
                                                PRUint32 lLength,
                                                PRUint32 rLength ) const
  {
    if (lLength != rLength)
      return (lLength > rLength) ? 1 : -1;
    PRInt32 result=PRInt32(PL_strncasecmp(lhs, rhs, lLength));
    
    
    if (result<0) 
      result=-1;
    return result;
  }
