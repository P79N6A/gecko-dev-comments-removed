




#include "base/basictypes.h"

#include "jsfriendapi.h"
#include "jswrapper.h"

#include "nsIInterfaceRequestorUtils.h"
#include "nsJSNPRuntime.h"
#include "nsNPAPIPlugin.h"
#include "nsNPAPIPluginInstance.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptContext.h"
#include "nsDOMJSUtils.h"
#include "nsCxPusher.h"
#include "nsIDocument.h"
#include "nsIJSRuntimeService.h"
#include "nsIXPConnect.h"
#include "nsIDOMElement.h"
#include "prmem.h"
#include "nsIContent.h"
#include "nsPluginInstanceOwner.h"
#include "nsWrapperCacheInlines.h"
#include "js/HashTable.h"
#include "mozilla/HashFunctions.h"


#define NPRUNTIME_JSCLASS_NAME "NPObject JS wrapper class"

using namespace mozilla::plugins::parent;
using namespace mozilla;

#include "mozilla/plugins/PluginScriptableObjectParent.h"
using mozilla::plugins::PluginScriptableObjectParent;
using mozilla::plugins::ParentNPObject;

struct JSObjWrapperHasher : public js::DefaultHasher<nsJSObjWrapperKey>
{
  typedef nsJSObjWrapperKey Key;
  typedef Key Lookup;

  static uint32_t hash(const Lookup &l) {
    return HashGeneric(l.mJSObj, l.mNpp);
  }

  static void rekey(Key &k, const Key& newKey) {
    MOZ_ASSERT(k.mNpp == newKey.mNpp);
    k.mJSObj = newKey.mJSObj;
  }
};








typedef js::HashMap<nsJSObjWrapperKey,
                    nsJSObjWrapper*,
                    JSObjWrapperHasher,
                    js::SystemAllocPolicy> JSObjWrapperTable;
static JSObjWrapperTable sJSObjWrappers;


static PLDHashTable sNPObjWrappers;




static int32_t sWrapperCount;



static JSRuntime *sJSRuntime;

static nsTArray<NPObject*>* sDelayedReleases;

namespace {

inline bool
NPObjectIsOutOfProcessProxy(NPObject *obj)
{
  return obj->_class == PluginScriptableObjectParent::GetClass();
}

} 




class AutoJSExceptionReporter
{
public:
  AutoJSExceptionReporter(JSContext *cx)
    : mCx(cx)
  {
  }

  ~AutoJSExceptionReporter()
  {
    JS_ReportPendingException(mCx);
  }

protected:
  JSContext *mCx;
};


NPClass nsJSObjWrapper::sJSObjWrapperNPClass =
  {
    NP_CLASS_STRUCT_VERSION,
    nsJSObjWrapper::NP_Allocate,
    nsJSObjWrapper::NP_Deallocate,
    nsJSObjWrapper::NP_Invalidate,
    nsJSObjWrapper::NP_HasMethod,
    nsJSObjWrapper::NP_Invoke,
    nsJSObjWrapper::NP_InvokeDefault,
    nsJSObjWrapper::NP_HasProperty,
    nsJSObjWrapper::NP_GetProperty,
    nsJSObjWrapper::NP_SetProperty,
    nsJSObjWrapper::NP_RemoveProperty,
    nsJSObjWrapper::NP_Enumerate,
    nsJSObjWrapper::NP_Construct
  };

static bool
NPObjWrapper_AddProperty(JSContext *cx, JS::Handle<JSObject*> obj, JS::Handle<jsid> id, JS::MutableHandle<JS::Value> vp);

static bool
NPObjWrapper_DelProperty(JSContext *cx, JS::Handle<JSObject*> obj, JS::Handle<jsid> id, bool *succeeded);

static bool
NPObjWrapper_SetProperty(JSContext *cx, JS::Handle<JSObject*> obj, JS::Handle<jsid> id, bool strict,
                         JS::MutableHandle<JS::Value> vp);

static bool
NPObjWrapper_GetProperty(JSContext *cx, JS::Handle<JSObject*> obj, JS::Handle<jsid> id, JS::MutableHandle<JS::Value> vp);

static bool
NPObjWrapper_newEnumerate(JSContext *cx, JS::Handle<JSObject*> obj, JSIterateOp enum_op,
                          JS::Value *statep, jsid *idp);

static bool
NPObjWrapper_NewResolve(JSContext *cx, JS::Handle<JSObject*> obj, JS::Handle<jsid> id, unsigned flags,
                        JS::MutableHandle<JSObject*> objp);

static bool
NPObjWrapper_Convert(JSContext *cx, JS::Handle<JSObject*> obj, JSType type, JS::MutableHandle<JS::Value> vp);

static void
NPObjWrapper_Finalize(JSFreeOp *fop, JSObject *obj);

static bool
NPObjWrapper_Call(JSContext *cx, unsigned argc, JS::Value *vp);

static bool
NPObjWrapper_Construct(JSContext *cx, unsigned argc, JS::Value *vp);

static bool
CreateNPObjectMember(NPP npp, JSContext *cx, JSObject *obj, NPObject *npobj,
                     JS::Handle<jsid> id, NPVariant* getPropertyResult, JS::Value *vp);

const JSClass sNPObjectJSWrapperClass =
  {
    NPRUNTIME_JSCLASS_NAME,
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS | JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE,
    NPObjWrapper_AddProperty,
    NPObjWrapper_DelProperty,
    NPObjWrapper_GetProperty,
    NPObjWrapper_SetProperty,
    (JSEnumerateOp)NPObjWrapper_newEnumerate,
    (JSResolveOp)NPObjWrapper_NewResolve,
    NPObjWrapper_Convert,
    NPObjWrapper_Finalize,
    nullptr,                                                
    NPObjWrapper_Call,
    nullptr,                                                
    NPObjWrapper_Construct
  };

typedef struct NPObjectMemberPrivate {
    JS::Heap<JSObject *> npobjWrapper;
    JS::Heap<JS::Value> fieldValue;
    JS::Heap<jsid> methodName;
    NPP   npp;
} NPObjectMemberPrivate;

static bool
NPObjectMember_Convert(JSContext *cx, JS::Handle<JSObject*> obj, JSType type, JS::MutableHandle<JS::Value> vp);

static void
NPObjectMember_Finalize(JSFreeOp *fop, JSObject *obj);

static bool
NPObjectMember_Call(JSContext *cx, unsigned argc, JS::Value *vp);

static void
NPObjectMember_Trace(JSTracer *trc, JSObject *obj);

static const JSClass sNPObjectMemberClass =
  {
    "NPObject Ambiguous Member class", JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS,
    JS_PropertyStub, JS_DeletePropertyStub,
    JS_PropertyStub, JS_StrictPropertyStub, JS_EnumerateStub,
    JS_ResolveStub, NPObjectMember_Convert,
    NPObjectMember_Finalize, nullptr, NPObjectMember_Call,
    nullptr, nullptr, NPObjectMember_Trace
  };

static void
OnWrapperDestroyed();

static void
DelayedReleaseGCCallback(JSGCStatus status)
{
  if (JSGC_END == status) {
    
    
    nsAutoPtr<nsTArray<NPObject*> > delayedReleases(sDelayedReleases);
    sDelayedReleases = nullptr;

    if (delayedReleases) {
      for (uint32_t i = 0; i < delayedReleases->Length(); ++i) {
        NPObject* obj = (*delayedReleases)[i];
        if (obj)
          _releaseobject(obj);
        OnWrapperDestroyed();
      }
    }
  }
}

