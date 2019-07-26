






#ifndef nsDOMCSSRect_h_
#define nsDOMCSSRect_h_

#include "mozilla/Attributes.h"
#include "nsISupportsImpl.h"
#include "nsIDOMRect.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"

class nsROCSSPrimitiveValue;

class nsDOMCSSRect : public nsIDOMRect,
                     public nsWrapperCache
{
public:
  nsDOMCSSRect(nsROCSSPrimitiveValue* aTop,
               nsROCSSPrimitiveValue* aRight,
               nsROCSSPrimitiveValue* aBottom,
               nsROCSSPrimitiveValue* aLeft);
  virtual ~nsDOMCSSRect(void);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIDOMRECT

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsDOMCSSRect)

  nsROCSSPrimitiveValue* Top() const { return mTop; }
  nsROCSSPrimitiveValue* Right() const { return mRight; }
  nsROCSSPrimitiveValue* Bottom() const { return mBottom; }
  nsROCSSPrimitiveValue* Left() const { return mLeft; }

  nsISupports* GetParentObject() const { return nullptr; }

  virtual JSObject* WrapObject(JSContext* cx, JSObject* scope, bool* tried)
    MOZ_OVERRIDE MOZ_FINAL;

private:
  nsRefPtr<nsROCSSPrimitiveValue> mTop;
  nsRefPtr<nsROCSSPrimitiveValue> mRight;
  nsRefPtr<nsROCSSPrimitiveValue> mBottom;
  nsRefPtr<nsROCSSPrimitiveValue> mLeft;
};

#endif 
