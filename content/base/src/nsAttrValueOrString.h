













#ifndef nsAttrValueOrString_h___
#define nsAttrValueOrString_h___

#include "nsString.h"
#include "nsAttrValue.h"

class MOZ_STACK_CLASS nsAttrValueOrString
{
public:
  nsAttrValueOrString(const nsAString& aValue)
    : mAttrValue(nullptr)
    , mStringPtr(&aValue)
    , mCheapString(nullptr)
  { }
  nsAttrValueOrString(const nsAttrValue& aValue)
    : mAttrValue(&aValue)
    , mStringPtr(nullptr)
    , mCheapString(nullptr)
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
