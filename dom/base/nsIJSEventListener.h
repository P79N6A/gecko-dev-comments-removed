




































#ifndef nsIJSEventListener_h__
#define nsIJSEventListener_h__

#include "nsIScriptContext.h"

class nsIScriptObjectOwner;
class nsIDOMEventListener;
class nsIAtom;

#define NS_IJSEVENTLISTENER_IID     \
{ 0xe16e7146, 0x109d, 0x4f54, \
  { 0x94, 0x78, 0xda, 0xc4, 0x3a, 0x71, 0x0b, 0x52 } }





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

  virtual void ToString(const nsAString& aEventName, nsAString& aResult) = 0;

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
