





#include "mozilla/DebugOnly.h"

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
#include "nsJSPrincipals.h"
#include "nsIScriptSecurityManager.h"
#include "nsContentUtils.h"
#include "nsDOMJSUtils.h"
#include "mozilla/Services.h"
#include "xpcpublic.h"
#include "mozilla/scache/StartupCache.h"
#include "mozilla/scache/StartupCacheUtils.h"
#include "nsCCUncollectableMarker.h"
#include "mozilla/dom/BindingUtils.h"

using namespace mozilla::scache;
using namespace mozilla;

static const char kXBLCachePrefix[] = "xblcache";

static NS_DEFINE_CID(kDOMScriptObjectFactoryCID, NS_DOM_SCRIPT_OBJECT_FACTORY_CID);



class nsXBLDocGlobalObject : public nsIScriptGlobalObject,
                             public nsIScriptObjectPrincipal
{
public:
  nsXBLDocGlobalObject(nsXBLDocumentInfo *aGlobalObjectOwner);

  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  
  
  virtual nsresult EnsureScriptEnvironment();
  void ClearScriptContext()
  {
    mScriptContext = NULL;
  }

  virtual nsIScriptContext *GetContext();
  virtual JSObject *GetGlobalJSObject();
  virtual void OnFinalize(JSObject* aObject);
  virtual void SetScriptsEnabled(bool aEnabled, bool aFireTimeouts);

  
  virtual nsIPrincipal* GetPrincipal();

  static JSBool doCheckAccess(JSContext *cx, JSObject *obj, jsid id,
                              uint32_t accessType);

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsXBLDocGlobalObject,
                                           nsIScriptGlobalObject)

  void ClearGlobalObjectOwner();

  void UnmarkScriptContext();

protected:
  virtual ~nsXBLDocGlobalObject();

  nsIScriptContext *GetScriptContext();

  nsCOMPtr<nsIScriptContext> mScriptContext;
  JSObject *mJSObject;

  nsXBLDocumentInfo* mGlobalObjectOwner; 
  static JSClass gSharedGlobalClass;
};

JSBool
nsXBLDocGlobalObject::doCheckAccess(JSContext *cx, JSObject *obj, jsid id, uint32_t accessType)
{
  nsIScriptSecurityManager *ssm = nsContentUtils::GetSecurityManager();
  if (!ssm) {
    ::JS_ReportError(cx, "Unable to verify access to a global object property.");
    return JS_FALSE;
  }

  
  
  while (JS_GetClass(obj) != &nsXBLDocGlobalObject::gSharedGlobalClass) {
    if (!::JS_GetPrototype(cx, obj, &obj)) {
      return JS_FALSE;
    }
    if (!obj) {
      ::JS_ReportError(cx, "Invalid access to a global object property.");
      return JS_FALSE;
    }
  }

  nsresult rv = ssm->CheckPropertyAccess(cx, obj, JS_GetClass(obj)->name,
                                         id, accessType);
  return NS_SUCCEEDED(rv);
}

static JSBool
nsXBLDocGlobalObject_getProperty(JSContext *cx, JSHandleObject obj,
                                 JSHandleId id, JSMutableHandleValue vp)
{
  return nsXBLDocGlobalObject::
    doCheckAccess(cx, obj, id, nsIXPCSecurityManager::ACCESS_GET_PROPERTY);
}

static JSBool
nsXBLDocGlobalObject_setProperty(JSContext *cx, JSHandleObject obj,
                                 JSHandleId id, JSBool strict, JSMutableHandleValue vp)
{
  return nsXBLDocGlobalObject::
    doCheckAccess(cx, obj, id, nsIXPCSecurityManager::ACCESS_SET_PROPERTY);
}

static JSBool
nsXBLDocGlobalObject_checkAccess(JSContext *cx, JSHandleObject obj, JSHandleId id,
                                 JSAccessMode mode, JSMutableHandleValue vp)
{
  uint32_t translated;
  if (mode & JSACC_WRITE) {
    translated = nsIXPCSecurityManager::ACCESS_SET_PROPERTY;
  } else {
    translated = nsIXPCSecurityManager::ACCESS_GET_PROPERTY;
  }

  return nsXBLDocGlobalObject::
    doCheckAccess(cx, obj, id, translated);
}

