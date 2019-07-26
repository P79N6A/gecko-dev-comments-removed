





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
nsCaseInsensitiveCStringComparator::operator()(const char_type* aLhs,
                                               const char_type* aRhs,
                                               uint32_t aLhsLength,
                                               uint32_t aRhsLength) const
{
  if (aLhsLength != aRhsLength) {
    return (aLhsLength > aRhsLength) ? 1 : -1;
  }
  int32_t result = int32_t(PL_strncasecmp(aLhs, aRhs, aLhsLength));
  
  
  if (result < 0) {
    result = -1;
  }
  return result;
}
