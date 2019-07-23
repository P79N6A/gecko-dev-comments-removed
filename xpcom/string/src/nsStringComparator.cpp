





































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
nsCaseInsensitiveCStringComparator::operator()( const char_type* lhs, const char_type* rhs, PRUint32 aLength ) const
  {
    PRInt32 result=PRInt32(PL_strncasecmp(lhs, rhs, aLength));
    
    
    if (result<0) 
      result=-1;
    return result;
  }

int
nsCaseInsensitiveCStringComparator::operator()( char lhs, char rhs ) const
  {
    if (lhs == rhs) return 0;
    
    lhs = tolower(lhs);
    rhs = tolower(rhs);

    return lhs - rhs;
  }
