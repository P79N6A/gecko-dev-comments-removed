




































#ifndef nsIJSEventListener_h__
#define nsIJSEventListener_h__

#include "nsIScriptContext.h"
#include "jsapi.h"

class nsIScriptObjectOwner;
class nsIDOMEventListener;
class nsIAtom;

#define NS_IJSEVENTLISTENER_IID     \
{ 0x08ca15c4, 0x1c2d, 0x449e, \
  { 0x9e, 0x88, 0xaa, 0x8b, 0xbf, 0x00, 0xf7, 0x63 } }





class nsIJSEventListener : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IJSEVENTLISTENER_IID)

  nsIJSEventListener(nsIScriptContext *aContext, void *aScopeObject,
                     nsISupports *aTarget)
    : mContext(aContext), mScopeObject(aScopeObject),
      mTarget(do_QueryInterface(aTarget))
  {
  }

  nsIScriptContext *GetEventContext()
  {
    return mContext;
  }

  nsISupports *GetEventTarget()
  {
    return mTarget;
  }

  void *GetEventScope()
  {
    return mScopeObject;
  }

  virtual nsresult GetJSVal(const nsAString& aEventName, jsval* aJSVal) = 0;

protected:
  virtual ~nsIJSEventListener()
  {
  }
  nsCOMPtr<nsIScriptContext> mContext;
  void *mScopeObject;
  nsCOMPtr<nsISupports> mTarget;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIJSEventListener, NS_IJSEVENTLISTENER_IID)


nsresult NS_NewJSEventListener(nsIScriptContext *aContext,
                               void *aScopeObject, nsISupports *aObject,
                               nsIAtom* aType, nsIDOMEventListener **aReturn);

#endif 
