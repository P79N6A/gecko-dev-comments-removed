





































#include "nsIAtom.h"
#include "nsString.h"
#include "jsapi.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIScriptGlobalObject.h"
#include "nsString.h"
#include "nsUnicharUtils.h"
#include "nsReadableUtils.h"
#include "nsXBLProtoImplMethod.h"
#include "nsIScriptContext.h"
#include "nsContentUtils.h"
#include "nsIScriptSecurityManager.h"
#include "nsIXPConnect.h"

nsXBLProtoImplMethod::nsXBLProtoImplMethod(const PRUnichar* aName) :
  nsXBLProtoImplMember(aName), 
  mUncompiledMethod(nsnull)
#ifdef DEBUG
  , mIsCompiled(PR_FALSE)
#endif
{
  MOZ_COUNT_CTOR(nsXBLProtoImplMethod);
}

nsXBLProtoImplMethod::~nsXBLProtoImplMethod()
{
  MOZ_COUNT_DTOR(nsXBLProtoImplMethod);
}

void
nsXBLProtoImplMethod::Destroy(PRBool aIsCompiled)
{
  NS_PRECONDITION(aIsCompiled == mIsCompiled,
                  "Incorrect aIsCompiled in nsXBLProtoImplMethod::Destroy");
  if (aIsCompiled) {
    if (mJSMethodObject)
      nsContentUtils::RemoveJSGCRoot(&mJSMethodObject);
    mJSMethodObject = nsnull;
  }
  else {
    delete mUncompiledMethod;
    mUncompiledMethod = nsnull;
  }
}

void 
nsXBLProtoImplMethod::AppendBodyText(const nsAString& aText)
{
  NS_PRECONDITION(!mIsCompiled,
                  "Must not be compiled when accessing uncompiled method");
  if (!mUncompiledMethod) {
    mUncompiledMethod = new nsXBLUncompiledMethod();
    if (!mUncompiledMethod)
      return;
  }

  mUncompiledMethod->AppendBodyText(aText);
}

void 
nsXBLProtoImplMethod::AddParameter(const nsAString& aText)
{
  NS_PRECONDITION(!mIsCompiled,
                  "Must not be compiled when accessing uncompiled method");
  if (!mUncompiledMethod) {
    mUncompiledMethod = new nsXBLUncompiledMethod();
    if (!mUncompiledMethod)
      return;
  }

  mUncompiledMethod->AddParameter(aText);
}

void
nsXBLProtoImplMethod::SetLineNumber(PRUint32 aLineNumber)
{
  NS_PRECONDITION(!mIsCompiled,
                  "Must not be compiled when accessing uncompiled method");
  if (!mUncompiledMethod) {
    mUncompiledMethod = new nsXBLUncompiledMethod();
    if (!mUncompiledMethod)
      return;
  }

  mUncompiledMethod->SetLineNumber(aLineNumber);
}