static void
OnWrapperCreated()
{
  if (sWrapperCount++ == 0) {
    static const char rtsvc_id[] = "@mozilla.org/js/xpc/RuntimeService;1";
    nsCOMPtr<nsIJSRuntimeService> rtsvc(do_GetService(rtsvc_id));
    if (!rtsvc)
      return;

    rtsvc->GetRuntime(&sJSRuntime);
    NS_ASSERTION(sJSRuntime != nullptr, "no JSRuntime?!");

    
    
    rtsvc->RegisterGCCallback(DelayedReleaseGCCallback);
  }
}

static void
OnWrapperDestroyed()
{
  NS_ASSERTION(sWrapperCount, "Whaaa, unbalanced created/destroyed calls!");

  if (--sWrapperCount == 0) {
    if (sJSObjWrappers.initialized()) {
      MOZ_ASSERT(sJSObjWrappers.count() == 0);

      
      
      sJSObjWrappers.finish();
    }

    if (sNPObjWrappers.ops) {
      MOZ_ASSERT(sNPObjWrappers.entryCount == 0);

      
      
      PL_DHashTableFinish(&sNPObjWrappers);

      sNPObjWrappers.ops = nullptr;
    }

    
    sJSRuntime = nullptr;
  }
}

namespace mozilla {
namespace plugins {
namespace parent {

JSContext *
GetJSContext(NPP npp)
{
  NS_ENSURE_TRUE(npp, nullptr);

  nsNPAPIPluginInstance *inst = (nsNPAPIPluginInstance *)npp->ndata;
  NS_ENSURE_TRUE(inst, nullptr);

  nsRefPtr<nsPluginInstanceOwner> owner = inst->GetOwner();
  NS_ENSURE_TRUE(owner, nullptr);

  nsCOMPtr<nsIDocument> doc;
  owner->GetDocument(getter_AddRefs(doc));
  NS_ENSURE_TRUE(doc, nullptr);

  nsCOMPtr<nsISupports> documentContainer = doc->GetContainer();
  nsCOMPtr<nsIScriptGlobalObject> sgo(do_GetInterface(documentContainer));
  NS_ENSURE_TRUE(sgo, nullptr);

  nsIScriptContext *scx = sgo->GetContext();
  NS_ENSURE_TRUE(scx, nullptr);

  return scx->GetNativeContext();
}

}
}
}

static NPP
LookupNPP(NPObject *npobj);


static JS::Value
NPVariantToJSVal(NPP npp, JSContext *cx, const NPVariant *variant)
{
  switch (variant->type) {
  case NPVariantType_Void :
    return JSVAL_VOID;
  case NPVariantType_Null :
    return JSVAL_NULL;
  case NPVariantType_Bool :
    return BOOLEAN_TO_JSVAL(NPVARIANT_TO_BOOLEAN(*variant));
  case NPVariantType_Int32 :
    {
      
      
      return ::JS_NumberValue(NPVARIANT_TO_INT32(*variant));
    }
  case NPVariantType_Double :
    {
      return ::JS_NumberValue(NPVARIANT_TO_DOUBLE(*variant));
    }
  case NPVariantType_String :
    {
      const NPString *s = &NPVARIANT_TO_STRING(*variant);
      NS_ConvertUTF8toUTF16 utf16String(s->UTF8Characters, s->UTF8Length);

      JSString *str =
        ::JS_NewUCStringCopyN(cx, reinterpret_cast<const jschar*>
                                                  (utf16String.get()),
                              utf16String.Length());

      if (str) {
        return STRING_TO_JSVAL(str);
      }

      break;
    }
  case NPVariantType_Object:
    {
      if (npp) {
        JSObject *obj =
          nsNPObjWrapper::GetNewOrUsed(npp, cx, NPVARIANT_TO_OBJECT(*variant));

        if (obj) {
          return OBJECT_TO_JSVAL(obj);
        }
      }

      NS_ERROR("Error wrapping NPObject!");

      break;
    }
  default:
    NS_ERROR("Unknown NPVariant type!");
  }

  NS_ERROR("Unable to convert NPVariant to jsval!");

  return JSVAL_VOID;
}

bool
JSValToNPVariant(NPP npp, JSContext *cx, JS::Value val, NPVariant *variant)
{
  NS_ASSERTION(npp, "Must have an NPP to wrap a jsval!");

  if (JSVAL_IS_PRIMITIVE(val)) {
    if (val == JSVAL_VOID) {
      VOID_TO_NPVARIANT(*variant);
    } else if (JSVAL_IS_NULL(val)) {
      NULL_TO_NPVARIANT(*variant);
    } else if (JSVAL_IS_BOOLEAN(val)) {
      BOOLEAN_TO_NPVARIANT(JSVAL_TO_BOOLEAN(val), *variant);
    } else if (JSVAL_IS_INT(val)) {
      INT32_TO_NPVARIANT(JSVAL_TO_INT(val), *variant);
    } else if (JSVAL_IS_DOUBLE(val)) {
      double d = JSVAL_TO_DOUBLE(val);
      int i;
      if (JS_DoubleIsInt32(d, &i)) {
        INT32_TO_NPVARIANT(i, *variant);
      } else {
        DOUBLE_TO_NPVARIANT(d, *variant);
      }
    } else if (JSVAL_IS_STRING(val)) {
      JSString *jsstr = JSVAL_TO_STRING(val);
      size_t length;
      const jschar *chars = ::JS_GetStringCharsZAndLength(cx, jsstr, &length);
      if (!chars) {
          return false;
      }

      nsDependentString str(chars, length);

      uint32_t len;
      char *p = ToNewUTF8String(str, &len);

      if (!p) {
        return false;
      }

      STRINGN_TO_NPVARIANT(p, len, *variant);
    } else {
      NS_ERROR("Unknown primitive type!");

      return false;
    }

    return true;
  }

  
  
  
  
  
  
  
  JS::Rooted<JSObject*> obj(cx, val.toObjectOrNull());
  obj = js::CheckedUnwrap(obj);
  if (!obj) {
    obj = JSVAL_TO_OBJECT(val);
  }

  NPObject *npobj = nsJSObjWrapper::GetNewOrUsed(npp, cx, obj);
  if (!npobj) {
    return false;
  }

  
  OBJECT_TO_NPVARIANT(npobj, *variant);

  return true;
}

static void
ThrowJSException(JSContext *cx, const char *message)
{
  const char *ex = PeekException();

  if (ex) {
    nsAutoString ucex;

    if (message) {
      AppendASCIItoUTF16(message, ucex);

      AppendASCIItoUTF16(" [plugin exception: ", ucex);
    }

    AppendUTF8toUTF16(ex, ucex);

    if (message) {
      AppendASCIItoUTF16("].", ucex);
    }

    JSString *str = ::JS_NewUCStringCopyN(cx, (jschar *)ucex.get(),
                                          ucex.Length());

    if (str) {
      JS::Rooted<JS::Value> exn(cx, JS::StringValue(str));
      ::JS_SetPendingException(cx, exn);
    }

    PopException();
  } else {
    ::JS_ReportError(cx, message);
  }
}

static bool
ReportExceptionIfPending(JSContext *cx)
{
  const char *ex = PeekException();

  if (!ex) {
    return true;
  }

  ThrowJSException(cx, nullptr);

  return false;
}

nsJSObjWrapper::nsJSObjWrapper(NPP npp)
  : mNpp(npp)
{
  MOZ_COUNT_CTOR(nsJSObjWrapper);
  OnWrapperCreated();
}

nsJSObjWrapper::~nsJSObjWrapper()
{
  MOZ_COUNT_DTOR(nsJSObjWrapper);

  
  NP_Invalidate(this);

  OnWrapperDestroyed();
}

