



































#include "nsIScriptContext.h"
#include "nsITimer.h"
 
#include "nsPyDOM.h"
class nsIScriptObjectOwner;
class nsIArray;

#ifdef NS_DEBUG
class nsPyDOMObjectLeakStats
  {
    public:
      nsPyDOMObjectLeakStats()
        : mEventHandlerCount(0), mScriptObjectCount(0), mBindingCount(0),
          mScriptContextCount(0) {}

      ~nsPyDOMObjectLeakStats()
        {
          printf("pydom leaked objects:\n");
          PRBool leaked = PR_FALSE;
#define CHECKLEAK(attr) \
          if (attr) { \
            printf(" => %-20s % 6d\n", #attr ":", attr); \
            leaked = PR_TRUE; \
          }
      
          CHECKLEAK(mScriptObjectCount);
          CHECKLEAK(mBindingCount);
          CHECKLEAK(mScriptContextCount);
          extern PRInt32 cPyDOMISupportsObjects;
          if (_PyXPCOM_GetInterfaceCount()) {
            printf(" => %-20s % 6d (%d are nsdom objects)\n",
                   "pyxpcom interfaces:",
                   _PyXPCOM_GetInterfaceCount(), cPyDOMISupportsObjects
                   );
            leaked = PR_TRUE;
          }

          if (_PyXPCOM_GetGatewayCount()) {
            printf(" => %-20s % 6d\n",
                   "pyxpcom gateways:", _PyXPCOM_GetGatewayCount());
            leaked = PR_TRUE;
          }
          if (!leaked)
            printf(" => no leaks.\n");
#ifdef Py_DEBUG 
          printf(" %d Python references remain\n", _Py_RefTotal);
#endif
        }

      PRInt32 mEventHandlerCount;
      PRInt32 mScriptObjectCount;
      PRInt32 mBindingCount;
      PRInt32 mScriptContextCount;
  };
extern nsPyDOMObjectLeakStats gLeakStats;
#define PYLEAK_STAT_INCREMENT(_s) PR_AtomicIncrement(&gLeakStats.m ## _s ## Count)
#define PYLEAK_STAT_XINCREMENT(_what, _s) if (_what) PR_AtomicIncrement(&gLeakStats.m ## _s ## Count)
#define PYLEAK_STAT_DECREMENT(_s) PR_AtomicDecrement(&gLeakStats.m ## _s ## Count)
#define PYLEAK_STAT_XDECREMENT(_what, _s) if (_what) PR_AtomicDecrement(&gLeakStats.m ## _s ## Count)
#else
#define PYLEAK_STAT_INCREMENT(_s)
#define PYLEAK_STAT_XINCREMENT(_what, _s)
#define PYLEAK_STAT_DECREMENT(_s)
#define PYLEAK_STAT_XDECREMENT(_what, _s)
#endif

class nsPythonContext : public nsIScriptContext,
                        public nsITimerCallback
{
public:
  nsPythonContext();
  virtual ~nsPythonContext();

  NS_DECL_ISUPPORTS

  virtual PRUint32 GetScriptTypeID()
    { return nsIProgrammingLanguage::PYTHON; }

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
                                       const char *aURL,
                                       PRUint32 aLineNo,
                                       nsScriptObjectHolder &aHandler);
  virtual nsresult CallEventHandler(nsISupports* aTarget, void *aScope,
                                    void* aHandler,
                                    nsIArray *argv, nsIVariant **rv);
  virtual nsresult BindCompiledEventHandler(nsISupports*aTarget, void *aScope,
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

  virtual nsresult SetProperty(void *aTarget, const char *aPropName,
                               nsISupports *aVal);

  virtual PRBool GetProcessingScriptTag();
  virtual void SetProcessingScriptTag(PRBool aResult);

  virtual void SetGCOnDestruction(PRBool aGCOnDestruction) {;}

  virtual nsresult InitClasses(void *aGlobalObj);
  virtual void ClearScope(void* aGlobalObj, PRBool aWhatever);

  virtual void WillInitializeContext();
  virtual void DidInitializeContext();
  virtual void DidSetDocument(nsISupports *aDocdoc, void *aGlobal);

  virtual nsresult Serialize(nsIObjectOutputStream* aStream, void *aScriptObject);
  virtual nsresult Deserialize(nsIObjectInputStream* aStream,
                               nsScriptObjectHolder &aResult);

  virtual nsresult HoldScriptObject(void *object);
  virtual nsresult DropScriptObject(void *object);

  NS_DECL_NSITIMERCALLBACK
  
  PyObject *PyObject_FromInterface(nsISupports *target,
                                   const nsIID &iid)
  {
    return ::PyObject_FromNSDOMInterface(mDelegate, target, iid);
  }
protected:

  
  nsCAutoString FixSource(const nsAString &aSource);
  PyObject *InternalCompile(const nsAString &source, const char *url,
                            PRUint32 lineNo);

  PRPackedBool mIsInitialized;
  PRPackedBool mScriptsEnabled;
  PRPackedBool mProcessingScriptTag;

  
  nsIScriptGlobalObject *mScriptGlobal;

  nsresult HandlePythonError();

  PyObject *mDelegate;  
};
