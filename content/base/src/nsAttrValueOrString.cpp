




#include "nsAttrValueOrString.h"

const nsAString&
nsAttrValueOrString::String() const
{
  if (mStringPtr) {
    return *mStringPtr;
  }

  if (mAttrValue->Type() == nsAttrValue::eString) {
    mCheapString = mAttrValue->GetStringValue();
    mStringPtr = &mCheapString;
    return *mStringPtr;
  }

  mAttrValue->ToString(mCheapString);
  mStringPtr = &mCheapString;
  return *mStringPtr;
}