void
nsJSObjWrapper::ClearJSObject() {
  
  JS_RemoveObjectRootRT(sJSRuntime, &mJSObj);

  
  mJSObj = nullptr;
}


NPObject *
nsJSObjWrapper::NP_Allocate(NPP npp, NPClass *aClass)
{
  NS_ASSERTION(aClass == &sJSObjWrapperNPClass,
               "Huh, wrong class passed to NP_Allocate()!!!");

  return new nsJSObjWrapper(npp);
}


void
nsJSObjWrapper::NP_Deallocate(NPObject *npobj)
{
  
  delete (nsJSObjWrapper *)npobj;
}


void
nsJSObjWrapper::NP_Invalidate(NPObject *npobj)
{
  nsJSObjWrapper *jsnpobj = (nsJSObjWrapper *)npobj;

  if (jsnpobj && jsnpobj->mJSObj) {

    
    MOZ_ASSERT(sJSObjWrappers.initialized());
    nsJSObjWrapperKey key(jsnpobj->mJSObj, jsnpobj->mNpp);
    sJSObjWrappers.remove(key);

    jsnpobj->ClearJSObject();
  }
}

static bool
GetProperty(JSContext *cx, JSObject *obj, NPIdentifier id, JS::MutableHandle<JS::Value> rval)
{
  NS_ASSERTION(NPIdentifierIsInt(id) || NPIdentifierIsString(id),
               "id must be either string or int!\n");
  return ::JS_GetPropertyById(cx, obj, NPIdentifierToJSId(id), rval);
}


bool
nsJSObjWrapper::NP_HasMethod(NPObject *npobj, NPIdentifier id)
{
  NPP npp = NPPStack::Peek();
  JSContext *cx = GetJSContext(npp);

  if (!cx) {
    return false;
  }

  if (!npobj) {
    ThrowJSException(cx,
                     "Null npobj in nsJSObjWrapper::NP_HasMethod!");

    return false;
  }

  nsJSObjWrapper *npjsobj = (nsJSObjWrapper *)npobj;

  nsCxPusher pusher;
  pusher.Push(cx);
  JSAutoCompartment ac(cx, npjsobj->mJSObj);

  AutoJSExceptionReporter reporter(cx);

  JS::Rooted<JS::Value> v(cx);
  bool ok = GetProperty(cx, npjsobj->mJSObj, id, &v);

  return ok && !JSVAL_IS_PRIMITIVE(v) &&
    ::JS_ObjectIsFunction(cx, JSVAL_TO_OBJECT(v));
}

static bool
doInvoke(NPObject *npobj, NPIdentifier method, const NPVariant *args,
         uint32_t argCount, bool ctorCall, NPVariant *result)
{
  NPP npp = NPPStack::Peek();
  JSContext *cx = GetJSContext(npp);

  if (!cx) {
    return false;
  }

  if (!npobj || !result) {
    ThrowJSException(cx, "Null npobj, or result in doInvoke!");

    return false;
  }

  
  VOID_TO_NPVARIANT(*result);

  nsJSObjWrapper *npjsobj = (nsJSObjWrapper *)npobj;

  nsCxPusher pusher;
  pusher.Push(cx);
  JSAutoCompartment ac(cx, npjsobj->mJSObj);
  JS::Rooted<JS::Value> fv(cx);

  AutoJSExceptionReporter reporter(cx);

  if (method != NPIdentifier_VOID) {
    if (!GetProperty(cx, npjsobj->mJSObj, method, &fv) ||
        ::JS_TypeOfValue(cx, fv) != JSTYPE_FUNCTION) {
      return false;
    }
  } else {
    fv = OBJECT_TO_JSVAL(npjsobj->mJSObj);
  }

  JS::Value jsargs_buf[8];
  JS::Value *jsargs = jsargs_buf;

  if (argCount > (sizeof(jsargs_buf) / sizeof(JS::Value))) {
    
    
    jsargs = (JS::Value *)PR_Malloc(argCount * sizeof(JS::Value));
    if (!jsargs) {
      ::JS_ReportOutOfMemory(cx);

      return false;
    }
  }

  JS::Rooted<JS::Value> v(cx);
  bool ok;

  {
    JS::AutoArrayRooter tvr(cx, 0, jsargs);

    
    for (uint32_t i = 0; i < argCount; ++i) {
      jsargs[i] = NPVariantToJSVal(npp, cx, args + i);
      tvr.changeLength(i + 1);
    }

    if (ctorCall) {
      JSObject *newObj =
        ::JS_New(cx, npjsobj->mJSObj, argCount, jsargs);

      if (newObj) {
        v = OBJECT_TO_JSVAL(newObj);
        ok = true;
      } else {
        ok = false;
      }
    } else {
      ok = ::JS_CallFunctionValue(cx, npjsobj->mJSObj, fv, argCount, jsargs, v.address());
    }

  }

  if (jsargs != jsargs_buf)
    PR_Free(jsargs);

  if (ok)
    ok = JSValToNPVariant(npp, cx, v, result);

  return ok;
}


bool
nsJSObjWrapper::NP_Invoke(NPObject *npobj, NPIdentifier method,
                          const NPVariant *args, uint32_t argCount,
                          NPVariant *result)
{
  if (method == NPIdentifier_VOID) {
    return false;
  }

  return doInvoke(npobj, method, args, argCount, false, result);
}


bool
nsJSObjWrapper::NP_InvokeDefault(NPObject *npobj, const NPVariant *args,
                                 uint32_t argCount, NPVariant *result)
{
  return doInvoke(npobj, NPIdentifier_VOID, args, argCount, false,
                  result);
}


bool
nsJSObjWrapper::NP_HasProperty(NPObject *npobj, NPIdentifier id)
{
  NPP npp = NPPStack::Peek();
  JSContext *cx = GetJSContext(npp);

  if (!cx) {
    return false;
  }

  if (!npobj) {
    ThrowJSException(cx,
                     "Null npobj in nsJSObjWrapper::NP_HasProperty!");

    return false;
  }

  nsJSObjWrapper *npjsobj = (nsJSObjWrapper *)npobj;
  bool found, ok = false;

  nsCxPusher pusher;
  pusher.Push(cx);
  AutoJSExceptionReporter reporter(cx);
  JSAutoCompartment ac(cx, npjsobj->mJSObj);

  NS_ASSERTION(NPIdentifierIsInt(id) || NPIdentifierIsString(id),
               "id must be either string or int!\n");
  ok = ::JS_HasPropertyById(cx, npjsobj->mJSObj, NPIdentifierToJSId(id), &found);
  return ok && found;
}


bool
nsJSObjWrapper::NP_GetProperty(NPObject *npobj, NPIdentifier id,
                               NPVariant *result)
{
  NPP npp = NPPStack::Peek();
  JSContext *cx = GetJSContext(npp);

  if (!cx) {
    return false;
  }

  if (!npobj) {
    ThrowJSException(cx,
                     "Null npobj in nsJSObjWrapper::NP_GetProperty!");

    return false;
  }

  nsJSObjWrapper *npjsobj = (nsJSObjWrapper *)npobj;

  nsCxPusher pusher;
  pusher.Push(cx);
  AutoJSExceptionReporter reporter(cx);
  JSAutoCompartment ac(cx, npjsobj->mJSObj);

  JS::Rooted<JS::Value> v(cx);
  return (GetProperty(cx, npjsobj->mJSObj, id, &v) &&
          JSValToNPVariant(npp, cx, v, result));
}


