



































#ifndef nsJSEnvironment_h___
#define nsJSEnvironment_h___

#include "nsIScriptContext.h"
#include "nsIScriptRuntime.h"
#include "nsCOMPtr.h"
#include "jsapi.h"
#include "nsIObserver.h"
#include "nsIXPCScriptNotify.h"
#include "prtime.h"
#include "nsCycleCollectionParticipant.h"
#include "nsScriptNameSpaceManager.h"

class nsIXPConnectJSObjectHolder;

class nsJSContext : public nsIScriptContext,
                    public nsIXPCScriptNotify
{
public:
  nsJSContext(JSRuntime *aRuntime);
  virtual ~nsJSContext();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsJSContext,
                                                         nsIScriptContext)

  virtual PRUint32 GetScriptTypeID()
    { return nsIProgrammingLanguage::JAVASCRIPT; }

  virtual nsresult EvaluateString(const nsAString& aScript,
                                  void *aScopeObject,
                                  nsIPrincipal *principal,
                                  const char *aURL,
                                  PRUint32 aLineNo,
                                  PRUint32 aVersion,
                                  nsAString *aRetValue,
                                  PRBool* aIsUndefined);
  virtual nsresult EvaluateStringWithValue(const nsAString& aScript,
                                     void *aScopeObject,
                                     nsIPrincipal *aPrincipal,
                                     const char *aURL,
                                     PRUint32 aLineNo,
                                     PRUint32 aVersion,
                                     void* aRetValue,
                                     PRBool* aIsUndefined);

  virtual nsresult CompileScript(const PRUnichar* aText,
                                 PRInt32 aTextLength,
                                 void *aScopeObject,
                                 nsIPrincipal *principal,
                                 const char *aURL,
                                 PRUint32 aLineNo,
                                 PRUint32 aVersion,
                                 nsScriptObjectHolder &aScriptObject);
  virtual nsresult ExecuteScript(void* aScriptObject,
                                 void *aScopeObject,
                                 nsAString* aRetValue,
                                 PRBool* aIsUndefined);

  virtual nsresult CompileEventHandler(nsIAtom *aName,
                                       PRUint32 aArgCount,
                                       const char** aArgNames,
                                       const nsAString& aBody,
                                       const char *aURL, PRUint32 aLineNo,
                                       PRUint32 aVersion,
                                       nsScriptObjectHolder &aHandler);
  virtual nsresult CallEventHandler(nsISupports* aTarget, void *aScope,
                                    void* aHandler,
                                    nsIArray *argv, nsIVariant **rv);
  virtual nsresult BindCompiledEventHandler(nsISupports *aTarget,
                                            void *aScope,
                                            nsIAtom *aName,
                                            void *aHandler);
  virtual nsresult GetBoundEventHandler(nsISupports* aTarget, void *aScope,
                                        nsIAtom* aName,
                                        nsScriptObjectHolder &aHandler);
  virtual nsresult CompileFunction(void* aTarget,
                                   const nsACString& aName,
                                   PRUint32 aArgCount,
                                   const char** aArgArray,
                                   const nsAString& aBody,
                                   const char* aURL,
                                   PRUint32 aLineNo,
                                   PRUint32 aVersion,
                                   PRBool aShared,
                                   void** aFunctionObject);

  virtual void SetDefaultLanguageVersion(PRUint32 aVersion);
  virtual nsIScriptGlobalObject *GetGlobalObject();
  virtual void *GetNativeContext();
  virtual void *GetNativeGlobal();
  virtual nsresult CreateNativeGlobalForInner(
                                      nsIScriptGlobalObject *aGlobal,
                                      PRBool aIsChrome,
                                      void **aNativeGlobal,
                                      nsISupports **aHolder);
  virtual nsresult ConnectToInner(nsIScriptGlobalObject *aNewInner,
                                  void *aOuterGlobal);
  virtual nsresult InitContext(nsIScriptGlobalObject *aGlobalObject);
  virtual PRBool IsContextInitialized();
  virtual void FinalizeContext();

  virtual void GC();

  virtual void ScriptEvaluated(PRBool aTerminated);
  virtual nsresult SetTerminationFunction(nsScriptTerminationFunc aFunc,
                                          nsISupports* aRef);
  virtual PRBool GetScriptsEnabled();
  virtual void SetScriptsEnabled(PRBool aEnabled, PRBool aFireTimeouts);

  virtual nsresult SetProperty(void *aTarget, const char *aPropName, nsISupports *aVal);

  virtual PRBool GetProcessingScriptTag();
  virtual void SetProcessingScriptTag(PRBool aResult);

  virtual PRBool GetExecutingScript();

  virtual void SetGCOnDestruction(PRBool aGCOnDestruction);

  virtual nsresult InitClasses(void *aGlobalObj);
  virtual void ClearScope(void* aGlobalObj, PRBool bClearPolluters);

  virtual void WillInitializeContext();
  virtual void DidInitializeContext();
  virtual void DidSetDocument(nsISupports *aDocdoc, void *aGlobal) {;}

  virtual nsresult Serialize(nsIObjectOutputStream* aStream, void *aScriptObject);
  virtual nsresult Deserialize(nsIObjectInputStream* aStream,
                               nsScriptObjectHolder &aResult);

  virtual nsresult DropScriptObject(void *object);
  virtual nsresult HoldScriptObject(void *object);

  virtual void EnterModalState();
  virtual void LeaveModalState();

  NS_DECL_NSIXPCSCRIPTNOTIFY

  static void LoadStart();
  static void LoadEnd();

  
  
  static void CC();

  
  
  
  
  
  
  
  
  
  
  
  
  
  static PRBool MaybeCC(PRBool aHigherProbability);

  
  
  static PRBool IntervalCC();

  
  static void CCIfUserInactive();

  static void FireGCTimer(PRBool aLoadInProgress);

