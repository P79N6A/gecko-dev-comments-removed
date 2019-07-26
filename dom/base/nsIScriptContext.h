




#ifndef nsIScriptContext_h__
#define nsIScriptContext_h__

#include "nscore.h"
#include "nsStringGlue.h"
#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsIProgrammingLanguage.h"
#include "jspubtd.h"
#include "js/GCAPI.h"

class nsIScriptGlobalObject;
class nsIScriptSecurityManager;
class nsIPrincipal;
class nsIAtom;
class nsIArray;
class nsIVariant;
class nsIObjectInputStream;
class nsIObjectOutputStream;
class nsIScriptObjectPrincipal;
class nsIDOMWindow;
class nsIURI;

#define NS_ISCRIPTCONTEXT_IID \
{ 0xf3859ce7, 0x7551, 0x4760, \
  { 0x84, 0x29, 0x64, 0x4f, 0x26, 0x1e, 0xdb, 0x91 } }



#define SCRIPTVERSION_DEFAULT JSVERSION_DEFAULT

class nsIOffThreadScriptReceiver;





class nsIScriptContext : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTCONTEXT_IID)

  















  virtual nsresult EvaluateString(const nsAString& aScript,
                                  JS::Handle<JSObject*> aScopeObject,
                                  JS::CompileOptions& aOptions,
                                  bool aCoerceToString,
                                  JS::Value* aRetValue,
                                  void **aOffThreadToken = nullptr) = 0;

  




















  virtual nsresult BindCompiledEventHandler(nsISupports* aTarget,
                                            JS::Handle<JSObject*> aScope,
                                            JS::Handle<JSObject*> aHandler,
                                            JS::MutableHandle<JSObject*> aBoundHandler) = 0;

  



  virtual nsIScriptGlobalObject *GetGlobalObject() = 0;

  



  virtual JSContext* GetNativeContext() = 0;

  


  virtual nsresult InitContext() = 0;

  






  virtual bool IsContextInitialized() = 0;

  





  virtual void GC(JS::gcreason::Reason aReason) = 0;

  


  virtual bool GetScriptsEnabled() = 0;
  virtual void SetScriptsEnabled(bool aEnabled, bool aFireTimeouts) = 0;

  
  
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

