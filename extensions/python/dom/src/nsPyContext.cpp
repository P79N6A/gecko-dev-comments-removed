


































#include "nsIScriptContext.h"
#include "nsIScriptGlobalObject.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsITimelineService.h"
#include "nsITimer.h"
#include "nsIArray.h"
#include "nsIAtom.h"
#include "prtime.h"
#include "nsStringAPI.h"
#include "nsIWeakReference.h" 
#include "nsIWeakReferenceUtils.h" 
#include "nsGUIEvent.h"
#include "nsServiceManagerUtils.h"
#include "nsDOMScriptObjectHolder.h"
#include "nsIDOMDocument.h"

#include "nsPyDOM.h"
#include "nsPyContext.h"
#include "compile.h"
#include "eval.h"
#include "marshal.h"

#ifdef NS_DEBUG
nsPyDOMObjectLeakStats gLeakStats;
#endif


static inline const char *
AtomToEventHandlerName(nsIAtom *aName)
{
  const char *name;

  aName->GetUTF8String(&name);

#ifdef DEBUG
  const char *cp;
  char c;
  for (cp = name; *cp != '\0'; ++cp)
  {
    c = *cp;
    NS_ASSERTION (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z'),
                  "non-ASCII non-alphabetic event handler name");
  }
#endif

  return name;
}




class nsPyObjectHolder : public nsISupports
{
public:
  NS_DECL_ISUPPORTS
  nsPyObjectHolder(PyObject *ob) : m_ob(ob) {Py_XINCREF(m_ob);}
  ~nsPyObjectHolder() {
	  CEnterLeavePython _celp;
	  Py_XDECREF(m_ob);
  }
private:
  PyObject *m_ob;
};


NS_INTERFACE_MAP_BEGIN(nsPyObjectHolder)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsPyObjectHolder)
NS_IMPL_RELEASE(nsPyObjectHolder)


nsPythonContext::nsPythonContext() :
    mIsInitialized(PR_FALSE),
    mScriptGlobal(nsnull),
    mScriptsEnabled(PR_TRUE),
    mProcessingScriptTag(PR_FALSE),
    mDelegate(NULL)
{
  PYLEAK_STAT_INCREMENT(ScriptContext);
}

nsPythonContext::~nsPythonContext()
{
  PYLEAK_STAT_DECREMENT(ScriptContext);
}


NS_INTERFACE_MAP_BEGIN(nsPythonContext)
  NS_INTERFACE_MAP_ENTRY(nsIScriptContext)
  NS_INTERFACE_MAP_ENTRY(nsITimerCallback)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIScriptContext)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsPythonContext)
NS_IMPL_RELEASE(nsPythonContext)


nsresult nsPythonContext::HandlePythonError()
{
  
  
  if (!PyErr_Occurred())
    return NS_OK;

  nsScriptErrorEvent errorevent(PR_TRUE, NS_LOAD_ERROR);
  nsString strFilename;

  PyObject *exc, *typ, *tb;
  PyErr_Fetch(&exc, &typ, &tb);
  PyErr_NormalizeException( &exc, &typ, &tb);
  
  
  if (tb) {
    PyObject *frame = PyObject_GetAttrString(tb, "tb_frame");
    if (frame) {
      PyObject *obLineNo = PyObject_GetAttrString(frame, "f_lineno");
      if (obLineNo) {
        errorevent.lineNr = PyInt_AsLong(obLineNo);
        Py_DECREF(obLineNo);
      } else {
        NS_ERROR("Traceback had no lineNo attribute?");
        PyErr_Clear();
      }
      PyObject *code = PyObject_GetAttrString(frame, "f_code");
      if (code) {
        PyObject *filename = PyObject_GetAttrString(code, "co_filename");
        if (filename && PyString_Check(filename)) {
          CopyUTF8toUTF16(nsCString(PyString_AsString(filename)), strFilename);
          errorevent.fileName = strFilename.get();
        }
        Py_XDECREF(filename);
        Py_DECREF(code);
      }
      Py_DECREF(frame);
    }
  }
  PRBool outOfMem = PyErr_GivenExceptionMatches(exc, PyExc_MemoryError);
  
  nsCAutoString cerrMsg;
  PyXPCOM_FormatGivenException(cerrMsg, exc, typ, tb);
  nsAutoString errMsg;
  CopyUTF8toUTF16(cerrMsg, errMsg);
  NS_ASSERTION(!errMsg.IsEmpty(), "Failed to extract a Python error message");
  errorevent.errorMsg = errMsg.get();
  nsEventStatus status = nsEventStatus_eIgnore;

  
  if (mScriptGlobal != nsnull && !outOfMem) {
    mScriptGlobal->HandleScriptError(&errorevent, &status);
  }

  PyErr_Restore(exc, typ, tb);

  if (status != nsEventStatus_eConsumeNoDefault) {
    
    
    nsCAutoString streamout("Python DOM script error");
    nsCAutoString level;
    if (status != nsEventStatus_eIgnore) {
      streamout += NS_LITERAL_CSTRING(" (suppressed by event handler)");
      level = NS_LITERAL_CSTRING("info");
    } else {
      level = NS_LITERAL_CSTRING("warning");
    }
    PyXPCOM_FormatCurrentException(streamout);
    PyXPCOM_Log(level.get(), streamout);
  }
  
  nsresult ret = PyXPCOM_SetCOMErrorFromPyException();
  PyErr_Clear();
  return ret;
}