bool
nsJSObjWrapper::NP_SetProperty(NPObject *npobj, NPIdentifier id,
                               const NPVariant *value)
{
  NPP npp = NPPStack::Peek();
  JSContext *cx = GetJSContext(npp);

  if (!cx) {
    return false;
  }

  if (!npobj) {
    ThrowJSException(cx,
                     "Null npobj in nsJSObjWrapper::NP_SetProperty!");

    return false;
  }

  nsJSObjWrapper *npjsobj = (nsJSObjWrapper *)npobj;
  bool ok = false;

  nsCxPusher pusher;
  pusher.Push(cx);
  AutoJSExceptionReporter reporter(cx);
  JSAutoCompartment ac(cx, npjsobj->mJSObj);

  JS::Rooted<JS::Value> v(cx, NPVariantToJSVal(npp, cx, value));

  NS_ASSERTION(NPIdentifierIsInt(id) || NPIdentifierIsString(id),
               "id must be either string or int!\n");
  ok = ::JS_SetPropertyById(cx, npjsobj->mJSObj, NPIdentifierToJSId(id), v);

  return ok;
}


bool
nsJSObjWrapper::NP_RemoveProperty(NPObject *npobj, NPIdentifier id)
{
  NPP npp = NPPStack::Peek();
  JSContext *cx = GetJSContext(npp);

  if (!cx) {
    return false;
  }

  if (!npobj) {
    ThrowJSException(cx,
                     "Null npobj in nsJSObjWrapper::NP_RemoveProperty!");

    return false;
  }

  nsJSObjWrapper *npjsobj = (nsJSObjWrapper *)npobj;
  bool ok = false;

  nsCxPusher pusher;
  pusher.Push(cx);
  AutoJSExceptionReporter reporter(cx);
  bool deleted = false;
  JSAutoCompartment ac(cx, npjsobj->mJSObj);

  NS_ASSERTION(NPIdentifierIsInt(id) || NPIdentifierIsString(id),
               "id must be either string or int!\n");
  ok = ::JS_DeletePropertyById2(cx, npjsobj->mJSObj, NPIdentifierToJSId(id), &deleted);
  if (ok && deleted) {
    
    

    bool hasProp;
    ok = ::JS_HasPropertyById(cx, npjsobj->mJSObj, NPIdentifierToJSId(id), &hasProp);

    if (ok && hasProp) {
      
      

      deleted = false;
    }
  }

  return ok && deleted;
}


bool
nsJSObjWrapper::NP_Enumerate(NPObject *npobj, NPIdentifier **idarray,
                             uint32_t *count)
{
  NPP npp = NPPStack::Peek();
  JSContext *cx = GetJSContext(npp);

  *idarray = 0;
  *count = 0;

  if (!cx) {
    return false;
  }

  if (!npobj) {
    ThrowJSException(cx,
                     "Null npobj in nsJSObjWrapper::NP_Enumerate!");

    return false;
  }

  nsJSObjWrapper *npjsobj = (nsJSObjWrapper *)npobj;

  nsCxPusher pusher;
  pusher.Push(cx);
  AutoJSExceptionReporter reporter(cx);
  JSAutoCompartment ac(cx, npjsobj->mJSObj);

  JS::AutoIdArray ida(cx, JS_Enumerate(cx, npjsobj->mJSObj));
  if (!ida) {
    return false;
  }

  *count = ida.length();
  *idarray = (NPIdentifier *)PR_Malloc(*count * sizeof(NPIdentifier));
  if (!*idarray) {
    ThrowJSException(cx, "Memory allocation failed for NPIdentifier!");
    return false;
  }

  for (uint32_t i = 0; i < *count; i++) {
    JS::Rooted<JS::Value> v(cx);
    if (!JS_IdToValue(cx, ida[i], v.address())) {
      PR_Free(*idarray);
      return false;
    }

    NPIdentifier id;
    if (v.isString()) {
      JS::Rooted<JSString*> str(cx, v.toString());
      str = JS_InternJSString(cx, str);
      if (!str) {
        PR_Free(*idarray);
        return false;
      }
      id = StringToNPIdentifier(cx, str);
    } else {
      NS_ASSERTION(JSVAL_IS_INT(v),
                   "The element in ida must be either string or int!\n");
      id = IntToNPIdentifier(JSVAL_TO_INT(v));
    }

    (*idarray)[i] = id;
  }

  return true;
}


bool
nsJSObjWrapper::NP_Construct(NPObject *npobj, const NPVariant *args,
                             uint32_t argCount, NPVariant *result)
{
  return doInvoke(npobj, NPIdentifier_VOID, args, argCount, true, result);
}







static void
JSObjWrapperKeyMarkCallback(JSTracer *trc, void *key, void *data) {
  JSObject *obj = static_cast<JSObject*>(key);
  nsJSObjWrapper* wrapper = static_cast<nsJSObjWrapper*>(data);
  JSObject *prior = obj;
  JS_CallObjectTracer(trc, &obj, "sJSObjWrappers key object");
  NPP npp = wrapper->mNpp;
  nsJSObjWrapperKey oldKey(prior, npp);
  nsJSObjWrapperKey newKey(obj, npp);
  sJSObjWrappers.rekeyIfMoved(oldKey, newKey);
}




NPObject *
nsJSObjWrapper::GetNewOrUsed(NPP npp, JSContext *cx, JS::Handle<JSObject*> obj)
{
  if (!npp) {
    NS_ERROR("Null NPP passed to nsJSObjWrapper::GetNewOrUsed()!");

    return nullptr;
  }

  if (!cx) {
    cx = GetJSContext(npp);

    if (!cx) {
      NS_ERROR("Unable to find a JSContext in nsJSObjWrapper::GetNewOrUsed()!");

      return nullptr;
    }
  }

  
  
  

  const JSClass *clazz = JS_GetClass(obj);

  if (clazz == &sNPObjectJSWrapperClass) {
    
    

    NPObject *npobj = (NPObject *)::JS_GetPrivate(obj);

    
    
    
    
    
    
    if (!npobj)
      return nullptr;

    if (LookupNPP(npobj) == npp)
      return _retainobject(npobj);
  }

  if (!sJSObjWrappers.initialized()) {
    
    if (!sJSObjWrappers.init(16)) {
      NS_ERROR("Error initializing PLDHashTable!");

      return nullptr;
    }
  }

  nsJSObjWrapperKey key(obj, npp);

  JSObjWrapperTable::AddPtr p = sJSObjWrappers.lookupForAdd(key);

  if (p) {
    MOZ_ASSERT(p->value());
    

    return _retainobject(p->value());
  }

  

  nsJSObjWrapper *wrapper =
    (nsJSObjWrapper *)_createobject(npp, &sJSObjWrapperNPClass);

  if (!wrapper) {
    
    return nullptr;
  }

  wrapper->mJSObj = obj;

  if (!sJSObjWrappers.add(p, key, wrapper)) {
    
    _releaseobject(wrapper);
    return nullptr;
  }

  NS_ASSERTION(wrapper->mNpp == npp, "nsJSObjWrapper::mNpp not initialized!");

  
  
  if (!::JS_AddNamedObjectRoot(cx, &wrapper->mJSObj, "nsJSObjWrapper::mJSObject")) {
    NS_ERROR("Failed to root JSObject!");

    sJSObjWrappers.remove(key);
    _releaseobject(wrapper);

    return nullptr;
  }

  
  JS_StoreObjectPostBarrierCallback(cx, JSObjWrapperKeyMarkCallback, obj, wrapper);

  return wrapper;
}







