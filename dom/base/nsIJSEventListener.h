




































#ifndef nsIJSEventListener_h__
#define nsIJSEventListener_h__

#include "nsIScriptContext.h"
#include "jsapi.h"
#include "nsIDOMEventListener.h"

class nsIScriptObjectOwner;
class nsIAtom;

#define NS_IJSEVENTLISTENER_IID     \
{ 0x468406d2, 0xf6aa, 0x404f, \
  { 0x92, 0xa1, 0x53, 0xd1, 0x5f, 0x6e, 0x5e, 0x19 } }





class nsIJSEventListener : public nsIDOMEventListener
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IJSEVENTLISTENER_IID)

  nsIJSEventListener(nsIScriptContext *aContext, void *aScopeObject,
                     nsISupports *aTarget, void *aHandler)
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

  void *GetHandler() const
  {
    return mHandler;
  }

  
  
  
  virtual void SetHandler(void *aHandler) = 0;

  virtual PRInt64 SizeOf() const = 0;
protected:
  virtual ~nsIJSEventListener()
  {
  }
  nsCOMPtr<nsIScriptContext> mContext;
  void *mScopeObject;
  nsCOMPtr<nsISupports> mTarget;
  void *mHandler;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIJSEventListener, NS_IJSEVENTLISTENER_IID)


nsresult NS_NewJSEventListener(nsIScriptContext *aContext,
                               void *aScopeObject, nsISupports *aTarget,
                               nsIAtom* aType, void *aHandler,
                               nsIDOMEventListener **aReturn);

#endif 