static void
nsXBLDocGlobalObject_finalize(JSFreeOp *fop, JSObject *obj)
{
  nsISupports *nativeThis = (nsISupports*)JS_GetPrivate(obj);

  nsCOMPtr<nsIScriptGlobalObject> sgo(do_QueryInterface(nativeThis));

  if (sgo)
    sgo->OnFinalize(obj);

  
  NS_RELEASE(nativeThis);
}

static JSBool
nsXBLDocGlobalObject_resolve(JSContext *cx, JSHandleObject obj, JSHandleId id)
{
  JSBool did_resolve = JS_FALSE;
  return JS_ResolveStandardClass(cx, obj, id, &did_resolve);
}


JSClass nsXBLDocGlobalObject::gSharedGlobalClass = {
    "nsXBLPrototypeScript compilation scope",
    JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS |
    JSCLASS_IMPLEMENTS_BARRIERS | JSCLASS_GLOBAL_FLAGS_WITH_SLOTS(0),
    JS_PropertyStub,  JS_PropertyStub,
    nsXBLDocGlobalObject_getProperty, nsXBLDocGlobalObject_setProperty,
    JS_EnumerateStub, nsXBLDocGlobalObject_resolve,
    JS_ConvertStub, nsXBLDocGlobalObject_finalize,
    nsXBLDocGlobalObject_checkAccess, NULL, NULL, NULL,
    NULL
};






nsXBLDocGlobalObject::nsXBLDocGlobalObject(nsXBLDocumentInfo *aGlobalObjectOwner)
    : mJSObject(nullptr),
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

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsXBLDocGlobalObject)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsXBLDocGlobalObject)

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
    uint32_t column = report->uctokenptr - report->uclinebuf;

    const PRUnichar* ucmessage =
      static_cast<const PRUnichar*>(report->ucmessage);
    const PRUnichar* uclinebuf =
      static_cast<const PRUnichar*>(report->uclinebuf);

    errorObject->Init
         (ucmessage ? nsDependentString(ucmessage) : EmptyString(),
          NS_ConvertUTF8toUTF16(report->filename),
          uclinebuf ? nsDependentString(uclinebuf) : EmptyString(),
          report->lineno, column, report->flags,
          "xbl javascript"
          );
    consoleService->LogMessage(errorObject);
  }
}






nsIScriptContext *
nsXBLDocGlobalObject::GetScriptContext()
{
  return GetContext();
}

nsresult
nsXBLDocGlobalObject::EnsureScriptEnvironment()
{
  if (mScriptContext) {
    
    return NS_OK;
  }

  nsCOMPtr<nsIScriptRuntime> scriptRuntime;
  NS_GetJSRuntime(getter_AddRefs(scriptRuntime));
  NS_ENSURE_TRUE(scriptRuntime, NS_OK);

  nsCOMPtr<nsIScriptContext> newCtx = scriptRuntime->CreateContext(false, nullptr);
  MOZ_ASSERT(newCtx);

  newCtx->WillInitializeContext();
  
  
  
  DebugOnly<nsresult> rv = newCtx->InitContext();
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Script Language's InitContext failed");
  newCtx->DidInitializeContext();

  mScriptContext = newCtx;

  JSContext *cx = mScriptContext->GetNativeContext();
  JSAutoRequest ar(cx);

  
  
  
  JS_SetErrorReporter(cx, XBL_ProtoErrorReporter);

  mJSObject = JS_NewGlobalObject(cx, &gSharedGlobalClass,
                                 nsJSPrincipals::get(GetPrincipal()),
                                 JS::SystemZone);
  if (!mJSObject)
      return NS_OK;

  
  
  nsIURI *ownerURI = mGlobalObjectOwner->DocumentURI();
  xpc::SetLocationForGlobal(mJSObject, ownerURI);

  ::JS_SetGlobalObject(cx, mJSObject);

  
  
  ::JS_SetPrivate(mJSObject, this);
  NS_ADDREF(this);
  return NS_OK;
}