void
nsPythonContext::WillInitializeContext()
{
  mIsInitialized = PR_FALSE;
  
  
  CEnterLeavePython _celp;
  if (!mDelegate) {
    
    PyObject *mod = PyImport_ImportModule("nsdom.context");
    if (mod==NULL) {
      HandlePythonError();
      return;
    }
    PyObject *klass = PyObject_GetAttrString(mod, "ScriptContext");
    Py_DECREF(mod);
    if (klass == NULL) {
      HandlePythonError();
      return;
    }
    mDelegate = PyObject_Call(klass, NULL, NULL);
    Py_DECREF(klass);
  }
  PyObject *ret = PyObject_CallMethod(mDelegate, "WillInitializeContext", NULL);
  if (ret == NULL)
    HandlePythonError();
}

void
nsPythonContext::DidInitializeContext()
{
  NS_ASSERTION(mDelegate, "No delegate!");
  if (mDelegate) {
    CEnterLeavePython _celp;
    PyObject *ret = PyObject_CallMethod(mDelegate, "DidInitializeContext", NULL);
    Py_XDECREF(ret);
    HandlePythonError();
  }
  mIsInitialized = PR_TRUE;
}

PRBool
nsPythonContext::IsContextInitialized()
{
  return mIsInitialized;
}

nsresult
nsPythonContext::InitContext(nsIScriptGlobalObject *aGlobalObject)
{
  NS_TIMELINE_MARK_FUNCTION("nsPythonContext::InitContext");
  NS_ENSURE_TRUE(!mIsInitialized, NS_ERROR_ALREADY_INITIALIZED);
  NS_ASSERTION(mDelegate != NULL, "WillInitContext didn't create delegate");

  if (mDelegate == NULL)
    return NS_ERROR_UNEXPECTED;

  CEnterLeavePython _celp;
  PyObject *obGlobal;
  if (aGlobalObject) {
    obGlobal = PyObject_FromNSDOMInterface(mDelegate, aGlobalObject,
                                           NS_GET_IID(nsIScriptGlobalObject));
    if (!obGlobal)
      return HandlePythonError();
  } else {
    obGlobal = Py_None;
    Py_INCREF(Py_None);
  }

  PyObject *ret = PyObject_CallMethod(mDelegate, "InitContext", "N", obGlobal);
  if (ret == NULL)
    return HandlePythonError();
  
  mScriptGlobal = aGlobalObject;
  return NS_OK;
}