protected:
  nsresult InitializeExternalClasses();
  
  nsresult FindXPCNativeWrapperClass(nsIXPConnectJSObjectHolder *aHolder);

  
  JS_FORCES_STACK nsresult ConvertSupportsTojsvals(nsISupports *aArgs,
                                                   void *aScope,
                                                   PRUint32 *aArgc,
                                                   void **aArgv,
                                                   void **aMarkp);

  nsresult AddSupportsPrimitiveTojsvals(nsISupports *aArg, jsval *aArgv);

  
  
  nsresult JSObjectFromInterface(nsISupports *aSup, void *aScript, 
                                 JSObject **aRet);

  
  
  
  void ReportPendingException();
private:
  void DestroyJSContext();

  nsrefcnt GetCCRefcnt();

  JSContext *mContext;
  PRUint32 mNumEvaluations;

protected:
  struct TerminationFuncHolder;
  friend struct TerminationFuncHolder;
  
  struct TerminationFuncClosure
  {
    TerminationFuncClosure(nsScriptTerminationFunc aFunc,
                           nsISupports* aArg,
                           TerminationFuncClosure* aNext) :
      mTerminationFunc(aFunc),
      mTerminationFuncArg(aArg),
      mNext(aNext)
    {
    }
    ~TerminationFuncClosure()
    {
      delete mNext;
    }
    
    nsScriptTerminationFunc mTerminationFunc;
    nsCOMPtr<nsISupports> mTerminationFuncArg;
    TerminationFuncClosure* mNext;
  };

  struct TerminationFuncHolder
  {
    TerminationFuncHolder(nsJSContext* aContext)
      : mContext(aContext),
        mTerminations(aContext->mTerminations)
    {
      aContext->mTerminations = nsnull;
    }
    ~TerminationFuncHolder()
    {
      
      
      
      
      if (mTerminations) {
        TerminationFuncClosure* cur = mTerminations;
        while (cur->mNext) {
          cur = cur->mNext;
        }
        cur->mNext = mContext->mTerminations;
        mContext->mTerminations = mTerminations;
      }
    }

    nsJSContext* mContext;
    TerminationFuncClosure* mTerminations;
  };
  
  TerminationFuncClosure* mTerminations;

private:
  PRPackedBool mIsInitialized;
  PRPackedBool mScriptsEnabled;
  PRPackedBool mGCOnDestruction;
  PRPackedBool mProcessingScriptTag;

  PRUint32 mExecuteDepth;
  PRUint32 mDefaultJSOptions;
  PRTime mOperationCallbackTime;

  PRTime mModalStateTime;
  PRUint32 mModalStateDepth;

  
  
  
  

  nsCOMPtr<nsISupports> mGlobalWrapperRef;

  static int JSOptionChangedCallback(const char *pref, void *data);

  static JSBool DOMOperationCallback(JSContext *cx);
};

class nsIJSRuntimeService;

class nsJSRuntime : public nsIScriptRuntime
{
public:
  
  static JSRuntime *sRuntime;

public:
  
  NS_DECL_ISUPPORTS

  virtual PRUint32 GetScriptTypeID() {
            return nsIProgrammingLanguage::JAVASCRIPT;
  }

  virtual nsresult CreateContext(nsIScriptContext **ret);

  virtual nsresult ParseVersion(const nsString &aVersionStr, PRUint32 *flags);

  virtual nsresult DropScriptObject(void *object);
  virtual nsresult HoldScriptObject(void *object);
  
  static void Startup();
  static void Shutdown();
  
  static nsresult Init();
  
  static nsScriptNameSpaceManager* GetNameSpaceManager();
};





#define NS_IJSARGARRAY_IID \
 { /*{E96FB2AE-CB4F-44a0-81F8-D91C80AFE9A3} */ \
 0xe96fb2ae, 0xcb4f, 0x44a0, \
 { 0x81, 0xf8, 0xd9, 0x1c, 0x80, 0xaf, 0xe9, 0xa3 } }

class nsIJSArgArray: public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IJSARGARRAY_IID)
  
  
  
  virtual nsresult GetArgs(PRUint32 *argc, void **argv) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIJSArgArray, NS_IJSARGARRAY_IID)


nsresult NS_CreateJSRuntime(nsIScriptRuntime **aRuntime);


void NS_ScriptErrorReporter(JSContext *cx, const char *message, JSErrorReport *report);

#endif 
