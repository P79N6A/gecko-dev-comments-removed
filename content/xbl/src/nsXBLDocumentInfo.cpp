





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
#include "jsfriendapi.h"
#include "nsIURI.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "nsIChromeRegistry.h"
#include "nsIPrincipal.h"
#include "nsJSPrincipals.h"
#include "nsIScriptSecurityManager.h"
#include "nsContentUtils.h"
#include "nsCxPusher.h"
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

class nsXBLDocGlobalObject : public nsISupports
{
public:
  nsXBLDocGlobalObject(nsXBLDocumentInfo *aGlobalObjectOwner);

  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsXBLDocGlobalObject)

  JSObject *GetCompilationGlobal();
  void UnmarkCompilationGlobal();
  void Destroy();
  nsIPrincipal* GetPrincipal();
  nsresult EnsureScriptEnvironment();

  static bool doCheckAccess(JSContext *cx, JS::Handle<JSObject*> obj,
                            JS::Handle<jsid> id, uint32_t accessType);

  void ClearGlobalObjectOwner();

  static JSClass gSharedGlobalClass;

protected:
  virtual ~nsXBLDocGlobalObject();

  JS::Heap<JSObject*> mJSObject;
  nsXBLDocumentInfo* mGlobalObjectOwner; 
  bool mDestroyed; 
};

bool
nsXBLDocGlobalObject::doCheckAccess(JSContext *cx, JS::Handle<JSObject*> obj,
                                    JS::Handle<jsid> id, uint32_t accessType)
{
  nsIScriptSecurityManager *ssm = nsContentUtils::GetSecurityManager();
  if (!ssm) {
    ::JS_ReportError(cx, "Unable to verify access to a global object property.");
    return false;
  }

  
  
  JS::Rooted<JSObject*> base(cx, obj);
  while (JS_GetClass(base) != &nsXBLDocGlobalObject::gSharedGlobalClass) {
    if (!::JS_GetPrototype(cx, base, &base)) {
      return false;
    }
    if (!base) {
      ::JS_ReportError(cx, "Invalid access to a global object property.");
      return false;
    }
  }

  nsresult rv = ssm->CheckPropertyAccess(cx, base, JS_GetClass(base)->name,
                                         id, accessType);
  return NS_SUCCEEDED(rv);
}

static bool
nsXBLDocGlobalObject_getProperty(JSContext *cx, JS::Handle<JSObject*> obj,
                                 JS::Handle<jsid> id, JS::MutableHandle<JS::Value> vp)
{
  return nsXBLDocGlobalObject::
    doCheckAccess(cx, obj, id, nsIXPCSecurityManager::ACCESS_GET_PROPERTY);
}

static bool
nsXBLDocGlobalObject_setProperty(JSContext *cx, JS::Handle<JSObject*> obj,
                                 JS::Handle<jsid> id, bool strict, JS::MutableHandle<JS::Value> vp)
{
  return nsXBLDocGlobalObject::
    doCheckAccess(cx, obj, id, nsIXPCSecurityManager::ACCESS_SET_PROPERTY);
}

static bool
nsXBLDocGlobalObject_checkAccess(JSContext *cx, JS::Handle<JSObject*> obj, JS::Handle<jsid> id,
                                 JSAccessMode mode, JS::MutableHandle<JS::Value> vp)
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
  nsXBLDocGlobalObject* dgo = static_cast<nsXBLDocGlobalObject*>(nativeThis);

  if (dgo)
    dgo->Destroy();

  
  
  nsContentUtils::DeferredFinalize(nativeThis);
}

static bool
nsXBLDocGlobalObject_resolve(JSContext *cx, JS::Handle<JSObject*> obj, JS::Handle<jsid> id)
{
  bool did_resolve = false;
  return JS_ResolveStandardClass(cx, obj, id, &did_resolve);
}


JSClass nsXBLDocGlobalObject::gSharedGlobalClass = {
    "nsXBLPrototypeScript compilation scope",
    JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS |
    JSCLASS_IMPLEMENTS_BARRIERS | JSCLASS_GLOBAL_FLAGS_WITH_SLOTS(0),
    JS_PropertyStub,  JS_DeletePropertyStub,
    nsXBLDocGlobalObject_getProperty, nsXBLDocGlobalObject_setProperty,
    JS_EnumerateStub, nsXBLDocGlobalObject_resolve,
    JS_ConvertStub, nsXBLDocGlobalObject_finalize,
    nsXBLDocGlobalObject_checkAccess, nullptr, nullptr, nullptr,
    nullptr
};






