





































#include "nsXBLDocumentInfo.h"
#include "nsHashtable.h"
#include "nsIDocument.h"
#include "nsXBLPrototypeBinding.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptContext.h"
#include "nsIScriptRuntime.h"
#include "nsIDOMScriptObjectFactory.h"
#include "jsapi.h"
#include "nsIURI.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "nsIChromeRegistry.h"
#include "nsIPrincipal.h"
#include "nsIScriptSecurityManager.h"
#include "nsContentUtils.h"
#include "nsDOMJSUtils.h"
#include "mozilla/Services.h"
#include "xpcpublic.h"
 
static NS_DEFINE_CID(kDOMScriptObjectFactoryCID, NS_DOM_SCRIPT_OBJECT_FACTORY_CID);



class nsXBLDocGlobalObject : public nsIScriptGlobalObject,
                             public nsIScriptObjectPrincipal
{
public:
  nsXBLDocGlobalObject(nsIScriptGlobalObjectOwner *aGlobalObjectOwner);

  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  
  
  virtual nsresult EnsureScriptEnvironment(PRUint32 aLangID);
  virtual nsresult SetScriptContext(PRUint32 lang_id, nsIScriptContext *aContext);

  virtual nsIScriptContext *GetContext();
  virtual JSObject *GetGlobalJSObject();
  virtual void OnFinalize(PRUint32 aLangID, void *aScriptGlobal);
  virtual void SetScriptsEnabled(PRBool aEnabled, PRBool aFireTimeouts);

  
  virtual nsIPrincipal* GetPrincipal();

  static JSBool doCheckAccess(JSContext *cx, JSObject *obj, jsid id,
                              PRUint32 accessType);

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsXBLDocGlobalObject,
                                           nsIScriptGlobalObject)

  void ClearGlobalObjectOwner();

protected:
  virtual ~nsXBLDocGlobalObject();

  void SetContext(nsIScriptContext *aContext);
  nsIScriptContext *GetScriptContext(PRUint32 language);
  void *GetScriptGlobal(PRUint32 language);

  nsCOMPtr<nsIScriptContext> mScriptContext;
  JSObject *mJSObject;    

  nsIScriptGlobalObjectOwner* mGlobalObjectOwner; 
  static JSClass gSharedGlobalClass;
};

JSBool
nsXBLDocGlobalObject::doCheckAccess(JSContext *cx, JSObject *obj, jsid id, PRUint32 accessType)
{
  nsIScriptSecurityManager *ssm = nsContentUtils::GetSecurityManager();
  if (!ssm) {
    ::JS_ReportError(cx, "Unable to verify access to a global object property.");
    return JS_FALSE;
  }

  
  
  while (JS_GET_CLASS(cx, obj) != &nsXBLDocGlobalObject::gSharedGlobalClass) {
    obj = ::JS_GetPrototype(cx, obj);
    if (!obj) {
      ::JS_ReportError(cx, "Invalid access to a global object property.");
      return JS_FALSE;
    }
  }

  nsresult rv = ssm->CheckPropertyAccess(cx, obj, JS_GET_CLASS(cx, obj)->name,
                                         id, accessType);
  return NS_SUCCEEDED(rv);
}

static JSBool
nsXBLDocGlobalObject_getProperty(JSContext *cx, JSObject *obj,
                                 jsid id, jsval *vp)
{
  return nsXBLDocGlobalObject::
    doCheckAccess(cx, obj, id, nsIXPCSecurityManager::ACCESS_GET_PROPERTY);
}

static JSBool
nsXBLDocGlobalObject_setProperty(JSContext *cx, JSObject *obj,
                                 jsid id, JSBool strict, jsval *vp)
{
  return nsXBLDocGlobalObject::
    doCheckAccess(cx, obj, id, nsIXPCSecurityManager::ACCESS_SET_PROPERTY);
}

static JSBool
nsXBLDocGlobalObject_checkAccess(JSContext *cx, JSObject *obj, jsid id,
                                 JSAccessMode mode, jsval *vp)
{
  PRUint32 translated;
  if (mode & JSACC_WRITE) {
    translated = nsIXPCSecurityManager::ACCESS_SET_PROPERTY;
  } else {
    translated = nsIXPCSecurityManager::ACCESS_GET_PROPERTY;
  }

  return nsXBLDocGlobalObject::
    doCheckAccess(cx, obj, id, translated);
}

static void
nsXBLDocGlobalObject_finalize(JSContext *cx, JSObject *obj)
{
  nsISupports *nativeThis = (nsISupports*)JS_GetPrivate(cx, obj);

  nsCOMPtr<nsIScriptGlobalObject> sgo(do_QueryInterface(nativeThis));

  if (sgo)
    sgo->OnFinalize(nsIProgrammingLanguage::JAVASCRIPT, obj);

  
  NS_RELEASE(nativeThis);
}