nsIScriptContext *
nsXBLDocGlobalObject::GetContext()
{
  
  
  if (! mScriptContext) {
    nsresult rv = EnsureScriptEnvironment();
    
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to setup JS!?");
    NS_ENSURE_SUCCESS(rv, nullptr);
    NS_ASSERTION(mScriptContext, "Failed to find a script context!?");
  }
  return mScriptContext;
}

void
nsXBLDocGlobalObject::ClearGlobalObjectOwner()
{
  mGlobalObjectOwner = nullptr;
}

void
nsXBLDocGlobalObject::UnmarkScriptContext()
{
  if (mScriptContext) {
    xpc_UnmarkGrayObject(mScriptContext->GetNativeGlobal());
  }
}

JSObject *
nsXBLDocGlobalObject::GetGlobalJSObject()
{
  
  

  if (!mScriptContext)
    return nullptr;

  JSContext* cx = mScriptContext->GetNativeContext();
  if (!cx)
    return nullptr;

  JSObject *ret = ::JS_GetGlobalObject(cx);
  NS_ASSERTION(mJSObject == ret, "How did this magic switch happen?");
  return ret;
}

void
nsXBLDocGlobalObject::OnFinalize(JSObject* aObject)
{
  NS_ASSERTION(aObject == mJSObject, "Wrong object finalized!");
  mJSObject = NULL;
}

void
nsXBLDocGlobalObject::SetScriptsEnabled(bool aEnabled, bool aFireTimeouts)
{
    
}






nsIPrincipal*
nsXBLDocGlobalObject::GetPrincipal()
{
  if (!mGlobalObjectOwner) {
    
    
    return nullptr;
  }

  nsRefPtr<nsXBLDocumentInfo> docInfo =
    static_cast<nsXBLDocumentInfo*>(mGlobalObjectOwner);

  nsCOMPtr<nsIDocument> document = docInfo->GetDocument();
  if (!document)
    return NULL;

  return document->NodePrincipal();
}

static bool IsChromeURI(nsIURI* aURI)
{
  bool isChrome = false;
  if (NS_SUCCEEDED(aURI->SchemeIs("chrome", &isChrome)))
      return isChrome;
  return false;
}



static bool
TraverseProtos(nsHashKey *aKey, void *aData, void* aClosure)
{
  nsCycleCollectionTraversalCallback *cb = 
    static_cast<nsCycleCollectionTraversalCallback*>(aClosure);
  nsXBLPrototypeBinding *proto = static_cast<nsXBLPrototypeBinding*>(aData);
  proto->Traverse(*cb);
  return kHashEnumerateNext;
}

static bool
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

static bool
TraceProtos(nsHashKey *aKey, void *aData, void* aClosure)
{
  ProtoTracer* closure = static_cast<ProtoTracer*>(aClosure);
  nsXBLPrototypeBinding *proto = static_cast<nsXBLPrototypeBinding*>(aData);
  proto->Trace(closure->mCallback, closure->mClosure);
  return kHashEnumerateNext;
}

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsXBLDocumentInfo)
  if (tmp->mBindingTable) {
    tmp->mBindingTable->Enumerate(UnlinkProtoJSObjects, nullptr);
  }
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mDocument)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mGlobalObject)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsXBLDocumentInfo)
  if (tmp->mDocument &&
      nsCCUncollectableMarker::InGeneration(cb, tmp->mDocument->GetMarkedCCGeneration())) {
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
    return NS_SUCCESS_INTERRUPTED_TRAVERSE;
  }
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDocument)
  if (tmp->mBindingTable) {
    tmp->mBindingTable->Enumerate(TraverseProtos, &cb);
  }
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mGlobalObject");
  cb.NoteXPCOMChild(static_cast<nsIScriptGlobalObject*>(tmp->mGlobalObject));
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsXBLDocumentInfo)
  if (tmp->mBindingTable) {
    ProtoTracer closure = { aCallback, aClosure };
    tmp->mBindingTable->Enumerate(TraceProtos, &closure);
  }
