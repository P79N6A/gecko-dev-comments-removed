






































#ifndef nsDOMString_h___
#define nsDOMString_h___

#include "nsAString.h"

inline PRBool DOMStringIsNull(const nsAString& aString)
{
  return aString.IsVoid();
}

inline void SetDOMStringToNull(nsAString& aString)
{
  aString.SetIsVoid(PR_TRUE);
}

#endif 