void
nsPythonContext::DidSetDocument(nsISupports *aSupDoc, void *aGlobal)
{
  NS_TIMELINE_MARK_FUNCTION("nsPythonContext::DidSetDocument");
  NS_ASSERTION(mDelegate != NULL, "No delegate");

  if (mDelegate == NULL)
    return;

  CEnterLeavePython _celp;
  PyObject *obDoc;
  if (aSupDoc) {
    nsCOMPtr<nsIDOMDocument> doc(do_QueryInterface(aSupDoc));
    NS_ASSERTION(nsnull != doc, "not an nsIDOMDocument!?");
    if (nsnull != doc)
      obDoc = PyObject_FromNSDOMInterface(mDelegate, aSupDoc,
                                          NS_GET_IID(nsIDOMDocument));
    if (!obDoc) {
      HandlePythonError();
      return;
    }
  } else {
    obDoc = Py_None;
    Py_INCREF(Py_None);
  }
  PyObject *ret = PyObject_CallMethod(mDelegate, "DidSetDocument",
                                      "NO", obDoc, aGlobal);
  Py_XDECREF(ret);
  HandlePythonError();
}

nsresult
nsPythonContext::EvaluateStringWithValue(const nsAString& aScript,
                                     void *aScopeObject,
                                     nsIPrincipal *aPrincipal,
                                     const char *aURL,
                                     PRUint32 aLineNo,
                                     PRUint32 aVersion,
                                     void* aRetValue,
                                     PRBool* aIsUndefined)
{
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  if (!mScriptsEnabled) {
    if (aIsUndefined) {
      *aIsUndefined = PR_TRUE;
    }

    return NS_OK;
  }
  NS_ERROR("Not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsPythonContext::EvaluateString(const nsAString& aScript,
                            void *aScopeObject,
                            nsIPrincipal *aPrincipal,
                            const char *aURL,
                            PRUint32 aLineNo,
                            PRUint32 aVersion,
                            nsAString *aRetValue,
                            PRBool* aIsUndefined)
{
  NS_TIMELINE_MARK_FUNCTION("nsPythonContext::EvaluateString");
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  *aIsUndefined = PR_TRUE;
  if (!mScriptsEnabled) {
    *aIsUndefined = PR_TRUE;
    if (aRetValue) {
      aRetValue->Truncate();
    }
    return NS_OK;
  }

  CEnterLeavePython _celp;

  
  PyObject *obScript = PyObject_FromNSString(aScript);

  PyObject *ret = PyObject_CallMethod(mDelegate, "EvaluateString", "NOOsii",
                                      obScript,
                                      (PyObject *)aScopeObject,
                                      Py_None, 
                                      aURL,
                                      aLineNo,
                                      aVersion);

  
  Py_XDECREF(ret);
  return HandlePythonError();
}

nsresult
nsPythonContext::ExecuteScript(void* aScriptObject,
                           void *aScopeObject,
                           nsAString* aRetValue,
                           PRBool* aIsUndefined)
{
  NS_TIMELINE_MARK_FUNCTION("nsPythonContext::ExecuteScript");
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  if (aIsUndefined) {
    *aIsUndefined = PR_TRUE;
  }
  if (!mScriptsEnabled) {
    if (aRetValue) {
      aRetValue->Truncate();
    }
    return NS_OK;
  }
  NS_ENSURE_TRUE(aScriptObject, NS_ERROR_NULL_POINTER);
  NS_ASSERTION(mDelegate, "Script context has no delegate");
  NS_ENSURE_TRUE(mDelegate, NS_ERROR_UNEXPECTED);
  CEnterLeavePython _celp;

  PyObject *ret = PyObject_CallMethod(mDelegate, "ExecuteScript", "OO",
                                      aScriptObject, aScopeObject);
  
  if (!ret) {
    HandlePythonError();
    if (aRetValue)
      aRetValue->Truncate();
  } else if (ret == Py_None) {
    if (aRetValue)
      aRetValue->Truncate();
  } else {
    if (aRetValue) {
      PyObject_AsNSString(ret, *aRetValue);
    }
    if (aIsUndefined) {
      *aIsUndefined = PR_FALSE;
    }
  }
  Py_XDECREF(ret);
  return NS_OK;
}

nsresult
nsPythonContext::CompileScript(const PRUnichar* aText,
                           PRInt32 aTextLength,
                           void *aScopeObject,
                           nsIPrincipal *aPrincipal,
                           const char *aURL,
                           PRUint32 aLineNo,
                           PRUint32 aVersion,
                           nsScriptObjectHolder &aScriptObject)
{
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);
  NS_TIMELINE_MARK_FUNCTION("nsPythonContext::CompileScript");

  NS_ASSERTION(mDelegate, "Script context has no delegate");
  NS_ENSURE_TRUE(mDelegate, NS_ERROR_UNEXPECTED);

  CEnterLeavePython _celp;

  PyObject *obCode = PyObject_FromNSString(aText, aTextLength);
  if (!obCode)
    return HandlePythonError();
  PyObject *obPrincipal = Py_None; 

  PyObject *ret = PyObject_CallMethod(mDelegate, "CompileScript",
                                      "NOOsii",
                                      obCode,
                                      aScopeObject ? aScopeObject : Py_None,
                                      obPrincipal,
                                      aURL,
                                      aLineNo,
                                      aVersion);
  if (!ret) {
    return HandlePythonError();
  }
  NS_ASSERTION(aScriptObject.getScriptTypeID()==nsIProgrammingLanguage::PYTHON,
               "Expecting Python script object holder");
  aScriptObject.set(ret);
  Py_DECREF(ret);
  return NS_OK;
}