static JSBool
nsXBLDocGlobalObject_resolve(JSContext *cx, JSObject *obj, jsid id)
{
  JSBool did_resolve = JS_FALSE;
  return JS_ResolveStandardClass(cx, obj, id, &did_resolve);
}


JSClass nsXBLDocGlobalObject::gSharedGlobalClass = {
    "nsXBLPrototypeScript compilation scope",
    JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS | JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub,  JS_PropertyStub,
    nsXBLDocGlobalObject_getProperty, nsXBLDocGlobalObject_setProperty,
    JS_EnumerateStub, nsXBLDocGlobalObject_resolve,
    JS_ConvertStub, nsXBLDocGlobalObject_finalize,
    NULL, nsXBLDocGlobalObject_checkAccess
};






nsXBLDocGlobalObject::nsXBLDocGlobalObject(nsIScriptGlobalObjectOwner *aGlobalObjectOwner)
    : mJSObject(nsnull),
      mGlobalObjectOwner(aGlobalObjectOwner) 
{
}


nsXBLDocGlobalObject::~nsXBLDocGlobalObject()
{}


NS_IMPL_CYCLE_COLLECTION_1(nsXBLDocGlobalObject, mScriptContext)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsXBLDocGlobalObject)
  NS_INTERFACE_MAP_ENTRY(nsIScriptGlobalObject)
  NS_INTERFACE_MAP_ENTRY(nsIScriptObjectPrincipal)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIScriptGlobalObject)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsXBLDocGlobalObject, nsIScriptGlobalObject)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(nsXBLDocGlobalObject, nsIScriptGlobalObject)

void
XBL_ProtoErrorReporter(JSContext *cx,
                       const char *message,
                       JSErrorReport *report)
{
  
  
  nsCOMPtr<nsIScriptError>
    errorObject(do_CreateInstance("@mozilla.org/scripterror;1"));
  nsCOMPtr<nsIConsoleService>
    consoleService(do_GetService("@mozilla.org/consoleservice;1"));

  if (errorObject && consoleService) {
    PRUint32 column = report->uctokenptr - report->uclinebuf;

    errorObject->Init
         (reinterpret_cast<const PRUnichar*>(report->ucmessage),
          NS_ConvertUTF8toUTF16(report->filename).get(),
          reinterpret_cast<const PRUnichar*>(report->uclinebuf),
          report->lineno, column, report->flags,
          "xbl javascript"
          );
    consoleService->LogMessage(errorObject);
  }
}






void
nsXBLDocGlobalObject::SetContext(nsIScriptContext *aScriptContext)
{
  if (!aScriptContext) {
    mScriptContext = nsnull;
    return;
  }
  NS_ASSERTION(aScriptContext->GetScriptTypeID() ==
                                        nsIProgrammingLanguage::JAVASCRIPT,
               "xbl is not multi-language");
  aScriptContext->WillInitializeContext();
  
  
  
  nsresult rv;
  rv = aScriptContext->InitContext();
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Script Language's InitContext failed");
  aScriptContext->SetGCOnDestruction(PR_FALSE);
  aScriptContext->DidInitializeContext();
  
  mScriptContext = aScriptContext;
}

nsresult
nsXBLDocGlobalObject::SetScriptContext(PRUint32 lang_id, nsIScriptContext *aContext)
{
  NS_ASSERTION(lang_id == nsIProgrammingLanguage::JAVASCRIPT, "Only JS allowed!");
  SetContext(aContext);
  return NS_OK;
}

nsIScriptContext *
nsXBLDocGlobalObject::GetScriptContext(PRUint32 language)
{
  
  NS_ENSURE_TRUE(language==nsIProgrammingLanguage::JAVASCRIPT, nsnull);
  return GetContext();
}

void *
nsXBLDocGlobalObject::GetScriptGlobal(PRUint32 language)
{
  
  NS_ENSURE_TRUE(language==nsIProgrammingLanguage::JAVASCRIPT, nsnull);
  return GetGlobalJSObject();
}

