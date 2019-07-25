






































#ifndef nsDOMString_h___
#define nsDOMString_h___

#include "nsStringGlue.h"

inline bool DOMStringIsNull(const nsAString& aString)
{
  return aString.IsVoid();
}

inline void SetDOMStringToNull(nsAString& aString)
{
  aString.SetIsVoid(true);
}

#endif 