nsresult
nsPythonContext::CompileEventHandler(nsIAtom *aName,
                                 PRUint32 aArgCount,
                                 const char** aArgNames,
                                 const nsAString& aBody,
                                 const char *aURL, PRUint32 aLineNo,
                                 nsScriptObjectHolder &aHandler)
{
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);
  NS_TIMELINE_MARK_FUNCTION("nsPythonContext::CompileEventHandler");

  NS_ASSERTION(mDelegate, "Script context has no delegate");
  NS_ENSURE_TRUE(mDelegate, NS_ERROR_UNEXPECTED);

  CEnterLeavePython _celp;

  aHandler.drop();

  PyObject *argNames = PyList_New(aArgCount);
  if (!argNames)
    return HandlePythonError();
  for (PRUint32 i=0;i<aArgCount;i++) {
    PyList_SET_ITEM(argNames, i, PyString_FromString(aArgNames[i]));
  }
  PyObject *ret = PyObject_CallMethod(mDelegate, "CompileEventHandler",
                                      "sNNsi",
                                      AtomToEventHandlerName(aName),
                                      argNames,
                                      PyObject_FromNSString(aBody),
                                      aURL, aLineNo);
  if (!ret)
    return HandlePythonError();

  NS_ASSERTION(aHandler.getScriptTypeID()==nsIProgrammingLanguage::PYTHON,
               "Expecting Python script object holder");
  aHandler.set(ret);
  Py_DECREF(ret);
  return NS_OK;
}

nsresult
nsPythonContext::BindCompiledEventHandler(nsISupports *aTarget, void *aScope,
                                          nsIAtom *aName, void *aHandler)
{
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  NS_ASSERTION(mDelegate, "Script context has no delegate");
  NS_ENSURE_TRUE(mDelegate, NS_ERROR_UNEXPECTED);

  CEnterLeavePython _celp;

  PyObject *obTarget;
  obTarget = PyObject_FromNSDOMInterface(mDelegate, aTarget);
  if (!obTarget)
    return HandlePythonError();

  PyObject *ret = PyObject_CallMethod(mDelegate, "BindCompiledEventHandler",
                                      "NOsO",
                                      obTarget, aScope,
                                      AtomToEventHandlerName(aName),
                                      aHandler);
  Py_XDECREF(ret);
  return HandlePythonError();
}


