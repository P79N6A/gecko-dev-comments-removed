




































#ifndef nsIJSEventListener_h__
#define nsIJSEventListener_h__

#include "nsIScriptContext.h"
#include "jsapi.h"
#include "nsIDOMEventListener.h"

class nsIScriptObjectOwner;
class nsIAtom;

#define NS_IJSEVENTLISTENER_IID \
{ 0xafc5d047, 0xdb6b, 0x4076, \
  { 0xb3, 0xfa, 0x57, 0x96, 0x1e, 0x21, 0x48, 0x42 } }





class nsIJSEventListener : public nsIDOMEventListener
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IJSEVENTLISTENER_IID)

  nsIJSEventListener(nsIScriptContext* aContext, JSObject* aScopeObject,
                     nsISupports *aTarget, JSObject *aHandler)
    : mContext(aContext), mScopeObject(aScopeObject),
      mTarget(do_QueryInterface(aTarget)), mHandler(aHandler)
  {
  }

  nsIScriptContext *GetEventContext() const
  {
    return mContext;
  }

  nsISupports *GetEventTarget() const
  {
    return mTarget;
  }

  JSObject* GetEventScope() const
  {
    return mScopeObject;
  }

  JSObject *GetHandler() const
  {
    return mHandler;
  }

  
  
  
  virtual void SetHandler(JSObject *aHandler) = 0;

  virtual PRInt64 SizeOf() const = 0;
protected:
  virtual ~nsIJSEventListener()
  {
  }
  nsCOMPtr<nsIScriptContext> mContext;
  JSObject* mScopeObject;
  nsCOMPtr<nsISupports> mTarget;
  JSObject *mHandler;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIJSEventListener, NS_IJSEVENTLISTENER_IID)


nsresult NS_NewJSEventListener(nsIScriptContext *aContext,
                               JSObject* aScopeObject, nsISupports* aTarget,
                               nsIAtom* aType, JSObject* aHandler,
                               nsIDOMEventListener **aReturn);

#endif 
