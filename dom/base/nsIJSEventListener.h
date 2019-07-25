




#ifndef nsIJSEventListener_h__
#define nsIJSEventListener_h__

#include "nsIScriptContext.h"
#include "jsapi.h"
#include "xpcpublic.h"
#include "nsIDOMEventListener.h"

class nsIAtom;

#define NS_IJSEVENTLISTENER_IID \
{ 0x92f9212b, 0xa6aa, 0x4867, \
  { 0x93, 0x8a, 0x56, 0xbe, 0x17, 0x67, 0x4f, 0xd4 } }








class nsIJSEventListener : public nsIDOMEventListener
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IJSEVENTLISTENER_IID)

  nsIJSEventListener(nsIScriptContext* aContext, JSObject* aScopeObject,
                     nsISupports *aTarget, JSObject *aHandler)
    : mContext(aContext), mScopeObject(aScopeObject), mHandler(aHandler)
  {
    nsCOMPtr<nsISupports> base = do_QueryInterface(aTarget);
    mTarget = base.get();
  }

  nsIScriptContext *GetEventContext() const
  {
    return mContext;
  }

  nsISupports *GetEventTarget() const
  {
    return mTarget;
  }

  void Disconnect()
  {
    mTarget = nullptr;
  }

  JSObject* GetEventScope() const
  {
    return xpc_UnmarkGrayObject(mScopeObject);
  }

  JSObject *GetHandler() const
  {
    return xpc_UnmarkGrayObject(mHandler);
  }

  
  
  
  virtual void SetHandler(JSObject *aHandler) = 0;

  
  
  
  
  
  virtual size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const
  {
    return 0;

    
    
    
    
    
    
    
    
  }

  virtual size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

protected:
  virtual ~nsIJSEventListener()
  {
    NS_ASSERTION(!mTarget, "Should have called Disconnect()!");
  }
  nsCOMPtr<nsIScriptContext> mContext;
  JSObject* mScopeObject;
  nsISupports* mTarget;
  JSObject *mHandler;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIJSEventListener, NS_IJSEVENTLISTENER_IID)


nsresult NS_NewJSEventListener(nsIScriptContext *aContext,
                               JSObject* aScopeObject, nsISupports* aTarget,
                               nsIAtom* aType, JSObject* aHandler,
                               nsIJSEventListener **aReturn);

#endif 
