






































#ifndef nsDOMCSSRect_h_
#define nsDOMCSSRect_h_

#include "nsISupports.h"
#include "nsIDOMRect.h"
#include "nsCOMPtr.h"
class nsIDOMCSSPrimitiveValue;

class nsDOMCSSRect : public nsIDOMRect {
public:
  nsDOMCSSRect(nsIDOMCSSPrimitiveValue* aTop,
               nsIDOMCSSPrimitiveValue* aRight,
               nsIDOMCSSPrimitiveValue* aBottom,
               nsIDOMCSSPrimitiveValue* aLeft);
  virtual ~nsDOMCSSRect(void);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMRECT

private:
  nsCOMPtr<nsIDOMCSSPrimitiveValue> mTop;
  nsCOMPtr<nsIDOMCSSPrimitiveValue> mRight;
  nsCOMPtr<nsIDOMCSSPrimitiveValue> mBottom;
  nsCOMPtr<nsIDOMCSSPrimitiveValue> mLeft;
};

#endif 
