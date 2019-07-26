




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

typedef void (*nsScriptTerminationFunc)(nsISupports* aRef);

#define NS_ISCRIPTCONTEXTPRINCIPAL_IID \
  { 0xd012cdb3, 0x8f1e, 0x4440, \
    { 0x8c, 0xbd, 0x32, 0x7f, 0x98, 0x1d, 0x37, 0xb4 } }

class nsIScriptContextPrincipal : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTCONTEXTPRINCIPAL_IID)

  virtual nsIScriptObjectPrincipal* GetObjectPrincipal() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptContextPrincipal,
                              NS_ISCRIPTCONTEXTPRINCIPAL_IID)

#define NS_ISCRIPTCONTEXT_IID \
{ 0xf5af1c3c, 0xebad, 0x4d00, \
  { 0xa2, 0xa4, 0x12, 0x2e, 0x27, 0x16, 0x59, 0x01 } }



#define SCRIPTVERSION_DEFAULT JSVERSION_DEFAULT





class nsIScriptContext : public nsIScriptContextPrincipal
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTCONTEXT_IID)

  













  virtual nsresult EvaluateString(const nsAString& aScript,
                                  JSObject& aScopeObject,
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

  











  virtual nsresult CallEventHandler(nsISupports* aTarget,
                                    JSObject* aScope, JSObject* aHandler,
                                    nsIArray *argv, nsIVariant **rval) = 0;

  




















  virtual nsresult BindCompiledEventHandler(nsISupports* aTarget,
                                            JSObject* aScope,
                                            JSObject* aHandler,
                                            JS::MutableHandle<JSObject*> aBoundHandler) = 0;

  



  virtual nsIScriptGlobalObject *GetGlobalObject() = 0;

  



  virtual JSContext* GetNativeContext() = 0;

  



  virtual JSObject* GetNativeGlobal() = 0;

  


  virtual nsresult InitContext() = 0;

  






  virtual bool IsContextInitialized() = 0;

  





  virtual void GC(JS::gcreason::Reason aReason) = 0;

  











  virtual void ScriptEvaluated(bool aTerminated) = 0;

  virtual nsresult Serialize(nsIObjectOutputStream* aStream,
                             JSScript* aScriptObject) = 0;
  
  

  virtual nsresult Deserialize(nsIObjectInputStream* aStream,
                               JS::MutableHandle<JSScript*> aResult) = 0;

  









  virtual void SetTerminationFunction(nsScriptTerminationFunc aFunc,
                                      nsIDOMWindow* aRef) = 0;

  


  virtual bool GetScriptsEnabled() = 0;
  virtual void SetScriptsEnabled(bool aEnabled, bool aFireTimeouts) = 0;

  
  
  virtual nsresult SetProperty(JSObject* aTarget, const char* aPropName, nsISupports* aVal) = 0;
  



  virtual bool GetProcessingScriptTag() = 0;
  virtual void SetProcessingScriptTag(bool aResult) = 0;

  


  virtual bool GetExecutingScript() = 0;

  





  virtual nsresult InitClasses(JSObject* aGlobalObj) = 0;

  


  virtual void WillInitializeContext() = 0;

  


  virtual void DidInitializeContext() = 0;

  





  virtual nsresult DropScriptObject(void *object) = 0;
  virtual nsresult HoldScriptObject(void *object) = 0;

  virtual void EnterModalState() = 0;
  virtual void LeaveModalState() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptContext, NS_ISCRIPTCONTEXT_IID)

#endif 

