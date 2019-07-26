






#ifndef nsDOMCSSRect_h_
#define nsDOMCSSRect_h_

#include "nsISupports.h"
#include "nsIDOMRect.h"
#include "nsCOMPtr.h"

class nsROCSSPrimitiveValue;

class nsDOMCSSRect : public nsIDOMRect {
public:
  nsDOMCSSRect(nsROCSSPrimitiveValue* aTop,
               nsROCSSPrimitiveValue* aRight,
               nsROCSSPrimitiveValue* aBottom,
               nsROCSSPrimitiveValue* aLeft);
  virtual ~nsDOMCSSRect(void);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMRECT

private:
  nsRefPtr<nsROCSSPrimitiveValue> mTop;
  nsRefPtr<nsROCSSPrimitiveValue> mRight;
  nsRefPtr<nsROCSSPrimitiveValue> mBottom;
  nsRefPtr<nsROCSSPrimitiveValue> mLeft;
};

#endif 
