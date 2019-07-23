




































#ifndef nsIJSEventListener_h__
#define nsIJSEventListener_h__

#include "nsIScriptContext.h"

class nsIScriptObjectOwner;
class nsIDOMEventListener;
class nsIAtom;

#define NS_IJSEVENTLISTENER_IID     \
{ 0xa6cf9118, 0x15b3, 0x11d2,       \
{0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32} }





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

  virtual void SetEventName(nsIAtom* aName) = 0;

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
                               nsIDOMEventListener **aReturn);

#endif 
