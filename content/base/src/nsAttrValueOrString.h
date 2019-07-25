













#ifndef nsAttrValueOrString_h___
#define nsAttrValueOrString_h___

#include "nsString.h"
#include "nsAttrValue.h"

class NS_STACK_CLASS nsAttrValueOrString
{
public:
  nsAttrValueOrString(const nsAString& aValue)
    : mAttrValue(nsnull)
    , mStringPtr(&aValue)
    , mCheapString(nsnull)
  { }
  nsAttrValueOrString(const nsAttrValue& aValue)
    : mAttrValue(&aValue)
    , mStringPtr(nsnull)
    , mCheapString(nsnull)
  { }

  






  const nsAString& String() const;

  



  bool EqualsAsStrings(const nsAttrValue& aOther) const
  {
    if (mStringPtr) {
      return aOther.Equals(*mStringPtr, eCaseMatters);
    }
    return aOther.EqualsAsStrings(*mAttrValue);
  }

protected:
  const nsAttrValue*       mAttrValue;
  mutable const nsAString* mStringPtr;
  mutable nsCheapString    mCheapString;
};

#endif 
