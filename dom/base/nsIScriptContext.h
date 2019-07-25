




































#ifndef nsIScriptContext_h__
#define nsIScriptContext_h__

#include "nscore.h"
#include "nsStringGlue.h"
#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsIProgrammingLanguage.h"
#include "jsfriendapi.h"
#include "jspubtd.h"

class nsIScriptGlobalObject;
class nsIScriptSecurityManager;
class nsIPrincipal;
class nsIAtom;
class nsIArray;
class nsIVariant;
class nsIObjectInputStream;
class nsIObjectOutputStream;
template<class> class nsScriptObjectHolder;
class nsIScriptObjectPrincipal;
class nsIDOMWindow;

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
{ 0xdfaea249, 0xaaad, 0x48bd, \
  { 0xb8, 0x04, 0x92, 0xad, 0x30, 0x88, 0xd0, 0xc6 } }



#define SCRIPTVERSION_DEFAULT JSVERSION_DEFAULT





class nsIScriptContext : public nsIScriptContextPrincipal
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTCONTEXT_IID)

  
  virtual PRUint32 GetScriptTypeID() = 0;

  virtual void SetGlobalObject(nsIScriptGlobalObject* aGlobalObject) = 0;

  




















  virtual nsresult EvaluateString(const nsAString& aScript,
                                  JSObject* aScopeObject,
                                  nsIPrincipal *aPrincipal,
                                  nsIPrincipal *aOriginPrincipal,
                                  const char *aURL,
                                  PRUint32 aLineNo,
                                  PRUint32 aVersion,
                                  nsAString *aRetValue,
                                  bool* aIsUndefined) = 0;

  virtual nsresult EvaluateStringWithValue(const nsAString& aScript,
                                           JSObject* aScopeObject,
                                           nsIPrincipal *aPrincipal,
                                           const char *aURL,
                                           PRUint32 aLineNo,
                                           PRUint32 aVersion,
                                           JS::Value* aRetValue,
                                           bool* aIsUndefined) = 0;

  














  virtual nsresult CompileScript(const PRUnichar* aText,
                                 PRInt32 aTextLength,
                                 nsIPrincipal* aPrincipal,
                                 const char* aURL,
                                 PRUint32 aLineNo,
                                 PRUint32 aVersion,
                                 nsScriptObjectHolder<JSScript>& aScriptObject) = 0;

  













  virtual nsresult ExecuteScript(JSScript* aScriptObject,
                                 JSObject* aScopeObject,
                                 nsAString* aRetValue,
                                 bool* aIsUndefined) = 0;

  


























  virtual nsresult CompileEventHandler(nsIAtom* aName,
                                       PRUint32 aArgCount,
                                       const char** aArgNames,
                                       const nsAString& aBody,
                                       const char* aURL,
                                       PRUint32 aLineNo,
                                       PRUint32 aVersion,
                                       nsScriptObjectHolder<JSObject>& aHandler) = 0;

  











  virtual nsresult CallEventHandler(nsISupports* aTarget,
                                    JSObject* aScope, JSObject* aHandler,
                                    nsIArray *argv, nsIVariant **rval) = 0;

  




















  virtual nsresult BindCompiledEventHandler(nsISupports* aTarget,
                                            JSObject* aScope,
                                            JSObject* aHandler,
                                            nsScriptObjectHolder<JSObject>& aBoundHandler) = 0;

  






  virtual nsresult CompileFunction(JSObject* aTarget,
                                   const nsACString& aName,
                                   PRUint32 aArgCount,
                                   const char** aArgArray,
                                   const nsAString& aBody,
                                   const char* aURL,
                                   PRUint32 aLineNo,
                                   PRUint32 aVersion,
                                   bool aShared,
                                   JSObject** aFunctionObject) = 0;

  



  virtual nsIScriptGlobalObject *GetGlobalObject() = 0;

  



  virtual JSContext* GetNativeContext() = 0;

  



  virtual JSObject* GetNativeGlobal() = 0;

  




  virtual nsresult CreateNativeGlobalForInner(
                                      nsIScriptGlobalObject *aNewInner,
                                      bool aIsChrome,
                                      nsIPrincipal *aPrincipal,
                                      JSObject** aNativeGlobal,
                                      nsISupports **aHolder) = 0;

  




  virtual nsresult ConnectToInner(nsIScriptGlobalObject *aNewInner,
                                  JSObject *aOuterGlobal) = 0;


  


  virtual nsresult InitContext() = 0;

  



  virtual nsresult InitOuterWindow() = 0;

  






  virtual bool IsContextInitialized() = 0;

  





  virtual void GC(js::gcreason::Reason aReason) = 0;

  











  virtual void ScriptEvaluated(bool aTerminated) = 0;

  virtual nsresult Serialize(nsIObjectOutputStream* aStream,
                             JSScript* aScriptObject) = 0;
  
  

  virtual nsresult Deserialize(nsIObjectInputStream* aStream,
                               nsScriptObjectHolder<JSScript>& aResult) = 0;

  









  virtual void SetTerminationFunction(nsScriptTerminationFunc aFunc,
                                      nsIDOMWindow* aRef) = 0;

  


  virtual bool GetScriptsEnabled() = 0;
  virtual void SetScriptsEnabled(bool aEnabled, bool aFireTimeouts) = 0;

  
  
  virtual nsresult SetProperty(JSObject* aTarget, const char* aPropName, nsISupports* aVal) = 0;
  



  virtual bool GetProcessingScriptTag() = 0;
  virtual void SetProcessingScriptTag(bool aResult) = 0;

  


  virtual bool GetExecutingScript() = 0;

  



  virtual void SetGCOnDestruction(bool aGCOnDestruction) = 0;

  





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