nsresult
nsPythonContext::CompileFunction(void* aTarget,
                             const nsACString& aName,
                             PRUint32 aArgCount,
                             const char** aArgArray,
                             const nsAString& aBody,
                             const char* aURL,
                             PRUint32 aLineNo,
                             PRBool aShared,
                             void** aFunctionObject)
{
  NS_TIMELINE_MARK_FUNCTION("nsPythonContext::CompileFunction");
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  NS_ERROR("CompileFunction not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsPythonContext::CallEventHandler(nsISupports *aTarget, void *aScope, void* aHandler,
                                    nsIArray *aargv, nsIVariant **arv)
{
  NS_TIMELINE_MARK_FUNCTION("nsPythonContext::CallEventHandler");
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  NS_ASSERTION(mDelegate, "Script context has no delegate");
  NS_ENSURE_TRUE(mDelegate, NS_ERROR_UNEXPECTED);

  CEnterLeavePython _celp;

  PyObject *obTarget;
  obTarget = PyObject_FromNSDOMInterface(mDelegate, aTarget);
  if (!obTarget)
    return HandlePythonError();

  
  
  PyObject *obArgv = NULL;
  nsCOMPtr<nsIPyArgArray> pyargv(do_QueryInterface(aargv));
  if (pyargv) {
    obArgv = pyargv->GetArgs();
    Py_XINCREF(obArgv); 
  }
  if (!obArgv) {
    obArgv = PyObject_FromNSDOMInterface(mDelegate, aargv);
    if (!obArgv) {
      Py_DECREF(obTarget);
      return HandlePythonError();
    }
  }

  PyObject *ret = PyObject_CallMethod(mDelegate, "CallEventHandler",
                                      "NOON",
                                      obTarget, aScope, aHandler,
                                      obArgv);
  if (ret) {
    PyObject_AsVariant(ret, arv);
    Py_DECREF(ret);
  }
  return HandlePythonError();
}

nsresult
nsPythonContext::GetBoundEventHandler(nsISupports* aTarget, void *aScope,
                                      nsIAtom* aName,
                                      nsScriptObjectHolder &aHandler)
{
  NS_TIMELINE_MARK_FUNCTION("nsPythonContext::GetBoundEventHandler");
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  NS_ASSERTION(mDelegate, "Script context has no delegate");
  NS_ENSURE_TRUE(mDelegate, NS_ERROR_UNEXPECTED);

  CEnterLeavePython _celp;

  aHandler.drop();

  PyObject *obTarget;
  obTarget = PyObject_FromNSDOMInterface(mDelegate, aTarget);
  if (!obTarget)
    return HandlePythonError();

  PyObject *ret = PyObject_CallMethod(mDelegate, "GetBoundEventHandler",
                                      "NOs",
                                      obTarget, aScope,
                                      AtomToEventHandlerName(aName));
  if (!ret)
    return HandlePythonError();
  if (ret == Py_None)
    return NS_OK;
  NS_ASSERTION(aHandler.getScriptTypeID()==nsIProgrammingLanguage::PYTHON,
               "Expecting Python script object holder");
  aHandler.set(ret);
  Py_DECREF(ret);
  return NS_OK;
}

nsresult
nsPythonContext::SetProperty(void *aTarget, const char *aPropName, nsISupports *aVal)
{
  NS_TIMELINE_MARK_FUNCTION("nsPythonContext::SetProperty");
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  NS_ASSERTION(mDelegate, "Script context has no delegate");
  NS_ENSURE_TRUE(mDelegate, NS_ERROR_UNEXPECTED);

  CEnterLeavePython _celp;
  PyObject *obVal;
  obVal = PyObject_FromNSDOMInterface(mDelegate, aVal);
  if (!obVal)
    return HandlePythonError();

  PyObject *ret = PyObject_CallMethod(mDelegate, "SetProperty",
                                      "OsN",
                                      aTarget, aPropName, obVal);
  Py_XDECREF(ret);
  return HandlePythonError();
}

nsresult
nsPythonContext::InitClasses(void *aGlobalObj)
{
    return NS_OK;
}

nsresult
nsPythonContext::Serialize(nsIObjectOutputStream* aStream, void *aScriptObject)
{
  NS_TIMELINE_MARK_FUNCTION("nsPythonContext::Serialize");
  CEnterLeavePython _celp;
  nsresult rv;
  PyObject *pyScriptObject = (PyObject *)aScriptObject;
  if (!PyCode_Check(pyScriptObject) && !PyFunction_Check(pyScriptObject)) {
    NS_ERROR("aScriptObject is not a code or function object");
    return NS_ERROR_UNEXPECTED;
  }
  rv = aStream->Write32(PyImport_GetMagicNumber());
  if (NS_FAILED(rv)) return rv;

  PyObject *obMarshal =
#ifdef Py_MARSHAL_VERSION
      PyMarshal_WriteObjectToString((PyObject *)aScriptObject,
                                    Py_MARSHAL_VERSION);
#else
      
      PyMarshal_WriteObjectToString((PyObject *)aScriptObject);

#endif
  if (!obMarshal)
    return HandlePythonError();
  NS_ASSERTION(PyString_Check(obMarshal), "marshal returned a non string?");
  rv |= aStream->Write32(PyString_GET_SIZE(obMarshal));
  rv |= aStream->WriteBytes(PyString_AS_STRING(obMarshal), PyString_GET_SIZE(obMarshal));
  Py_DECREF(obMarshal);
  return rv;
}

nsresult
nsPythonContext::Deserialize(nsIObjectInputStream* aStream,
                             nsScriptObjectHolder &aResult)
{
  NS_TIMELINE_MARK_FUNCTION("nsPythonContext::Deserialize");
  nsresult rv;
  PRUint32 magic;
  aResult.drop();
  rv = aStream->Read32(&magic);
  if (NS_FAILED(rv)) return rv;

  
  
  
  
  PRUint32 nBytes;
  rv = aStream->Read32(&nBytes);
  if (NS_FAILED(rv)) return rv;

  char* data = nsnull;
  rv = aStream->ReadBytes(nBytes, &data);
  if (NS_FAILED(rv)) return rv;

  if (magic != (PRUint32)PyImport_GetMagicNumber()) {
    NS_WARNING("Python has different marshal version");
    if (data)
      nsMemory::Free(data);
    return NS_OK;
  }

  CEnterLeavePython _celp;
  PyObject *codeObject = PyMarshal_ReadObjectFromString(data, nBytes);
  if (data)
    nsMemory::Free(data);
  if (codeObject == NULL)
    return HandlePythonError();
  NS_ASSERTION(PyCode_Check(codeObject) || PyFunction_Check(codeObject),
               "unmarshal returned non code/functions");
  NS_ASSERTION(aResult.getScriptTypeID()==nsIProgrammingLanguage::PYTHON,
               "Expecting Python script object holder");
  aResult.set(codeObject);
  Py_DECREF(codeObject);
  return NS_OK;
}

void
nsPythonContext::SetDefaultLanguageVersion(PRUint32 aVersion)
{
  return;
}

nsIScriptGlobalObject *
nsPythonContext::GetGlobalObject()
{
  return mScriptGlobal;
}

void *
nsPythonContext::GetNativeContext()
{
  return nsnull;
}

void *
nsPythonContext::GetNativeGlobal()
{
  NS_ASSERTION(mDelegate, "Script context has no delegate");
  NS_ENSURE_TRUE(mDelegate, nsnull);
  NS_TIMELINE_MARK_FUNCTION("nsPythonContext::GetNativeGlobal");
  CEnterLeavePython _celp;
  PyObject *ret = PyObject_CallMethod(mDelegate, "GetNativeGlobal", NULL);
  if (!ret) {
    HandlePythonError();
    return nsnull;
  }
  
  NS_ASSERTION(ret->ob_refcnt > 1, "Can't have a new object here!?");
  Py_DECREF(ret);
  return ret;
}

nsresult
nsPythonContext::CreateNativeGlobalForInner(
                                nsIScriptGlobalObject *aGlobal,
                                PRBool aIsChrome,
                                void **aNativeGlobal, nsISupports **aHolder)
{
  NS_TIMELINE_MARK_FUNCTION("nsPythonContext::CreateNativeGlobalForInner");
  CEnterLeavePython _celp;
  PyObject *obGlob = PyObject_FromNSDOMInterface(mDelegate, aGlobal,
                                          NS_GET_IID(nsIScriptGlobalObject));
  if (!obGlob)
    return HandlePythonError();
  
  PyObject *ret = PyObject_CallMethod(mDelegate,
                                      "CreateNativeGlobalForInner",
                                      "Ni", obGlob, aIsChrome);
  if (!ret) {
    HandlePythonError();
    return nsnull;
  }
  
  nsPyObjectHolder *holder = new nsPyObjectHolder(ret);
  *aNativeGlobal = ret;
  Py_DECREF(ret);
  if (!holder)
    return NS_ERROR_OUT_OF_MEMORY;

  return holder->QueryInterface(NS_GET_IID(nsISupports), (void **)aHolder);
}

nsresult
nsPythonContext::ConnectToInner(nsIScriptGlobalObject *aNewInner, void *aOuterGlobal)
{
  NS_ENSURE_ARG(aNewInner);
  NS_TIMELINE_MARK_FUNCTION("nsPythonContext::ConnectToInner");
  CEnterLeavePython _celp;
  PyObject *obNewInner = PyObject_FromNSDOMInterface(mDelegate, aNewInner,
                                          NS_GET_IID(nsIScriptGlobalObject));
  if (!obNewInner)
    return HandlePythonError();
  
  PyObject *obInnerScope = (PyObject *)aNewInner->GetScriptGlobal(
                                            nsIProgrammingLanguage::PYTHON);

  PyObject *ret = PyObject_CallMethod(mDelegate, "ConnectToInner", "NOO",
                                      obNewInner, aOuterGlobal, obInnerScope);
  Py_XDECREF(ret);
  return HandlePythonError();
}

PRBool
nsPythonContext::GetProcessingScriptTag()
{
  return mProcessingScriptTag;
}

void
nsPythonContext::SetProcessingScriptTag(PRBool aResult)
{
  mProcessingScriptTag = aResult;
}

PRBool
nsPythonContext::GetScriptsEnabled()
{
  return mScriptsEnabled;
}

void
nsPythonContext::SetScriptsEnabled(PRBool aEnabled, PRBool aFireTimeouts)
{
  
  
  mScriptsEnabled = aEnabled;

  nsIScriptGlobalObject *global = GetGlobalObject();

  if (global) {
    global->SetScriptsEnabled(aEnabled, aFireTimeouts);
  }
}

nsresult
nsPythonContext::SetTerminationFunction(nsScriptTerminationFunc aFunc,
                                    nsISupports* aRef)
{
    NS_ERROR("Term functions need thought");
    return NS_ERROR_UNEXPECTED;
}

void
nsPythonContext::ScriptEvaluated(PRBool aTerminated)
{
  ;
}

void
nsPythonContext::FinalizeContext()
{
  NS_ASSERTION(mDelegate, "No delegate?");
  NS_ASSERTION(!mScriptGlobal, "ClearScope not called to clean me up?");
  if (mDelegate) {
    CEnterLeavePython _celp;
    PyObject *ret = PyObject_CallMethod(mDelegate, "FinalizeContext", NULL);
    HandlePythonError();
    Py_XDECREF(ret);
    Py_DECREF(mDelegate);
    mDelegate = NULL;
  }
}

void
nsPythonContext::GC()
{
  
}

void
nsPythonContext::ClearScope(void *aGlobalObj, PRBool aWhatever)
{
  CEnterLeavePython _celp;
  if (mDelegate) {
    PyObject *ret = PyObject_CallMethod(mDelegate, "ClearScope",
                                        "O",
                                        aGlobalObj);
    Py_XDECREF(ret);
  }
  mScriptGlobal = nsnull;
  HandlePythonError();
}

NS_IMETHODIMP
nsPythonContext::Notify(nsITimer *timer)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsPythonContext::DropScriptObject(void *object)
{
  if (object) {
    PYLEAK_STAT_DECREMENT(ScriptObject);
    CEnterLeavePython _celp;
    Py_DECREF((PyObject *)object);
  }
  return NS_OK;
}

nsresult
nsPythonContext::HoldScriptObject(void *object)
{
  if (object) {
    PYLEAK_STAT_INCREMENT(ScriptObject);
    CEnterLeavePython _celp;
    Py_INCREF((PyObject *)object);
  }
  return NS_OK;
}