static JSObject *
GetNPObjectWrapper(JSContext *cx, JSObject *aObj, bool wrapResult = true)
{
  JS::Rooted<JSObject*> obj(cx, aObj);
  while (obj && (obj = js::CheckedUnwrap(obj))) {
    if (JS_GetClass(obj) == &sNPObjectJSWrapperClass) {
      if (wrapResult && !JS_WrapObject(cx, &obj)) {
        return nullptr;
      }
      return obj;
    }
    if (!::JS_GetPrototype(cx, obj, &obj)) {
      return nullptr;
    }
  }
  return nullptr;
}

static NPObject *
GetNPObject(JSContext *cx, JSObject *obj)
{
  obj = GetNPObjectWrapper(cx, obj,  false);
  if (!obj) {
    return nullptr;
  }

  return (NPObject *)::JS_GetPrivate(obj);
}




static bool
NPObjWrapper_AddProperty(JSContext *cx, JS::Handle<JSObject*> obj, JS::Handle<jsid> id, JS::MutableHandle<JS::Value> vp)
{
  NPObject *npobj = GetNPObject(cx, obj);

  if (!npobj || !npobj->_class || !npobj->_class->hasProperty ||
      !npobj->_class->hasMethod) {
    ThrowJSException(cx, "Bad NPObject as private data!");

    return false;
  }

  if (NPObjectIsOutOfProcessProxy(npobj)) {
    return true;
  }

  PluginDestructionGuard pdg(LookupNPP(npobj));

  NPIdentifier identifier = JSIdToNPIdentifier(id);
  bool hasProperty = npobj->_class->hasProperty(npobj, identifier);
  if (!ReportExceptionIfPending(cx))
    return false;

  if (hasProperty)
    return true;

  
  
  bool hasMethod = npobj->_class->hasMethod(npobj, identifier);
  if (!ReportExceptionIfPending(cx))
    return false;

  if (!hasMethod) {
    ThrowJSException(cx, "Trying to add unsupported property on NPObject!");

    return false;
  }

  return true;
}

static bool
NPObjWrapper_DelProperty(JSContext *cx, JS::Handle<JSObject*> obj, JS::Handle<jsid> id, bool *succeeded)
{
  NPObject *npobj = GetNPObject(cx, obj);

  if (!npobj || !npobj->_class || !npobj->_class->hasProperty ||
      !npobj->_class->removeProperty) {
    ThrowJSException(cx, "Bad NPObject as private data!");

    return false;
  }

  PluginDestructionGuard pdg(LookupNPP(npobj));

  NPIdentifier identifier = JSIdToNPIdentifier(id);

  if (!NPObjectIsOutOfProcessProxy(npobj)) {
    bool hasProperty = npobj->_class->hasProperty(npobj, identifier);
    if (!ReportExceptionIfPending(cx))
      return false;

    if (!hasProperty) {
      *succeeded = true;
      return true;
    }
  }

  if (!npobj->_class->removeProperty(npobj, identifier))
    *succeeded = false;

  return ReportExceptionIfPending(cx);
}

static bool
NPObjWrapper_SetProperty(JSContext *cx, JS::Handle<JSObject*> obj, JS::Handle<jsid> id, bool strict,
                         JS::MutableHandle<JS::Value> vp)
{
  NPObject *npobj = GetNPObject(cx, obj);

  if (!npobj || !npobj->_class || !npobj->_class->hasProperty ||
      !npobj->_class->setProperty) {
    ThrowJSException(cx, "Bad NPObject as private data!");

    return false;
  }

  
  
  NPP npp = LookupNPP(npobj);

  if (!npp) {
    ThrowJSException(cx, "No NPP found for NPObject!");

    return false;
  }

  PluginDestructionGuard pdg(npp);

  NPIdentifier identifier = JSIdToNPIdentifier(id);

  if (!NPObjectIsOutOfProcessProxy(npobj)) {
    bool hasProperty = npobj->_class->hasProperty(npobj, identifier);
    if (!ReportExceptionIfPending(cx))
      return false;

    if (!hasProperty) {
      ThrowJSException(cx, "Trying to set unsupported property on NPObject!");

      return false;
    }
  }

  NPVariant npv;
  if (!JSValToNPVariant(npp, cx, vp, &npv)) {
    ThrowJSException(cx, "Error converting jsval to NPVariant!");

    return false;
  }

  bool ok = npobj->_class->setProperty(npobj, identifier, &npv);
  _releasevariantvalue(&npv); 
  if (!ReportExceptionIfPending(cx))
    return false;

  if (!ok) {
    ThrowJSException(cx, "Error setting property on NPObject!");

    return false;
  }

  return true;
}

static bool
NPObjWrapper_GetProperty(JSContext *cx, JS::Handle<JSObject*> obj, JS::Handle<jsid> id, JS::MutableHandle<JS::Value> vp)
{
  NPObject *npobj = GetNPObject(cx, obj);

  if (!npobj || !npobj->_class || !npobj->_class->hasProperty ||
      !npobj->_class->hasMethod || !npobj->_class->getProperty) {
    ThrowJSException(cx, "Bad NPObject as private data!");

    return false;
  }

  
  
  NPP npp = LookupNPP(npobj);
  if (!npp) {
    ThrowJSException(cx, "No NPP found for NPObject!");

    return false;
  }

  PluginDestructionGuard pdg(npp);

  bool hasProperty, hasMethod;

  NPVariant npv;
  VOID_TO_NPVARIANT(npv);

  NPIdentifier identifier = JSIdToNPIdentifier(id);

  if (NPObjectIsOutOfProcessProxy(npobj)) {
    PluginScriptableObjectParent* actor =
      static_cast<ParentNPObject*>(npobj)->parent;

    
    if (!actor)
      return false;

    bool success = actor->GetPropertyHelper(identifier, &hasProperty,
                                            &hasMethod, &npv);
    if (!ReportExceptionIfPending(cx)) {
      if (success)
        _releasevariantvalue(&npv);
      return false;
    }

    if (success) {
      
      if (hasProperty && hasMethod)
        return CreateNPObjectMember(npp, cx, obj, npobj, id, &npv, vp.address());

      if (hasProperty) {
        vp.set(NPVariantToJSVal(npp, cx, &npv));
        _releasevariantvalue(&npv);

        if (!ReportExceptionIfPending(cx))
          return false;
      }
    }
    return true;
  }

  hasProperty = npobj->_class->hasProperty(npobj, identifier);
  if (!ReportExceptionIfPending(cx))
    return false;

  hasMethod = npobj->_class->hasMethod(npobj, identifier);
  if (!ReportExceptionIfPending(cx))
    return false;

  
  if (hasProperty && hasMethod)
    return CreateNPObjectMember(npp, cx, obj, npobj, id, nullptr, vp.address());

  if (hasProperty) {
    if (npobj->_class->getProperty(npobj, identifier, &npv))
      vp.set(NPVariantToJSVal(npp, cx, &npv));

    _releasevariantvalue(&npv);

    if (!ReportExceptionIfPending(cx))
      return false;
  }

  return true;
}