nsresult
nsXBLDocGlobalObject::EnsureScriptEnvironment(PRUint32 aLangID)
{
  if (aLangID != nsIProgrammingLanguage::JAVASCRIPT) {
    NS_WARNING("XBL still JS only");
    return NS_ERROR_INVALID_ARG;
  }
  if (mScriptContext)
    return NS_OK; 
  nsCOMPtr<nsIDOMScriptObjectFactory> factory = do_GetService(kDOMScriptObjectFactoryCID);
  NS_ENSURE_TRUE(factory, nsnull);

  nsresult rv;

  nsCOMPtr<nsIScriptRuntime> scriptRuntime;
  rv = NS_GetScriptRuntimeByID(aLangID, getter_AddRefs(scriptRuntime));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIScriptContext> newCtx;
  rv = scriptRuntime->CreateContext(getter_AddRefs(newCtx));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = SetScriptContext(aLangID, newCtx);

  JSContext *cx = (JSContext *)mScriptContext->GetNativeContext();
  JSAutoRequest ar(cx);

  
  
  
  JS_SetErrorReporter(cx, XBL_ProtoErrorReporter);

  nsIPrincipal *principal = GetPrincipal();
  JSCompartment *compartment;

  rv = xpc_CreateGlobalObject(cx, &gSharedGlobalClass, principal, nsnull,
                              false, &mJSObject, &compartment);
  NS_ENSURE_SUCCESS(rv, nsnull);

  ::JS_SetGlobalObject(cx, mJSObject);

  
  
  ::JS_SetPrivate(cx, mJSObject, this);
  NS_ADDREF(this);
  return NS_OK;
}

nsIScriptContext *
nsXBLDocGlobalObject::GetContext()
{
  
  
  if (! mScriptContext) {
    nsresult rv = EnsureScriptEnvironment(nsIProgrammingLanguage::JAVASCRIPT);
    
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to setup JS!?");
    NS_ENSURE_SUCCESS(rv, nsnull);
    NS_ASSERTION(mScriptContext, "Failed to find a script context!?");
  }
  return mScriptContext;
}

void
nsXBLDocGlobalObject::ClearGlobalObjectOwner()
{
  mGlobalObjectOwner = nsnull;
}

JSObject *
nsXBLDocGlobalObject::GetGlobalJSObject()
{
  
  

  if (!mScriptContext)
    return nsnull;

  JSContext* cx = static_cast<JSContext*>
                             (mScriptContext->GetNativeContext());
  if (!cx)
    return nsnull;

  JSObject *ret = ::JS_GetGlobalObject(cx);
  NS_ASSERTION(mJSObject == ret, "How did this magic switch happen?");
  return ret;
}

void
nsXBLDocGlobalObject::OnFinalize(PRUint32 aLangID, void *aObject)
{
  NS_ASSERTION(aLangID == nsIProgrammingLanguage::JAVASCRIPT,
               "Only JS supported");
  NS_ASSERTION(aObject == mJSObject, "Wrong object finalized!");

  mJSObject = nsnull;
}

void
nsXBLDocGlobalObject::SetScriptsEnabled(PRBool aEnabled, PRBool aFireTimeouts)
{
    
}






nsIPrincipal*
nsXBLDocGlobalObject::GetPrincipal()
{
  if (!mGlobalObjectOwner) {
    
    
    return nsnull;
  }

  nsRefPtr<nsXBLDocumentInfo> docInfo =
    static_cast<nsXBLDocumentInfo*>(mGlobalObjectOwner);

  nsCOMPtr<nsIDocument> document = docInfo->GetDocument();
  if (!document)
    return NULL;

  return document->NodePrincipal();
}

static PRBool IsChromeURI(nsIURI* aURI)
{
  PRBool isChrome = PR_FALSE;
  if (NS_SUCCEEDED(aURI->SchemeIs("chrome", &isChrome)))
      return isChrome;
  return PR_FALSE;
}



static PRIntn
TraverseProtos(nsHashKey *aKey, void *aData, void* aClosure)
{
  nsCycleCollectionTraversalCallback *cb = 
    static_cast<nsCycleCollectionTraversalCallback*>(aClosure);
  nsXBLPrototypeBinding *proto = static_cast<nsXBLPrototypeBinding*>(aData);
  proto->Traverse(*cb);
  return kHashEnumerateNext;
}

static PRIntn
UnlinkProtoJSObjects(nsHashKey *aKey, void *aData, void* aClosure)
{
  nsXBLPrototypeBinding *proto = static_cast<nsXBLPrototypeBinding*>(aData);
  proto->UnlinkJSObjects();
  return kHashEnumerateNext;
}

struct ProtoTracer
{
  TraceCallback mCallback;
  void *mClosure;
};

static PRIntn
TraceProtos(nsHashKey *aKey, void *aData, void* aClosure)
{
  ProtoTracer* closure = static_cast<ProtoTracer*>(aClosure);
  nsXBLPrototypeBinding *proto = static_cast<nsXBLPrototypeBinding*>(aData);
  proto->Trace(closure->mCallback, closure->mClosure);
  return kHashEnumerateNext;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXBLDocumentInfo)
NS_IMPL_CYCLE_COLLECTION_ROOT_BEGIN(nsXBLDocumentInfo)
  if (tmp->mBindingTable) {
    tmp->mBindingTable->Enumerate(UnlinkProtoJSObjects, nsnull);
  }
