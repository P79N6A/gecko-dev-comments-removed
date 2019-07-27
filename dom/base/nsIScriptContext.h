




#ifndef nsIScriptContext_h__
#define nsIScriptContext_h__

#include "nscore.h"
#include "nsStringGlue.h"
#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "jspubtd.h"
#include "js/GCAPI.h"

class nsIScriptGlobalObject;

#define NS_ISCRIPTCONTEXT_IID \
{ 0x901f0d5e, 0x217a, 0x45fa, \
  { 0x9a, 0xca, 0x45, 0x0f, 0xe7, 0x2f, 0x10, 0x9a } }

class nsIOffThreadScriptReceiver;





class nsIScriptContext : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTCONTEXT_IID)

  



  virtual nsIScriptGlobalObject *GetGlobalObject() = 0;

  



  virtual JSContext* GetNativeContext() = 0;

  


  virtual nsresult InitContext() = 0;

  






  virtual bool IsContextInitialized() = 0;

  
  
  virtual nsresult SetProperty(JS::Handle<JSObject*> aTarget,
                               const char* aPropName, nsISupports* aVal) = 0;
  



  virtual bool GetProcessingScriptTag() = 0;
  virtual void SetProcessingScriptTag(bool aResult) = 0;

  





  virtual nsresult InitClasses(JS::Handle<JSObject*> aGlobalObj) = 0;

  


  virtual void WillInitializeContext() = 0;

  


  virtual void DidInitializeContext() = 0;

  


  virtual void SetWindowProxy(JS::Handle<JSObject*> aWindowProxy) = 0;
  virtual JSObject* GetWindowProxy() = 0;
  virtual JSObject* GetWindowProxyPreserveColor() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptContext, NS_ISCRIPTCONTEXT_IID)

#define NS_IOFFTHREADSCRIPTRECEIVER_IID \
{0x3a980010, 0x878d, 0x46a9,            \
  {0x93, 0xad, 0xbc, 0xfd, 0xd3, 0x8e, 0xa0, 0xc2}}

class nsIOffThreadScriptReceiver : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IOFFTHREADSCRIPTRECEIVER_IID)

  




  NS_IMETHOD OnScriptCompileComplete(JSScript* aScript, nsresult aStatus) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIOffThreadScriptReceiver, NS_IOFFTHREADSCRIPTRECEIVER_IID)

#endif 