static bool
CallNPMethodInternal(JSContext *cx, JS::Handle<JSObject*> obj, unsigned argc,
                     JS::Value *argv, JS::Value *rval, bool ctorCall)
{
  NPObject *npobj = GetNPObject(cx, obj);

  if (!npobj || !npobj->_class) {
    ThrowJSException(cx, "Bad NPObject as private data!");

    return false;
  }

  
  
  NPP npp = LookupNPP(npobj);

  if (!npp) {
    ThrowJSException(cx, "Error finding NPP for NPObject!");

    return false;
  }

  PluginDestructionGuard pdg(npp);

  NPVariant npargs_buf[8];
  NPVariant *npargs = npargs_buf;

  if (argc > (sizeof(npargs_buf) / sizeof(NPVariant))) {
    
    
    npargs = (NPVariant *)PR_Malloc(argc * sizeof(NPVariant));

    if (!npargs) {
      ThrowJSException(cx, "Out of memory!");

      return false;
    }
  }

  
  uint32_t i;
  for (i = 0; i < argc; ++i) {
    if (!JSValToNPVariant(npp, cx, argv[i], npargs + i)) {
      ThrowJSException(cx, "Error converting jsvals to NPVariants!");

      if (npargs != npargs_buf) {
        PR_Free(npargs);
      }

      return false;
    }
  }

  NPVariant v;
  VOID_TO_NPVARIANT(v);

  JSObject *funobj = JSVAL_TO_OBJECT(argv[-2]);
  bool ok;
  const char *msg = "Error calling method on NPObject!";

  if (ctorCall) {
    
    

    if (NP_CLASS_STRUCT_VERSION_HAS_CTOR(npobj->_class) &&
        npobj->_class->construct) {
      ok = npobj->_class->construct(npobj, npargs, argc, &v);
    } else {
      ok = false;

      msg = "Attempt to construct object from class with no constructor.";
    }
  } else if (funobj != obj) {
    
    

    if (npobj->_class->invoke) {
      JSFunction *fun = ::JS_GetObjectFunction(funobj);
      JS::Rooted<JSString*> funId(cx, ::JS_GetFunctionId(fun));
      JSString *name = ::JS_InternJSString(cx, funId);
      NPIdentifier id = StringToNPIdentifier(cx, name);

      ok = npobj->_class->invoke(npobj, id, npargs, argc, &v);
    } else {
      ok = false;

      msg = "Attempt to call a method on object with no invoke method.";
    }
  } else {
    if (npobj->_class->invokeDefault) {
      
      

      ok = npobj->_class->invokeDefault(npobj, npargs, argc, &v);
    } else {
      ok = false;

      msg = "Attempt to call a default method on object with no "
        "invokeDefault method.";
    }
  }

  
  for (i = 0; i < argc; ++i) {
    _releasevariantvalue(npargs + i);
  }

  if (npargs != npargs_buf) {
    PR_Free(npargs);
  }

  if (!ok) {
    
    
    if (ReportExceptionIfPending(cx))
      ThrowJSException(cx, msg);

    return false;
  }

  *rval = NPVariantToJSVal(npp, cx, &v);

  
  _releasevariantvalue(&v);

  return ReportExceptionIfPending(cx);
}

static bool
CallNPMethod(JSContext *cx, unsigned argc, JS::Value *vp)
{
  JS::Rooted<JSObject*> obj(cx, JS_THIS_OBJECT(cx, vp));
  if (!obj)
      return false;

  return CallNPMethodInternal(cx, obj, argc, JS_ARGV(cx, vp), vp, false);
}

struct NPObjectEnumerateState {
  uint32_t     index;
  uint32_t     length;
  NPIdentifier *value;
};

static bool
NPObjWrapper_newEnumerate(JSContext *cx, JS::Handle<JSObject*> obj, JSIterateOp enum_op,
                          JS::Value *statep, jsid *idp)
{
  NPObject *npobj = GetNPObject(cx, obj);
  NPIdentifier *enum_value;
  uint32_t length;
  NPObjectEnumerateState *state;

  if (!npobj || !npobj->_class) {
    ThrowJSException(cx, "Bad NPObject as private data!");
    return false;
  }

  PluginDestructionGuard pdg(LookupNPP(npobj));

  NS_ASSERTION(statep, "Must have a statep to enumerate!");

  switch(enum_op) {
  case JSENUMERATE_INIT:
  case JSENUMERATE_INIT_ALL:
    state = new NPObjectEnumerateState();
    if (!state) {
      ThrowJSException(cx, "Memory allocation failed for "
                       "NPObjectEnumerateState!");

      return false;
    }

    if (!NP_CLASS_STRUCT_VERSION_HAS_ENUM(npobj->_class) ||
        !npobj->_class->enumerate) {
      enum_value = 0;
      length = 0;
    } else if (!npobj->_class->enumerate(npobj, &enum_value, &length)) {
      delete state;

      if (ReportExceptionIfPending(cx)) {
        
        
        ThrowJSException(cx, "Error enumerating properties on scriptable "
                             "plugin object");
      }

      return false;
    }

    state->value = enum_value;
    state->length = length;
    state->index = 0;
    *statep = PRIVATE_TO_JSVAL(state);
    if (idp) {
      *idp = INT_TO_JSID(length);
    }

    break;

  case JSENUMERATE_NEXT:
    state = (NPObjectEnumerateState *)JSVAL_TO_PRIVATE(*statep);
    enum_value = state->value;
    length = state->length;
    if (state->index != length) {
      *idp = NPIdentifierToJSId(enum_value[state->index++]);
      return true;
    }

    

  case JSENUMERATE_DESTROY:
    state = (NPObjectEnumerateState *)JSVAL_TO_PRIVATE(*statep);
    if (state->value)
      PR_Free(state->value);
    delete state;
    *statep = JSVAL_NULL;

    break;
  }

  return true;
}

static bool
NPObjWrapper_NewResolve(JSContext *cx, JS::Handle<JSObject*> obj, JS::Handle<jsid> id, unsigned flags,
                        JS::MutableHandle<JSObject*> objp)
{
  NPObject *npobj = GetNPObject(cx, obj);

  if (!npobj || !npobj->_class || !npobj->_class->hasProperty ||
      !npobj->_class->hasMethod) {
    ThrowJSException(cx, "Bad NPObject as private data!");

    return false;
  }

  PluginDestructionGuard pdg(LookupNPP(npobj));

  NPIdentifier identifier = JSIdToNPIdentifier(id);

  bool hasProperty = npobj->_class->hasProperty(npobj, identifier);
  if (!ReportExceptionIfPending(cx))
    return false;

  if (hasProperty) {
    NS_ASSERTION(JSID_IS_STRING(id) || JSID_IS_INT(id),
                 "id must be either string or int!\n");
    if (!::JS_DefinePropertyById(cx, obj, id, JSVAL_VOID, nullptr,
                                 nullptr, JSPROP_ENUMERATE | JSPROP_SHARED)) {
        return false;
    }

    objp.set(obj);

    return true;
  }

  bool hasMethod = npobj->_class->hasMethod(npobj, identifier);
  if (!ReportExceptionIfPending(cx))
    return false;

  if (hasMethod) {
    NS_ASSERTION(JSID_IS_STRING(id) || JSID_IS_INT(id),
                 "id must be either string or int!\n");

    JSFunction *fnc = ::JS_DefineFunctionById(cx, obj, id, CallNPMethod, 0,
                                              JSPROP_ENUMERATE);

    objp.set(obj);

    return fnc != nullptr;
  }

  
  return true;
}

static bool
NPObjWrapper_Convert(JSContext *cx, JS::Handle<JSObject*> obj, JSType hint, JS::MutableHandle<JS::Value> vp)
{
  JS_ASSERT(hint == JSTYPE_NUMBER || hint == JSTYPE_STRING || hint == JSTYPE_VOID);

  
  
  
  
  
  
  
  
  
  

  JS::Rooted<JS::Value> v(cx, JSVAL_VOID);
  if (!JS_GetProperty(cx, obj, "toString", &v))
    return false;
  if (!JSVAL_IS_PRIMITIVE(v) && JS_ObjectIsCallable(cx, JSVAL_TO_OBJECT(v))) {
    if (!JS_CallFunctionValue(cx, obj, v, 0, nullptr, vp.address()))
      return false;
    if (JSVAL_IS_PRIMITIVE(vp))
      return true;
  }

  JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_CONVERT_TO,
                       JS_GetClass(obj)->name,
                       hint == JSTYPE_VOID
                       ? "primitive type"
                       : hint == JSTYPE_NUMBER
                       ? "number"
                       : "string");
  return false;
}