nsresult
nsXBLProtoImplMethod::InstallMember(nsIScriptContext* aContext,
                                    nsIContent* aBoundElement, 
                                    void* aScriptObject,
                                    void* aTargetClassObject,
                                    const nsCString& aClassStr)
{
  NS_PRECONDITION(mIsCompiled,
                  "Should not be installing an uncompiled method");
  JSContext* cx = (JSContext*) aContext->GetNativeContext();

  nsIDocument *ownerDoc = aBoundElement->GetOwnerDoc();
  nsIScriptGlobalObject *sgo;

  if (!ownerDoc || !(sgo = ownerDoc->GetScriptGlobalObject())) {
    NS_ERROR("Can't find global object for bound content!");
 
    return NS_ERROR_UNEXPECTED;
  }

  JSObject * scriptObject = (JSObject *) aScriptObject;
  NS_ASSERTION(scriptObject, "uh-oh, script Object should NOT be null or bad things will happen");
  if (!scriptObject)
    return NS_ERROR_FAILURE;

  JSObject * targetClassObject = (JSObject *) aTargetClassObject;
  JSObject * globalObject = sgo->GetGlobalJSObject();

  
  if (mJSMethodObject && targetClassObject) {
    nsDependentString name(mName);
    JSAutoRequest ar(cx);
    JSObject * method = ::JS_CloneFunctionObject(cx, mJSMethodObject, globalObject);
    if (!method) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    nsresult rv;
    nsAutoGCRoot root(&method, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (!::JS_DefineUCProperty(cx, targetClassObject,
                               NS_REINTERPRET_CAST(const jschar*, mName), 
                               name.Length(), OBJECT_TO_JSVAL(method),
                               NULL, NULL, JSPROP_ENUMERATE)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }
  return NS_OK;
}

nsresult 
nsXBLProtoImplMethod::CompileMember(nsIScriptContext* aContext, const nsCString& aClassStr,
                                    void* aClassObject)
{
  NS_PRECONDITION(!mIsCompiled,
                  "Trying to compile an already-compiled method");
  NS_PRECONDITION(aClassObject,
                  "Must have class object to compile");

#ifdef DEBUG
  
  mIsCompiled = PR_TRUE;
#endif
  
  
  if (!mUncompiledMethod)
    return NS_OK;

  
  if (!mName) {
    delete mUncompiledMethod;
    mUncompiledMethod = nsnull;
    return NS_OK;
  }

#ifdef DEBUG
  
  
  mIsCompiled = PR_FALSE;
#endif

  
  
  PRInt32 paramCount = mUncompiledMethod->GetParameterCount();
  char** args = nsnull;
  if (paramCount > 0) {
    args = new char*[paramCount];
    if (!args)
      return NS_ERROR_OUT_OF_MEMORY;
  }

  
  PRInt32 argPos = 0; 
  for (nsXBLParameter* curr = mUncompiledMethod->mParameters; 
       curr; 
       curr = curr->mNext) {
    args[argPos] = curr->mName;
    argPos++;
  }

  
  nsDependentString body;
  PRUnichar *bodyText = mUncompiledMethod->mBodyText.GetText();
  if (bodyText)
    body.Rebind(bodyText);

  
  
  NS_ConvertUTF16toUTF8 cname(mName);
  nsCAutoString functionUri(aClassStr);
  PRInt32 hash = functionUri.RFindChar('#');
  if (hash != kNotFound) {
    functionUri.Truncate(hash);
  }

  JSObject* methodObject = nsnull;
  nsresult rv = aContext->CompileFunction(aClassObject,
                                          cname,
                                          paramCount,
                                          (const char**)args,
                                          body, 
                                          functionUri.get(),
                                          mUncompiledMethod->mBodyText.GetLineNumber(),
                                          PR_TRUE,
                                          (void **) &methodObject);

  
  delete mUncompiledMethod;
  delete [] args;
  if (NS_FAILED(rv)) {
    mUncompiledMethod = nsnull;
    return rv;
  }

  mJSMethodObject = methodObject;

  if (methodObject) {
    
    rv = nsContentUtils::AddJSGCRoot(&mJSMethodObject,
                                     "nsXBLProtoImplMethod::mJSMethodObject");
    if (NS_FAILED(rv)) {
      mJSMethodObject = nsnull;
    }
  }
  
#ifdef DEBUG
  mIsCompiled = NS_SUCCEEDED(rv);
#endif
  return rv;
}

void
nsXBLProtoImplMethod::Traverse(nsCycleCollectionTraversalCallback &cb) const
{
  if (mIsCompiled) {
    cb.NoteScriptChild(nsIProgrammingLanguage::JAVASCRIPT, mJSMethodObject);
  }
}

nsresult
nsXBLProtoImplAnonymousMethod::Execute(nsIContent* aBoundElement)
{
  NS_PRECONDITION(mIsCompiled, "Can't execute uncompiled method");
  
  if (!mJSMethodObject) {
    
    return NS_OK;
  }

  
  
  nsIDocument* document = aBoundElement->GetOwnerDoc();
  if (!document) {
    return NS_OK;
  }

  nsIScriptGlobalObject* global = document->GetScriptGlobalObject();
  if (!global) {
    return NS_OK;
  }

  nsCOMPtr<nsIScriptContext> context = global->GetContext();
  if (!context) {
    return NS_OK;
  }
  
  JSContext* cx = (JSContext*) context->GetNativeContext();

  JSObject* globalObject = global->GetGlobalJSObject();

  nsCOMPtr<nsIXPConnectJSObjectHolder> wrapper;
  nsresult rv =
    nsContentUtils::XPConnect()->WrapNative(cx, globalObject,
                                            aBoundElement,
                                            NS_GET_IID(nsISupports),
                                            getter_AddRefs(wrapper));
  NS_ENSURE_SUCCESS(rv, rv);

  JSObject* thisObject;
  rv = wrapper->GetJSObject(&thisObject);
  NS_ENSURE_SUCCESS(rv, rv);

  JSAutoRequest ar(cx);

  
  
  
  JSObject* method = ::JS_CloneFunctionObject(cx, mJSMethodObject, thisObject);
  if (!method)
    return NS_ERROR_OUT_OF_MEMORY;

  

  
  nsCxPusher pusher(aBoundElement);

  
  rv = nsContentUtils::GetSecurityManager()->CheckFunctionAccess(cx, method,
                                                                 thisObject);

  JSBool ok = JS_TRUE;
  if (NS_SUCCEEDED(rv)) {
    jsval retval;
    ok = ::JS_CallFunctionValue(cx, thisObject, OBJECT_TO_JSVAL(method),
                                0 , nsnull , &retval);
  }

  if (!ok) {
    
    
    ::JS_ReportPendingException(cx);
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}
