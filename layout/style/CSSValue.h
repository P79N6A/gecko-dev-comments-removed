







#ifndef mozilla_dom_CSSValue_h_
#define mozilla_dom_CSSValue_h_

#include "nsWrapperCache.h"
#include "nsStringFwd.h"

class nsROCSSPrimitiveValue;
namespace mozilla {
class ErrorResult;
}

namespace mozilla {
namespace dom {




class CSSValue : public nsISupports,
                 public nsWrapperCache
{
public:
  
  virtual void GetCssText(nsString& aText, mozilla::ErrorResult& aRv) = 0;
  virtual void SetCssText(const nsAString& aText, mozilla::ErrorResult& aRv) = 0;
  virtual uint16_t CssValueType() const = 0;

  

  



  nsROCSSPrimitiveValue *AsPrimitiveValue();
};

}
}

#endif