static void
NPObjWrapper_Finalize(JSFreeOp *fop, JSObject *obj)
{
  NPObject *npobj = (NPObject *)::JS_GetPrivate(obj);
  if (npobj) {
    if (sNPObjWrappers.ops) {
      PL_DHashTableOperate(&sNPObjWrappers, npobj, PL_DHASH_REMOVE);
    }
  }

  if (!sDelayedReleases)
    sDelayedReleases = new nsTArray<NPObject*>;
  sDelayedReleases->AppendElement(npobj);
}

static bool
NPObjWrapper_Call(JSContext *cx, unsigned argc, JS::Value *vp)
{
  JS::Rooted<JSObject*> obj(cx, JSVAL_TO_OBJECT(JS_CALLEE(cx, vp)));
  return CallNPMethodInternal(cx, obj, argc, JS_ARGV(cx, vp), vp, false);
}

static bool
NPObjWrapper_Construct(JSContext *cx, unsigned argc, JS::Value *vp)
{
  JS::Rooted<JSObject*> obj(cx, JSVAL_TO_OBJECT(JS_CALLEE(cx, vp)));
  return CallNPMethodInternal(cx, obj, argc, JS_ARGV(cx, vp), vp, true);
}

class NPObjWrapperHashEntry : public PLDHashEntryHdr
{
public:
  NPObject *mNPObj; 
  JSObject *mJSObj;
  NPP mNpp;
};







void
nsNPObjWrapper::OnDestroy(NPObject *npobj)
{
  if (!npobj) {
    return;
  }

  if (npobj->_class == &nsJSObjWrapper::sJSObjWrapperNPClass) {
    

    return;
  }

  if (!sNPObjWrappers.ops) {
    

    return;
  }

  NPObjWrapperHashEntry *entry = static_cast<NPObjWrapperHashEntry *>
    (PL_DHashTableOperate(&sNPObjWrappers, npobj, PL_DHASH_LOOKUP));

  if (PL_DHASH_ENTRY_IS_BUSY(entry) && entry->mJSObj) {
    
    

    ::JS_SetPrivate(entry->mJSObj, nullptr);

    
    PL_DHashTableRawRemove(&sNPObjWrappers, entry);

    
  }
}




JSObject *
nsNPObjWrapper::GetNewOrUsed(NPP npp, JSContext *cx, NPObject *npobj)
{
  if (!npobj) {
    NS_ERROR("Null NPObject passed to nsNPObjWrapper::GetNewOrUsed()!");

    return nullptr;
  }

  if (npobj->_class == &nsJSObjWrapper::sJSObjWrapperNPClass) {
    

    JS::Rooted<JSObject*> obj(cx, ((nsJSObjWrapper *)npobj)->mJSObj);
    if (!JS_WrapObject(cx, &obj)) {
      return nullptr;
    }
    return obj;
  }

  if (!npp) {
    NS_ERROR("No npp passed to nsNPObjWrapper::GetNewOrUsed()!");

    return nullptr;
  }

  if (!sNPObjWrappers.ops) {
    

    if (!PL_DHashTableInit(&sNPObjWrappers, PL_DHashGetStubOps(), nullptr,
                           sizeof(NPObjWrapperHashEntry), 16)) {
      NS_ERROR("Error initializing PLDHashTable!");

      return nullptr;
    }
  }

  NPObjWrapperHashEntry *entry = static_cast<NPObjWrapperHashEntry *>
    (PL_DHashTableOperate(&sNPObjWrappers, npobj, PL_DHASH_ADD));

  if (!entry) {
    
    JS_ReportOutOfMemory(cx);

    return nullptr;
  }

  if (PL_DHASH_ENTRY_IS_BUSY(entry) && entry->mJSObj) {
    
    
    JS::Rooted<JSObject*> obj(cx, entry->mJSObj);
    if (!JS_WrapObject(cx, &obj)) {
      return nullptr;
    }
    return obj;
  }

  entry->mNPObj = npobj;
  entry->mNpp = npp;

  uint32_t generation = sNPObjWrappers.generation;

  

  JS::Rooted<JSObject*> obj(cx, ::JS_NewObject(cx, &sNPObjectJSWrapperClass, nullptr, nullptr));

  if (generation != sNPObjWrappers.generation) {
      
      

      entry = static_cast<NPObjWrapperHashEntry *>
        (PL_DHashTableOperate(&sNPObjWrappers, npobj, PL_DHASH_LOOKUP));
      NS_ASSERTION(entry && PL_DHASH_ENTRY_IS_BUSY(entry),
                   "Hashtable didn't find what we just added?");
  }

  if (!obj) {
    

    PL_DHashTableRawRemove(&sNPObjWrappers, entry);

    return nullptr;
  }

  OnWrapperCreated();

  entry->mJSObj = obj;

  ::JS_SetPrivate(obj, npobj);

  
  _retainobject(npobj);

  return obj;
}




struct NppAndCx
{
  NPP npp;
  JSContext *cx;
};

static PLDHashOperator
NPObjWrapperPluginDestroyedCallback(PLDHashTable *table, PLDHashEntryHdr *hdr,
                                    uint32_t number, void *arg)
{
  NPObjWrapperHashEntry *entry = (NPObjWrapperHashEntry *)hdr;
  NppAndCx *nppcx = reinterpret_cast<NppAndCx *>(arg);

  if (entry->mNpp == nppcx->npp) {
    
    
    const PLDHashTableOps *ops = table->ops;
    table->ops = nullptr;

    NPObject *npobj = entry->mNPObj;

    if (npobj->_class && npobj->_class->invalidate) {
      npobj->_class->invalidate(npobj);
    }

#ifdef NS_BUILD_REFCNT_LOGGING
    {
      int32_t refCnt = npobj->referenceCount;
      while (refCnt) {
        --refCnt;
        NS_LOG_RELEASE(npobj, refCnt, "BrowserNPObject");
      }
    }
#endif

    
    
    if (npobj->_class && npobj->_class->deallocate) {
      npobj->_class->deallocate(npobj);
    } else {
      PR_Free(npobj);
    }

    ::JS_SetPrivate(entry->mJSObj, nullptr);

    table->ops = ops;

    if (sDelayedReleases && sDelayedReleases->RemoveElement(npobj)) {
      OnWrapperDestroyed();
    }

    return PL_DHASH_REMOVE;
  }

  return PL_DHASH_NEXT;
}


void
nsJSNPRuntime::OnPluginDestroy(NPP npp)
{
  if (sJSObjWrappers.initialized()) {
    for (JSObjWrapperTable::Enum e(sJSObjWrappers); !e.empty(); e.popFront()) {
      nsJSObjWrapper *npobj = e.front().value();
      MOZ_ASSERT(npobj->_class == &nsJSObjWrapper::sJSObjWrapperNPClass);
      if (npobj->mNpp == npp) {
        npobj->ClearJSObject();
        _releaseobject(npobj);
        e.removeFront();
      }
    }
  }

  
  
  AutoSafeJSContext cx;
  if (sNPObjWrappers.ops) {
    NppAndCx nppcx = { npp, cx };
    PL_DHashTableEnumerate(&sNPObjWrappers,
                           NPObjWrapperPluginDestroyedCallback, &nppcx);
  }
}



