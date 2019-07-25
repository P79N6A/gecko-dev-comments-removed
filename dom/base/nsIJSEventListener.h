




































#ifndef nsIJSEventListener_h__
#define nsIJSEventListener_h__

#include "nsIScriptContext.h"
#include "jsapi.h"
#include "nsIDOMEventListener.h"

class nsIScriptObjectOwner;
class nsIAtom;

#define NS_IJSEVENTLISTENER_IID \
{ 0x2135cf56, 0x5954, 0x40fa, \
  { 0x80, 0xb8, 0xd7, 0xd8, 0xa9, 0x22, 0xa2, 0x8a } }





class nsIJSEventListener : public nsIDOMEventListener
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IJSEVENTLISTENER_IID)

  nsIJSEventListener(nsIScriptContext *aContext, void *aScopeObject,
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

  void *GetEventScope() const
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
  void *mScopeObject;
  nsCOMPtr<nsISupports> mTarget;
  JSObject *mHandler;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIJSEventListener, NS_IJSEVENTLISTENER_IID)


nsresult NS_NewJSEventListener(nsIScriptContext *aContext,
                               void *aScopeObject, nsISupports *aTarget,
                               nsIAtom* aType, JSObject* aHandler,
                               nsIDOMEventListener **aReturn);

#endif 