NS_IMPL_CYCLE_COLLECTION_ROOT_END
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsXBLDocumentInfo)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mDocument)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mGlobalObject)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsXBLDocumentInfo)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDocument)
  if (tmp->mBindingTable) {
    tmp->mBindingTable->Enumerate(TraverseProtos, &cb);
  }
  cb.NoteXPCOMChild(static_cast<nsIScriptGlobalObject*>(tmp->mGlobalObject));
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsXBLDocumentInfo)
  if (tmp->mBindingTable) {
    ProtoTracer closure = { aCallback, aClosure };
    tmp->mBindingTable->Enumerate(TraceProtos, &closure);
  }
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsXBLDocumentInfo)
  NS_INTERFACE_MAP_ENTRY(nsIScriptGlobalObjectOwner)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIScriptGlobalObjectOwner)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsXBLDocumentInfo, nsIScriptGlobalObjectOwner)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(nsXBLDocumentInfo,
                                           nsIScriptGlobalObjectOwner)

nsXBLDocumentInfo::nsXBLDocumentInfo(nsIDocument* aDocument)
  : mDocument(aDocument),
    mScriptAccess(PR_TRUE),
    mIsChrome(PR_FALSE),
    mBindingTable(nsnull),
    mFirstBinding(nsnull)
{
  nsIURI* uri = aDocument->GetDocumentURI();
  if (IsChromeURI(uri)) {
    
    nsCOMPtr<nsIXULChromeRegistry> reg =
      mozilla::services::GetXULChromeRegistryService();
    if (reg) {
      PRBool allow = PR_TRUE;
      reg->AllowScriptsForPackage(uri, &allow);
      mScriptAccess = allow;
    }
    mIsChrome = PR_TRUE;
  }
}

nsXBLDocumentInfo::~nsXBLDocumentInfo()
{
  
  if (mGlobalObject) {
    
    mGlobalObject->SetScriptContext(nsIProgrammingLanguage::JAVASCRIPT, nsnull);
    mGlobalObject->ClearGlobalObjectOwner(); 
  }
  if (mBindingTable) {
    NS_DROP_JS_OBJECTS(this, nsXBLDocumentInfo);
    delete mBindingTable;
  }
}

nsXBLPrototypeBinding*
nsXBLDocumentInfo::GetPrototypeBinding(const nsACString& aRef)
{
  if (!mBindingTable)
    return NULL;

  if (aRef.IsEmpty()) {
    
    return mFirstBinding;
  }

  const nsPromiseFlatCString& flat = PromiseFlatCString(aRef);
  nsCStringKey key(flat.get());
  return static_cast<nsXBLPrototypeBinding*>(mBindingTable->Get(&key));
}

static PRBool
DeletePrototypeBinding(nsHashKey* aKey, void* aData, void* aClosure)
{
  nsXBLPrototypeBinding* binding = static_cast<nsXBLPrototypeBinding*>(aData);
  delete binding;
  return PR_TRUE;
}

nsresult
nsXBLDocumentInfo::SetPrototypeBinding(const nsACString& aRef, nsXBLPrototypeBinding* aBinding)
{
  if (!mBindingTable) {
    mBindingTable = new nsObjectHashtable(nsnull, nsnull, DeletePrototypeBinding, nsnull);

    NS_HOLD_JS_OBJECTS(this, nsXBLDocumentInfo);
  }

  const nsPromiseFlatCString& flat = PromiseFlatCString(aRef);
  nsCStringKey key(flat.get());
  NS_ENSURE_STATE(!mBindingTable->Get(&key));
  mBindingTable->Put(&key, aBinding);

  return NS_OK;
}

void
nsXBLDocumentInfo::SetFirstPrototypeBinding(nsXBLPrototypeBinding* aBinding)
{
  mFirstBinding = aBinding;
}

PRBool FlushScopedSkinSheets(nsHashKey* aKey, void* aData, void* aClosure)
{
  nsXBLPrototypeBinding* proto = (nsXBLPrototypeBinding*)aData;
  proto->FlushSkinSheets();
  return PR_TRUE;
}

void
nsXBLDocumentInfo::FlushSkinStylesheets()
{
  if (mBindingTable)
    mBindingTable->Enumerate(FlushScopedSkinSheets);
}






nsIScriptGlobalObject*
nsXBLDocumentInfo::GetScriptGlobalObject()
{
  if (!mGlobalObject) {
    nsXBLDocGlobalObject *global = new nsXBLDocGlobalObject(this);
    if (!global)
      return nsnull;

    mGlobalObject = global;
  }

  return mGlobalObject;
}

nsXBLDocumentInfo* NS_NewXBLDocumentInfo(nsIDocument* aDocument)
{
  NS_PRECONDITION(aDocument, "Must have a document!");

  nsXBLDocumentInfo* result;

  result = new nsXBLDocumentInfo(aDocument);
  NS_ADDREF(result);
  return result;
}