static NPP
LookupNPP(NPObject *npobj)
{
  if (npobj->_class == &nsJSObjWrapper::sJSObjWrapperNPClass) {
    nsJSObjWrapper* o = static_cast<nsJSObjWrapper*>(npobj);
    return o->mNpp;
  }

  NPObjWrapperHashEntry *entry = static_cast<NPObjWrapperHashEntry *>
    (PL_DHashTableOperate(&sNPObjWrappers, npobj, PL_DHASH_ADD));

  if (PL_DHASH_ENTRY_IS_FREE(entry)) {
    return nullptr;
  }

  NS_ASSERTION(entry->mNpp, "Live NPObject entry w/o an NPP!");

  return entry->mNpp;
}

bool
CreateNPObjectMember(NPP npp, JSContext *cx, JSObject *obj, NPObject* npobj,
                     JS::Handle<jsid> id,  NPVariant* getPropertyResult, JS::Value *vp)
{
  NS_ENSURE_TRUE(vp, false);

  if (!npobj || !npobj->_class || !npobj->_class->getProperty ||
      !npobj->_class->invoke) {
    ThrowJSException(cx, "Bad NPObject");

    return false;
  }

  NPObjectMemberPrivate *memberPrivate =
    (NPObjectMemberPrivate *)PR_Malloc(sizeof(NPObjectMemberPrivate));
  if (!memberPrivate)
    return false;

  
  
  memset(memberPrivate, 0, sizeof(NPObjectMemberPrivate));

  JSObject *memobj = ::JS_NewObject(cx, &sNPObjectMemberClass, nullptr, nullptr);
  if (!memobj) {
    PR_Free(memberPrivate);
    return false;
  }

  *vp = OBJECT_TO_JSVAL(memobj);
  ::JS_AddValueRoot(cx, vp);

  ::JS_SetPrivate(memobj, (void *)memberPrivate);

  NPIdentifier identifier = JSIdToNPIdentifier(id);

  JS::Rooted<JS::Value> fieldValue(cx);
  NPVariant npv;

  if (getPropertyResult) {
    
    npv = *getPropertyResult;
  }
  else {
    VOID_TO_NPVARIANT(npv);

    NPBool hasProperty = npobj->_class->getProperty(npobj, identifier,
                                                    &npv);
    if (!ReportExceptionIfPending(cx)) {
      ::JS_RemoveValueRoot(cx, vp);
      return false;
    }

    if (!hasProperty) {
      ::JS_RemoveValueRoot(cx, vp);
      return false;
    }
  }

  fieldValue = NPVariantToJSVal(npp, cx, &npv);

  
  
  
  obj = GetNPObjectWrapper(cx, obj);

  memberPrivate->npobjWrapper = obj;

  memberPrivate->fieldValue = fieldValue;
  memberPrivate->methodName = id;
  memberPrivate->npp = npp;

  ::JS_RemoveValueRoot(cx, vp);

  return true;
}

static bool
NPObjectMember_Convert(JSContext *cx, JS::Handle<JSObject*> obj, JSType type, JS::MutableHandle<JS::Value> vp)
{
  NPObjectMemberPrivate *memberPrivate =
    (NPObjectMemberPrivate *)::JS_GetInstancePrivate(cx, obj,
                                                     &sNPObjectMemberClass,
                                                     nullptr);
  if (!memberPrivate) {
    NS_ERROR("no Ambiguous Member Private data!");
    return false;
  }

  switch (type) {
  case JSTYPE_VOID:
  case JSTYPE_STRING:
  case JSTYPE_NUMBER:
    vp.set(memberPrivate->fieldValue);
    if (!JSVAL_IS_PRIMITIVE(vp)) {
      return JS_DefaultValue(cx, JSVAL_TO_OBJECT(vp), type, vp.address());
    }
    return true;
  case JSTYPE_BOOLEAN:
  case JSTYPE_OBJECT:
    vp.set(memberPrivate->fieldValue);
    return true;
  case JSTYPE_FUNCTION:
    
    return true;
  default:
    NS_ERROR("illegal operation on JSObject prototype object");
    return false;
  }
}

static void
NPObjectMember_Finalize(JSFreeOp *fop, JSObject *obj)
{
  NPObjectMemberPrivate *memberPrivate;

  memberPrivate = (NPObjectMemberPrivate *)::JS_GetPrivate(obj);
  if (!memberPrivate)
    return;

  PR_Free(memberPrivate);
}

static bool
NPObjectMember_Call(JSContext *cx, unsigned argc, JS::Value *vp)
{
  JSObject *memobj = JSVAL_TO_OBJECT(JS_CALLEE(cx, vp));
  NS_ENSURE_TRUE(memobj, false);

  NPObjectMemberPrivate *memberPrivate =
    (NPObjectMemberPrivate *)::JS_GetInstancePrivate(cx, memobj,
                                                     &sNPObjectMemberClass,
                                                     JS_ARGV(cx, vp));
  if (!memberPrivate || !memberPrivate->npobjWrapper)
    return false;

  NPObject *npobj = GetNPObject(cx, memberPrivate->npobjWrapper);
  if (!npobj) {
    ThrowJSException(cx, "Call on invalid member object");

    return false;
  }

  NPVariant npargs_buf[8];
  NPVariant *npargs = npargs_buf;

  if (argc > (sizeof(npargs_buf) / sizeof(NPVariant))) {
    
    
    npargs = (NPVariant *)PR_Malloc(argc * sizeof(NPVariant));

    if (!npargs) {
      ThrowJSException(cx, "Out of memory!");

      return false;
    }
  }

  
  uint32_t i;
  JS::Value *argv = JS_ARGV(cx, vp);
  for (i = 0; i < argc; ++i) {
    if (!JSValToNPVariant(memberPrivate->npp, cx, argv[i], npargs + i)) {
      ThrowJSException(cx, "Error converting jsvals to NPVariants!");

      if (npargs != npargs_buf) {
        PR_Free(npargs);
      }

      return false;
    }
  }


  NPVariant npv;
  bool ok;
  ok = npobj->_class->invoke(npobj, JSIdToNPIdentifier(memberPrivate->methodName),
                             npargs, argc, &npv);

  
  for (i = 0; i < argc; ++i) {
    _releasevariantvalue(npargs + i);
  }

  if (npargs != npargs_buf) {
    PR_Free(npargs);
  }

  if (!ok) {
    
    
    if (ReportExceptionIfPending(cx))
      ThrowJSException(cx, "Error calling method on NPObject!");

    return false;
  }

  JS_SET_RVAL(cx, vp, NPVariantToJSVal(memberPrivate->npp, cx, &npv));

  
  _releasevariantvalue(&npv);

  return ReportExceptionIfPending(cx);
}

static void
NPObjectMember_Trace(JSTracer *trc, JSObject *obj)
{
  NPObjectMemberPrivate *memberPrivate =
    (NPObjectMemberPrivate *)::JS_GetPrivate(obj);
  if (!memberPrivate)
    return;

  
  JS_CallHeapIdTracer(trc, &memberPrivate->methodName, "NPObjectMemberPrivate.methodName");

  if (!JSVAL_IS_PRIMITIVE(memberPrivate->fieldValue)) {
    JS_CallHeapValueTracer(trc, &memberPrivate->fieldValue,
                           "NPObject Member => fieldValue");
  }

  
  
  
  if (memberPrivate->npobjWrapper) {
    JS_CallHeapObjectTracer(trc, &memberPrivate->npobjWrapper,
                            "NPObject Member => npobjWrapper");
  }
}
