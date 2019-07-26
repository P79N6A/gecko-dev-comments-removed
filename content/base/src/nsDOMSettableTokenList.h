







#ifndef nsDOMSettableTokenList_h___
#define nsDOMSettableTokenList_h___

#include "nsDOMTokenList.h"

class nsIAtom;



class nsDOMSettableTokenList MOZ_FINAL : public nsDOMTokenList
{
public:

  nsDOMSettableTokenList(mozilla::dom::Element* aElement, nsIAtom* aAttrAtom)
    : nsDOMTokenList(aElement, aAttrAtom) {}

  virtual JSObject* WrapObject(JSContext *cx, JSObject *scope) MOZ_OVERRIDE;

  
  void GetValue(nsAString& aResult) { Stringify(aResult); }
  void SetValue(const nsAString& aValue, mozilla::ErrorResult& rv);
};

#endif 