NS_IMPL_CYCLE_COLLECTION_TRACE_END

static void
UnmarkXBLJSObject(void* aP, const char* aName, void* aClosure)
{
  xpc_UnmarkGrayObject(static_cast<JSObject*>(aP));
}

static bool
UnmarkProtos(nsHashKey* aKey, void* aData, void* aClosure)
{
  nsXBLPrototypeBinding* proto = static_cast<nsXBLPrototypeBinding*>(aData);
  proto->Trace(UnmarkXBLJSObject, nullptr);
  return kHashEnumerateNext;
}

void
nsXBLDocumentInfo::MarkInCCGeneration(uint32_t aGeneration)
{
  if (mDocument) {
    mDocument->MarkUncollectableForCCGeneration(aGeneration);
  }
  
  if (mBindingTable) {
    mBindingTable->Enumerate(UnmarkProtos, nullptr);
  }
  if (mGlobalObject) {
    mGlobalObject->UnmarkScriptContext();
  }
}

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsXBLDocumentInfo)
  NS_INTERFACE_MAP_ENTRY(nsIScriptGlobalObjectOwner)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIScriptGlobalObjectOwner)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsXBLDocumentInfo)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsXBLDocumentInfo)

nsXBLDocumentInfo::nsXBLDocumentInfo(nsIDocument* aDocument)
  : mDocument(aDocument),
    mScriptAccess(true),
    mIsChrome(false),
    mBindingTable(nullptr),
    mFirstBinding(nullptr)
{
  nsIURI* uri = aDocument->GetDocumentURI();
  if (IsChromeURI(uri)) {
    
    nsCOMPtr<nsIXULChromeRegistry> reg =
      mozilla::services::GetXULChromeRegistryService();
    if (reg) {
      bool allow = true;
      reg->AllowScriptsForPackage(uri, &allow);
      mScriptAccess = allow;
    }
    mIsChrome = true;
  }
}