nsXBLDocGlobalObject::nsXBLDocGlobalObject(nsXBLDocumentInfo *aGlobalObjectOwner)
    : mJSObject(nullptr)
    , mGlobalObjectOwner(aGlobalObjectOwner) 
    , mDestroyed(false)

{
}


nsXBLDocGlobalObject::~nsXBLDocGlobalObject()
{
  MOZ_ASSERT(!mJSObject);
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXBLDocGlobalObject)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsXBLDocGlobalObject)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsXBLDocGlobalObject)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mJSObject)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsXBLDocGlobalObject)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsXBLDocGlobalObject)
  tmp->Destroy();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END


NS_IMPL_CYCLE_COLLECTING_ADDREF(nsXBLDocGlobalObject)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsXBLDocGlobalObject)

nsresult
nsXBLDocGlobalObject::EnsureScriptEnvironment()
{
  if (mJSObject || mDestroyed) {
    
    return NS_OK;
  }

  AutoSafeJSContext cx;
  JS::CompartmentOptions options;
  options.setZone(JS::SystemZone)
         .setInvisibleToDebugger(true);
  mJSObject = JS_NewGlobalObject(cx, &gSharedGlobalClass,
                                 nsJSPrincipals::get(GetPrincipal()),
                                 JS::DontFireOnNewGlobalHook,
                                 options);
  if (!mJSObject)
      return NS_OK;

  NS_HOLD_JS_OBJECTS(this, nsXBLDocGlobalObject);

  
  
  nsIURI *ownerURI = mGlobalObjectOwner->DocumentURI();
  xpc::SetLocationForGlobal(mJSObject, ownerURI);

  
  
  ::JS_SetPrivate(mJSObject, this);
  NS_ADDREF(this);
  return NS_OK;
}

void
nsXBLDocGlobalObject::ClearGlobalObjectOwner()
{
  mGlobalObjectOwner = nullptr;
}

void
nsXBLDocGlobalObject::UnmarkCompilationGlobal()
{
  xpc_UnmarkGrayObject(mJSObject);
}

JSObject *
nsXBLDocGlobalObject::GetCompilationGlobal()
{
  
  
  EnsureScriptEnvironment();
  return mJSObject;
}

void
nsXBLDocGlobalObject::Destroy()
{
  
  mDestroyed = true;
  if (!mJSObject)
    return;
  mJSObject = nullptr;
  NS_DROP_JS_OBJECTS(this, nsXBLDocGlobalObject);
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
    return nullptr;

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
  const TraceCallbacks &mCallbacks;
  void *mClosure;
};

static bool
TraceProtos(nsHashKey *aKey, void *aData, void* aClosure)
{
  ProtoTracer* closure = static_cast<ProtoTracer*>(aClosure);
  nsXBLPrototypeBinding *proto = static_cast<nsXBLPrototypeBinding*>(aData);
  proto->Trace(closure->mCallbacks, closure->mClosure);
  return kHashEnumerateNext;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXBLDocumentInfo)

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
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mGlobalObject)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsXBLDocumentInfo)
  if (tmp->mBindingTable) {
    ProtoTracer closure = { aCallbacks, aClosure };
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
  proto->Trace(TraceCallbackFunc(UnmarkXBLJSObject), nullptr);
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
    mGlobalObject->UnmarkCompilationGlobal();
  }
}

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsXBLDocumentInfo)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
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
    return nullptr;

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

JSObject*
nsXBLDocumentInfo::GetCompilationGlobal()
{
  EnsureGlobalObject();
  return mGlobalObject->GetCompilationGlobal();
}

void
nsXBLDocumentInfo::EnsureGlobalObject()
{
  if (!mGlobalObject) {
    mGlobalObject = new nsXBLDocGlobalObject(this);
  }
}

#ifdef DEBUG
void
AssertInCompilationScope()
{
  AutoJSContext cx;
  MOZ_ASSERT(JS_GetClass(JS::CurrentGlobalOrNull(cx)) ==
             &nsXBLDocGlobalObject::gSharedGlobalClass);
}
#endif
