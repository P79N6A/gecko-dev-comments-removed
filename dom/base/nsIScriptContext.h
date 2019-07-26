




#ifndef nsIScriptContext_h__
#define nsIScriptContext_h__

#include "nscore.h"
#include "nsStringGlue.h"
#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsIProgrammingLanguage.h"
#include "jsfriendapi.h"
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
{ 0x76d14525, 0x68ff, 0x4f43, \
  { 0xb1, 0x92, 0x9e, 0x67, 0x28, 0xaa, 0xaf, 0x19 } }



#define SCRIPTVERSION_DEFAULT JSVERSION_DEFAULT





class nsIScriptContext : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTCONTEXT_IID)

  













  virtual nsresult EvaluateString(const nsAString& aScript,
                                  JS::Handle<JSObject*> aScopeObject,
                                  JS::CompileOptions& aOptions,
                                  bool aCoerceToString,
                                  JS::Value* aRetValue) = 0;

  















  virtual nsresult CompileScript(const PRUnichar* aText,
                                 int32_t aTextLength,
                                 nsIPrincipal* aPrincipal,
                                 const char* aURL,
                                 uint32_t aLineNo,
                                 uint32_t aVersion,
                                 JS::MutableHandle<JSScript*> aScriptObject,
                                 bool aSaveSource = false) = 0;

  













  virtual nsresult ExecuteScript(JSScript* aScriptObject,
                                 JSObject* aScopeObject) = 0;

  




















  virtual nsresult BindCompiledEventHandler(nsISupports* aTarget,
                                            JS::Handle<JSObject*> aScope,
                                            JS::Handle<JSObject*> aHandler,
                                            JS::MutableHandle<JSObject*> aBoundHandler) = 0;

  



  virtual nsIScriptGlobalObject *GetGlobalObject() = 0;

  



  virtual JSContext* GetNativeContext() = 0;

  



  virtual JSObject* GetNativeGlobal() = 0;

  


  virtual nsresult InitContext() = 0;

  






  virtual bool IsContextInitialized() = 0;

  





  virtual void GC(JS::gcreason::Reason aReason) = 0;

  










  virtual void ScriptEvaluated(bool aTerminated) = 0;

  virtual nsresult Serialize(nsIObjectOutputStream* aStream,
                             JS::Handle<JSScript*> aScriptObject) = 0;
  
  

  virtual nsresult Deserialize(nsIObjectInputStream* aStream,
                               JS::MutableHandle<JSScript*> aResult) = 0;

  


  virtual bool GetScriptsEnabled() = 0;
  virtual void SetScriptsEnabled(bool aEnabled, bool aFireTimeouts) = 0;

  
  
  virtual nsresult SetProperty(JS::Handle<JSObject*> aTarget,
                               const char* aPropName, nsISupports* aVal) = 0;
  



  virtual bool GetProcessingScriptTag() = 0;
  virtual void SetProcessingScriptTag(bool aResult) = 0;

  





  virtual nsresult InitClasses(JS::Handle<JSObject*> aGlobalObj) = 0;

  


  virtual void WillInitializeContext() = 0;

  


  virtual void DidInitializeContext() = 0;

  virtual void EnterModalState() = 0;
  virtual void LeaveModalState() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptContext, NS_ISCRIPTCONTEXT_IID)

#endif 