nsXBLDocumentInfo::~nsXBLDocumentInfo()
{
  
  if (mGlobalObject) {
    
    mGlobalObject->ClearScriptContext();
    mGlobalObject->ClearGlobalObjectOwner(); 
  }
  if (mBindingTable) {
    delete mBindingTable;
    mBindingTable = nullptr;
    NS_DROP_JS_OBJECTS(this, nsXBLDocumentInfo);
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

static bool
DeletePrototypeBinding(nsHashKey* aKey, void* aData, void* aClosure)
{
  nsXBLPrototypeBinding* binding = static_cast<nsXBLPrototypeBinding*>(aData);
  delete binding;
  return true;
}

nsresult
nsXBLDocumentInfo::SetPrototypeBinding(const nsACString& aRef, nsXBLPrototypeBinding* aBinding)
{
  if (!mBindingTable) {
    mBindingTable = new nsObjectHashtable(nullptr, nullptr, DeletePrototypeBinding, nullptr);

    NS_HOLD_JS_OBJECTS(this, nsXBLDocumentInfo);
  }

  const nsPromiseFlatCString& flat = PromiseFlatCString(aRef);
  nsCStringKey key(flat.get());
  NS_ENSURE_STATE(!mBindingTable->Get(&key));
  mBindingTable->Put(&key, aBinding);

  return NS_OK;
}

void
nsXBLDocumentInfo::RemovePrototypeBinding(const nsACString& aRef)
{
  if (mBindingTable) {
    
    const nsPromiseFlatCString& flat = PromiseFlatCString(aRef);
    nsCStringKey key(flat);
    mBindingTable->Remove(&key);
  }
}



bool
WriteBinding(nsHashKey *aKey, void *aData, void* aClosure)
{
  nsXBLPrototypeBinding* binding = static_cast<nsXBLPrototypeBinding *>(aData);
  binding->Write((nsIObjectOutputStream*)aClosure);

  return kHashEnumerateNext;
}


nsresult
nsXBLDocumentInfo::ReadPrototypeBindings(nsIURI* aURI, nsXBLDocumentInfo** aDocInfo)
{
  *aDocInfo = nullptr;

  nsAutoCString spec(kXBLCachePrefix);
  nsresult rv = PathifyURI(aURI, spec);
  NS_ENSURE_SUCCESS(rv, rv);

  StartupCache* startupCache = StartupCache::GetSingleton();
  NS_ENSURE_TRUE(startupCache, NS_ERROR_FAILURE);

  nsAutoArrayPtr<char> buf;
  uint32_t len;
  rv = startupCache->GetBuffer(spec.get(), getter_Transfers(buf), &len);
  
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIObjectInputStream> stream;
  rv = NewObjectInputStreamFromBuffer(buf, len, getter_AddRefs(stream));
  NS_ENSURE_SUCCESS(rv, rv);
  buf.forget();

  
  
  
  uint32_t version;
  rv = stream->Read32(&version);
  NS_ENSURE_SUCCESS(rv, rv);
  if (version != XBLBinding_Serialize_Version) {
    
    
    startupCache->InvalidateCache();
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsCOMPtr<nsIPrincipal> principal;
  nsContentUtils::GetSecurityManager()->
    GetSystemPrincipal(getter_AddRefs(principal));

  nsCOMPtr<nsIDOMDocument> domdoc;
  rv = NS_NewXBLDocument(getter_AddRefs(domdoc), aURI, nullptr, principal);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domdoc);
  NS_ASSERTION(doc, "Must have a document!");
  nsRefPtr<nsXBLDocumentInfo> docInfo = new nsXBLDocumentInfo(doc);

  while (1) {
    uint8_t flags;
    nsresult rv = stream->Read8(&flags);
    NS_ENSURE_SUCCESS(rv, rv);
    if (flags == XBLBinding_Serialize_NoMoreBindings)
      break;

    nsXBLPrototypeBinding* binding = new nsXBLPrototypeBinding();
    rv = binding->Read(stream, docInfo, doc, flags);
    if (NS_FAILED(rv)) {
      delete binding;
      return rv;
    }
  }

  docInfo.swap(*aDocInfo);
  return NS_OK;
}

nsresult
nsXBLDocumentInfo::WritePrototypeBindings()
{
  
  if (!nsContentUtils::IsSystemPrincipal(mDocument->NodePrincipal()))
    return NS_OK;

  nsAutoCString spec(kXBLCachePrefix);
  nsresult rv = PathifyURI(DocumentURI(), spec);
  NS_ENSURE_SUCCESS(rv, rv);

  StartupCache* startupCache = StartupCache::GetSingleton();
  NS_ENSURE_TRUE(startupCache, rv);

  nsCOMPtr<nsIObjectOutputStream> stream;
  nsCOMPtr<nsIStorageStream> storageStream;
  rv = NewObjectOutputWrappedStorageStream(getter_AddRefs(stream),
                                           getter_AddRefs(storageStream),
                                           true);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stream->Write32(XBLBinding_Serialize_Version);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mBindingTable)
    mBindingTable->Enumerate(WriteBinding, stream);

  
  rv = stream->Write8(XBLBinding_Serialize_NoMoreBindings);
  NS_ENSURE_SUCCESS(rv, rv);

  stream->Close();
  NS_ENSURE_SUCCESS(rv, rv);

  uint32_t len;
  nsAutoArrayPtr<char> buf;
  rv = NewBufferFromStorageStream(storageStream, getter_Transfers(buf), &len);
  NS_ENSURE_SUCCESS(rv, rv);

  return startupCache->PutBuffer(spec.get(), buf, len);
}

void
nsXBLDocumentInfo::SetFirstPrototypeBinding(nsXBLPrototypeBinding* aBinding)
{
  mFirstBinding = aBinding;
}

bool FlushScopedSkinSheets(nsHashKey* aKey, void* aData, void* aClosure)
{
  nsXBLPrototypeBinding* proto = (nsXBLPrototypeBinding*)aData;
  proto->FlushSkinSheets();
  return true;
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
      return nullptr;

    mGlobalObject = global;
  }

  return mGlobalObject;
}
