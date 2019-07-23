




































#ifndef nsIScriptContext_h__
#define nsIScriptContext_h__

#include "nscore.h"
#include "nsStringGlue.h"
#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsIProgrammingLanguage.h"

class nsIScriptGlobalObject;
class nsIScriptSecurityManager;
class nsIPrincipal;
class nsIAtom;
class nsIArray;
class nsIVariant;
class nsIObjectInputStream;
class nsIObjectOutputStream;
class nsScriptObjectHolder;

typedef void (*nsScriptTerminationFunc)(nsISupports* aRef);

#define NS_ISCRIPTCONTEXT_IID \
{ /* {e7b9871d-3adc-4bf7-850d-7fb9554886bf} */ \
  0xe7b9871d, 0x3adc, 0x4bf7, \
 { 0x85, 0x0d, 0x7f, 0xb9, 0x55, 0x48, 0x86, 0xbf } }



#define SCRIPTVERSION_DEFAULT JSVERSION_DEFAULT








class nsIScriptContext : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTCONTEXT_IID)

  
  virtual PRUint32 GetScriptTypeID() = 0;

  


















  virtual nsresult EvaluateString(const nsAString& aScript,
                                  void *aScopeObject,
                                  nsIPrincipal *aPrincipal,
                                  const char *aURL,
                                  PRUint32 aLineNo,
                                  PRUint32 aVersion,
                                  nsAString *aRetValue,
                                  PRBool* aIsUndefined) = 0;

  
  
  virtual nsresult EvaluateStringWithValue(const nsAString& aScript,
                                           void *aScopeObject,
                                           nsIPrincipal *aPrincipal,
                                           const char *aURL,
                                           PRUint32 aLineNo,
                                           PRUint32 aVersion,
                                           void* aRetValue,
                                           PRBool* aIsUndefined) = 0;

  
















  virtual nsresult CompileScript(const PRUnichar* aText,
                                 PRInt32 aTextLength,
                                 void* aScopeObject,
                                 nsIPrincipal* aPrincipal,
                                 const char* aURL,
                                 PRUint32 aLineNo,
                                 PRUint32 aVersion,
                                 nsScriptObjectHolder &aScriptObject) = 0;

  













  virtual nsresult ExecuteScript(void* aScriptObject,
                                 void* aScopeObject,
                                 nsAString* aRetValue,
                                 PRBool* aIsUndefined) = 0;

  


























  virtual nsresult CompileEventHandler(nsIAtom* aName,
                                       PRUint32 aArgCount,
                                       const char** aArgNames,
                                       const nsAString& aBody,
                                       const char* aURL,
                                       PRUint32 aLineNo,
                                       PRUint32 aVersion,
                                       nsScriptObjectHolder &aHandler) = 0;

  











  virtual nsresult CallEventHandler(nsISupports* aTarget,
                                    void *aScope, void* aHandler,
                                    nsIArray *argv, nsIVariant **rval) = 0;

  






















  virtual nsresult BindCompiledEventHandler(nsISupports* aTarget, void *aScope,
                                            nsIAtom* aName,
                                            void* aHandler) = 0;

  





  virtual nsresult GetBoundEventHandler(nsISupports* aTarget, void *aScope,
                                        nsIAtom* aName,
                                        nsScriptObjectHolder &aHandler) = 0;

  






  virtual nsresult CompileFunction(void* aTarget,
                                   const nsACString& aName,
                                   PRUint32 aArgCount,
                                   const char** aArgArray,
                                   const nsAString& aBody,
                                   const char* aURL,
                                   PRUint32 aLineNo,
                                   PRUint32 aVersion,
                                   PRBool aShared,
                                   void **aFunctionObject) = 0;

  




  virtual void SetDefaultLanguageVersion(PRUint32 aVersion) = 0;

  



  virtual nsIScriptGlobalObject *GetGlobalObject() = 0;

  



  virtual void *GetNativeContext() = 0;

  



  virtual void *GetNativeGlobal() = 0;

  




  virtual nsresult CreateNativeGlobalForInner(
                                      nsIScriptGlobalObject *aNewInner,
                                      PRBool aIsChrome,
                                      void **aNativeGlobal,
                                      nsISupports **aHolder) = 0;

  




  virtual nsresult ConnectToInner(nsIScriptGlobalObject *aNewInner,
                                  void *aOuterGlobal) = 0;


  










  virtual nsresult InitContext(nsIScriptGlobalObject *aGlobalObject) = 0;

  






  virtual PRBool IsContextInitialized() = 0;

  


  virtual void FinalizeContext() = 0;

  





  virtual void GC() = 0;

  











  virtual void ScriptEvaluated(PRBool aTerminated) = 0;

  virtual nsresult Serialize(nsIObjectOutputStream* aStream,
                             void *aScriptObject) = 0;
  
  

  virtual nsresult Deserialize(nsIObjectInputStream* aStream,
                               nsScriptObjectHolder &aResult) = 0;

  









  virtual nsresult SetTerminationFunction(nsScriptTerminationFunc aFunc,
                                          nsISupports* aRef) = 0;

  


  virtual PRBool GetScriptsEnabled() = 0;
  virtual void SetScriptsEnabled(PRBool aEnabled, PRBool aFireTimeouts) = 0;

  
  
  virtual nsresult SetProperty(void *aTarget, const char *aPropName, nsISupports *aVal) = 0;
  



  virtual PRBool GetProcessingScriptTag() = 0;
  virtual void SetProcessingScriptTag(PRBool aResult) = 0;

  



  virtual void SetGCOnDestruction(PRBool aGCOnDestruction) = 0;

  





  virtual nsresult InitClasses(void *aGlobalObj) = 0;

  











  virtual void ClearScope(void* aGlobalObj, PRBool aClearFromProtoChain) = 0;

  


  virtual void WillInitializeContext() = 0;

  


  virtual void DidInitializeContext() = 0;

  




  virtual void DidSetDocument(nsISupports *aDoc, void *aGlobal) = 0;

  






  virtual nsresult DropScriptObject(void *object) = 0;
  virtual nsresult HoldScriptObject(void *object) = 0;

  
  virtual void ReportPendingException() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptContext, NS_ISCRIPTCONTEXT_IID)

#endif 

