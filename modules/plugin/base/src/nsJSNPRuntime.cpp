





































#include "base/basictypes.h"


#include "jscntxt.h"

#include "nsJSNPRuntime.h"
#include "nsNPAPIPlugin.h"
#include "nsNPAPIPluginInstance.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptContext.h"
#include "nsDOMJSUtils.h"
#include "nsIDocument.h"
#include "nsIJSRuntimeService.h"
#include "nsIJSContextStack.h"
#include "nsIXPConnect.h"
#include "nsIDOMElement.h"
#include "prmem.h"
#include "nsIContent.h"

using namespace mozilla::plugins::parent;

#include "mozilla/plugins/PluginScriptableObjectParent.h"
using mozilla::plugins::PluginScriptableObjectParent;
using mozilla::plugins::ParentNPObject;








static PLDHashTable sJSObjWrappers;


static PLDHashTable sNPObjWrappers;




static PRInt32 sWrapperCount;



static JSRuntime *sJSRuntime;



static nsIJSContextStack *sContextStack;

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

static JSBool
NPObjWrapper_AddProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

static JSBool
NPObjWrapper_DelProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

static JSBool
NPObjWrapper_SetProperty(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp);

static JSBool
NPObjWrapper_GetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

static JSBool
NPObjWrapper_newEnumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
                          jsval *statep, jsid *idp);

static JSBool
NPObjWrapper_NewResolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
                        JSObject **objp);

static JSBool
NPObjWrapper_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp);

static void
NPObjWrapper_Finalize(JSContext *cx, JSObject *obj);

static JSBool
NPObjWrapper_Call(JSContext *cx, uintN argc, jsval *vp);

static JSBool
NPObjWrapper_Construct(JSContext *cx, uintN argc, jsval *vp);

static JSBool
CreateNPObjectMember(NPP npp, JSContext *cx, JSObject *obj, NPObject *npobj,
                     jsid id, NPVariant* getPropertyResult, jsval *vp);

static JSClass sNPObjectJSWrapperClass =
  {
    NPRUNTIME_JSCLASS_NAME,
    JSCLASS_HAS_PRIVATE | JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE,
    NPObjWrapper_AddProperty, NPObjWrapper_DelProperty,
    NPObjWrapper_GetProperty, NPObjWrapper_SetProperty,
    (JSEnumerateOp)NPObjWrapper_newEnumerate,
    (JSResolveOp)NPObjWrapper_NewResolve, NPObjWrapper_Convert,
    NPObjWrapper_Finalize, nsnull, nsnull, NPObjWrapper_Call,
    NPObjWrapper_Construct, nsnull, nsnull
  };

typedef struct NPObjectMemberPrivate {
    JSObject *npobjWrapper;
    jsval fieldValue;
    NPIdentifier methodName;
    NPP   npp;
} NPObjectMemberPrivate;

static JSBool
NPObjectMember_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp);

static void
NPObjectMember_Finalize(JSContext *cx, JSObject *obj);

static JSBool
NPObjectMember_Call(JSContext *cx, uintN argc, jsval *vp);

static void
NPObjectMember_Trace(JSTracer *trc, JSObject *obj);

static JSClass sNPObjectMemberClass =
  {
    "NPObject Ambiguous Member class", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub,
    JS_PropertyStub, JS_StrictPropertyStub, JS_EnumerateStub,
    JS_ResolveStub, NPObjectMember_Convert,
    NPObjectMember_Finalize, nsnull, nsnull, NPObjectMember_Call,
    nsnull, nsnull, nsnull, NPObjectMember_Trace, nsnull
  };

static void
OnWrapperDestroyed();

static JSBool
DelayedReleaseGCCallback(JSContext* cx, JSGCStatus status)
{
  if (JSGC_END == status) {
    
    
    nsAutoPtr<nsTArray<NPObject*> > delayedReleases(sDelayedReleases);
    sDelayedReleases = nsnull;

    if (delayedReleases) {
      for (PRUint32 i = 0; i < delayedReleases->Length(); ++i) {
        NPObject* obj = (*delayedReleases)[i];
        if (obj)
          _releaseobject(obj);
        OnWrapperDestroyed();
      }
    }
  }
  return JS_TRUE;
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
    NS_ASSERTION(sJSRuntime != nsnull, "no JSRuntime?!");

    
    
    rtsvc->RegisterGCCallback(DelayedReleaseGCCallback);

    CallGetService("@mozilla.org/js/xpc/ContextStack;1", &sContextStack);
  }
}

static void
OnWrapperDestroyed()
{
  NS_ASSERTION(sWrapperCount, "Whaaa, unbalanced created/destroyed calls!");

  if (--sWrapperCount == 0) {
    if (sJSObjWrappers.ops) {
      NS_ASSERTION(sJSObjWrappers.entryCount == 0, "Uh, hash not empty?");

      
      
      PL_DHashTableFinish(&sJSObjWrappers);

      sJSObjWrappers.ops = nsnull;
    }

    if (sNPObjWrappers.ops) {
      NS_ASSERTION(sNPObjWrappers.entryCount == 0, "Uh, hash not empty?");

      
      
      PL_DHashTableFinish(&sNPObjWrappers);

      sNPObjWrappers.ops = nsnull;
    }

    
    sJSRuntime = nsnull;

    NS_IF_RELEASE(sContextStack);
  }
}

struct AutoCXPusher
{
  AutoCXPusher(JSContext *cx)
  {
    
    
    NS_PRECONDITION(sWrapperCount > 0,
                    "must have live wrappers when using AutoCXPusher");

    
    
    
    OnWrapperCreated();

    sContextStack->Push(cx);
  }

  ~AutoCXPusher()
  {
    JSContext *cx = nsnull;
    sContextStack->Pop(&cx);

    JSContext *currentCx = nsnull;
    sContextStack->Peek(&currentCx);

    if (!currentCx) {
      
      

      nsIScriptContext *scx = GetScriptContextFromJSContext(cx);

      if (scx) {
        scx->ScriptEvaluated(PR_TRUE);
      }
    }

    OnWrapperDestroyed();
  }
};

static JSContext *
GetJSContext(NPP npp)
{
  NS_ENSURE_TRUE(npp, nsnull);

  nsNPAPIPluginInstance *inst = (nsNPAPIPluginInstance *)npp->ndata;
  NS_ENSURE_TRUE(inst, nsnull);

  nsCOMPtr<nsIPluginInstanceOwner> owner;
  inst->GetOwner(getter_AddRefs(owner));
  NS_ENSURE_TRUE(owner, nsnull);

  nsCOMPtr<nsIDocument> doc;
  owner->GetDocument(getter_AddRefs(doc));
  NS_ENSURE_TRUE(doc, nsnull);

  nsCOMPtr<nsISupports> documentContainer = doc->GetContainer();
  nsCOMPtr<nsIScriptGlobalObject> sgo(do_GetInterface(documentContainer));
  NS_ENSURE_TRUE(sgo, nsnull);

  nsIScriptContext *scx = sgo->GetContext();
  NS_ENSURE_TRUE(scx, nsnull);

  return (JSContext *)scx->GetNativeContext();
}


static NPP
LookupNPP(NPObject *npobj);


static jsval
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
      
      
      jsval val;
      if (::JS_NewNumberValue(cx, NPVARIANT_TO_INT32(*variant), &val)) {
        return val;
      }

      break;
    }
  case NPVariantType_Double :
    {
      jsval val;
      if (::JS_NewNumberValue(cx, NPVARIANT_TO_DOUBLE(*variant), &val)) {
        return val;
      }

      break;
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
JSValToNPVariant(NPP npp, JSContext *cx, jsval val, NPVariant *variant)
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
      jsdouble d = JSVAL_TO_DOUBLE(val);
      jsint i;
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

      PRUint32 len;
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

  NPObject *npobj =
    nsJSObjWrapper::GetNewOrUsed(npp, cx, JSVAL_TO_OBJECT(val));
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
      ::JS_SetPendingException(cx, STRING_TO_JSVAL(str));
    }

    PopException();
  } else {
    ::JS_ReportError(cx, message);
  }
}

static JSBool
ReportExceptionIfPending(JSContext *cx)
{
  const char *ex = PeekException();

  if (!ex) {
    return JS_TRUE;
  }

  ThrowJSException(cx, nsnull);

  return JS_FALSE;
}


nsJSObjWrapper::nsJSObjWrapper(NPP npp)
  : nsJSObjWrapperKey(nsnull, npp)
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
    
    js_RemoveRoot(sJSRuntime, &jsnpobj->mJSObj);

    if (sJSObjWrappers.ops) {
      

      nsJSObjWrapperKey key(jsnpobj->mJSObj, jsnpobj->mNpp);
      PL_DHashTableOperate(&sJSObjWrappers, &key, PL_DHASH_REMOVE);
    }

    
    jsnpobj->mJSObj = nsnull;
  }
}

static JSBool
GetProperty(JSContext *cx, JSObject *obj, NPIdentifier id, jsval *rval)
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
    return PR_FALSE;
  }

  if (!npobj) {
    ThrowJSException(cx,
                     "Null npobj in nsJSObjWrapper::NP_HasMethod!");

    return PR_FALSE;
  }

  nsJSObjWrapper *npjsobj = (nsJSObjWrapper *)npobj;

  AutoCXPusher pusher(cx);
  JSAutoRequest ar(cx);
  JSAutoEnterCompartment ac;

  if (!ac.enter(cx, npjsobj->mJSObj))
    return PR_FALSE;

  AutoJSExceptionReporter reporter(cx);

  jsval v;
  JSBool ok = GetProperty(cx, npjsobj->mJSObj, id, &v);

  return ok && !JSVAL_IS_PRIMITIVE(v) &&
    ::JS_ObjectIsFunction(cx, JSVAL_TO_OBJECT(v));
}

static bool
doInvoke(NPObject *npobj, NPIdentifier method, const NPVariant *args,
         uint32_t argCount, PRBool ctorCall, NPVariant *result)
{
  NPP npp = NPPStack::Peek();
  JSContext *cx = GetJSContext(npp);

  if (!cx) {
    return PR_FALSE;
  }

  if (!npobj || !result) {
    ThrowJSException(cx, "Null npobj, or result in doInvoke!");

    return PR_FALSE;
  }

  
  VOID_TO_NPVARIANT(*result);

  nsJSObjWrapper *npjsobj = (nsJSObjWrapper *)npobj;
  jsval fv;

  AutoCXPusher pusher(cx);
  JSAutoRequest ar(cx);
  JSAutoEnterCompartment ac;

  if (!ac.enter(cx, npjsobj->mJSObj))
    return PR_FALSE;

  AutoJSExceptionReporter reporter(cx);

  if (method != NPIdentifier_VOID) {
    if (!GetProperty(cx, npjsobj->mJSObj, method, &fv) ||
        ::JS_TypeOfValue(cx, fv) != JSTYPE_FUNCTION) {
      return PR_FALSE;
    }
  } else {
    fv = OBJECT_TO_JSVAL(npjsobj->mJSObj);
  }

  jsval jsargs_buf[8];
  jsval *jsargs = jsargs_buf;

  if (argCount > (sizeof(jsargs_buf) / sizeof(jsval))) {
    
    
    jsargs = (jsval *)PR_Malloc(argCount * sizeof(jsval));
    if (!jsargs) {
      ::JS_ReportOutOfMemory(cx);

      return PR_FALSE;
    }
  }

  jsval v;
  JSBool ok;

  {
    js::AutoArrayRooter tvr(cx, 0, jsargs);

    
    for (PRUint32 i = 0; i < argCount; ++i) {
      jsargs[i] = NPVariantToJSVal(npp, cx, args + i);
      tvr.changeLength(i + 1);
    }

    if (ctorCall) {
      JSObject *newObj =
        ::JS_New(cx, npjsobj->mJSObj, argCount, jsargs);

      if (newObj) {
        v = OBJECT_TO_JSVAL(newObj);
        ok = JS_TRUE;
      } else {
        ok = JS_FALSE;
      }
    } else {
      ok = ::JS_CallFunctionValue(cx, npjsobj->mJSObj, fv, argCount, jsargs, &v);
    }

  }

  if (jsargs != jsargs_buf)
    PR_Free(jsargs);

  if (ok)
    ok = JSValToNPVariant(npp, cx, v, result);

  
  
  return ok == JS_TRUE;
}


bool
nsJSObjWrapper::NP_Invoke(NPObject *npobj, NPIdentifier method,
                          const NPVariant *args, uint32_t argCount,
                          NPVariant *result)
{
  if (method == NPIdentifier_VOID) {
    return PR_FALSE;
  }

  return doInvoke(npobj, method, args, argCount, PR_FALSE, result);
}


bool
nsJSObjWrapper::NP_InvokeDefault(NPObject *npobj, const NPVariant *args,
                                 uint32_t argCount, NPVariant *result)
{
  return doInvoke(npobj, NPIdentifier_VOID, args, argCount, PR_FALSE,
                  result);
}


bool
nsJSObjWrapper::NP_HasProperty(NPObject *npobj, NPIdentifier id)
{
  NPP npp = NPPStack::Peek();
  JSContext *cx = GetJSContext(npp);

  if (!cx) {
    return PR_FALSE;
  }

  if (!npobj) {
    ThrowJSException(cx,
                     "Null npobj in nsJSObjWrapper::NP_HasProperty!");

    return PR_FALSE;
  }

  nsJSObjWrapper *npjsobj = (nsJSObjWrapper *)npobj;
  JSBool found, ok = JS_FALSE;

  AutoCXPusher pusher(cx);
  JSAutoRequest ar(cx);
  AutoJSExceptionReporter reporter(cx);
  JSAutoEnterCompartment ac;

  if (!ac.enter(cx, npjsobj->mJSObj))
    return PR_FALSE;

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
    return PR_FALSE;
  }

  if (!npobj) {
    ThrowJSException(cx,
                     "Null npobj in nsJSObjWrapper::NP_GetProperty!");

    return PR_FALSE;
  }

  nsJSObjWrapper *npjsobj = (nsJSObjWrapper *)npobj;

  AutoCXPusher pusher(cx);
  JSAutoRequest ar(cx);
  AutoJSExceptionReporter reporter(cx);
  JSAutoEnterCompartment ac;

  if (!ac.enter(cx, npjsobj->mJSObj))
    return PR_FALSE;

  jsval v;
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
    return PR_FALSE;
  }

  if (!npobj) {
    ThrowJSException(cx,
                     "Null npobj in nsJSObjWrapper::NP_SetProperty!");

    return PR_FALSE;
  }

  nsJSObjWrapper *npjsobj = (nsJSObjWrapper *)npobj;
  JSBool ok = JS_FALSE;

  AutoCXPusher pusher(cx);
  JSAutoRequest ar(cx);
  AutoJSExceptionReporter reporter(cx);
  JSAutoEnterCompartment ac;

  if (!ac.enter(cx, npjsobj->mJSObj))
    return PR_FALSE;

  jsval v = NPVariantToJSVal(npp, cx, value);
  js::AutoValueRooter tvr(cx, v);

  NS_ASSERTION(NPIdentifierIsInt(id) || NPIdentifierIsString(id),
               "id must be either string or int!\n");
  ok = ::JS_SetPropertyById(cx, npjsobj->mJSObj, NPIdentifierToJSId(id), &v);

  
  
  return ok == JS_TRUE;
}


bool
nsJSObjWrapper::NP_RemoveProperty(NPObject *npobj, NPIdentifier id)
{
  NPP npp = NPPStack::Peek();
  JSContext *cx = GetJSContext(npp);

  if (!cx) {
    return PR_FALSE;
  }

  if (!npobj) {
    ThrowJSException(cx,
                     "Null npobj in nsJSObjWrapper::NP_RemoveProperty!");

    return PR_FALSE;
  }

  nsJSObjWrapper *npjsobj = (nsJSObjWrapper *)npobj;
  JSBool ok = JS_FALSE;

  AutoCXPusher pusher(cx);
  JSAutoRequest ar(cx);
  AutoJSExceptionReporter reporter(cx);
  jsval deleted = JSVAL_FALSE;
  JSAutoEnterCompartment ac;

  if (!ac.enter(cx, npjsobj->mJSObj))
    return PR_FALSE;

  NS_ASSERTION(NPIdentifierIsInt(id) || NPIdentifierIsString(id),
               "id must be either string or int!\n");
  ok = ::JS_DeletePropertyById2(cx, npjsobj->mJSObj, NPIdentifierToJSId(id), &deleted);
  if (ok && deleted == JSVAL_TRUE) {
    
    

    JSBool hasProp;
    ok = ::JS_HasPropertyById(cx, npjsobj->mJSObj, NPIdentifierToJSId(id), &hasProp);

    if (ok && hasProp) {
      
      

      deleted = JSVAL_FALSE;
    }
  }

  
  
  return ok == JS_TRUE && deleted == JSVAL_TRUE;
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
    return PR_FALSE;
  }

  if (!npobj) {
    ThrowJSException(cx,
                     "Null npobj in nsJSObjWrapper::NP_Enumerate!");

    return PR_FALSE;
  }

  nsJSObjWrapper *npjsobj = (nsJSObjWrapper *)npobj;

  AutoCXPusher pusher(cx);
  JSAutoRequest ar(cx);
  AutoJSExceptionReporter reporter(cx);
  JSAutoEnterCompartment ac;

  if (!ac.enter(cx, npjsobj->mJSObj))
    return PR_FALSE;

  JSIdArray *ida = ::JS_Enumerate(cx, npjsobj->mJSObj);
  if (!ida) {
    return PR_FALSE;
  }

  *count = ida->length;
  *idarray = (NPIdentifier *)PR_Malloc(*count * sizeof(NPIdentifier));
  if (!*idarray) {
    ThrowJSException(cx, "Memory allocation failed for NPIdentifier!");

    ::JS_DestroyIdArray(cx, ida);

    return PR_FALSE;
  }

  for (PRUint32 i = 0; i < *count; i++) {
    jsval v;
    if (!::JS_IdToValue(cx, ida->vector[i], &v)) {
      ::JS_DestroyIdArray(cx, ida);
      PR_Free(*idarray);
      return PR_FALSE;
    }

    NPIdentifier id;
    if (JSVAL_IS_STRING(v)) {
      JSString *str = JS_InternJSString(cx, JSVAL_TO_STRING(v));
      if (!str) {
          ::JS_DestroyIdArray(cx, ida);
          PR_Free(*idarray);

          return PR_FALSE;
      }
      id = StringToNPIdentifier(str);
    } else {
      NS_ASSERTION(JSVAL_IS_INT(v),
                   "The element in ida must be either string or int!\n");
      id = IntToNPIdentifier(JSVAL_TO_INT(v));
    }

    (*idarray)[i] = id;
  }

  ::JS_DestroyIdArray(cx, ida);

  return PR_TRUE;
}


bool
nsJSObjWrapper::NP_Construct(NPObject *npobj, const NPVariant *args,
                             uint32_t argCount, NPVariant *result)
{
  return doInvoke(npobj, NPIdentifier_VOID, args, argCount, PR_TRUE, result);
}


class JSObjWrapperHashEntry : public PLDHashEntryHdr
{
public:
  nsJSObjWrapper *mJSObjWrapper;
};


static PLDHashNumber
JSObjWrapperHash(PLDHashTable *table, const void *key)
{
  const nsJSObjWrapperKey *e = static_cast<const nsJSObjWrapperKey *>(key);

  return (PLDHashNumber)((PRWord)e->mJSObj ^ (PRWord)e->mNpp) >> 2;
}

static PRBool
JSObjWrapperHashMatchEntry(PLDHashTable *table, const PLDHashEntryHdr *entry,
                           const void *key)
{
  const nsJSObjWrapperKey *objWrapperKey =
    static_cast<const nsJSObjWrapperKey *>(key);
  const JSObjWrapperHashEntry *e =
    static_cast<const JSObjWrapperHashEntry *>(entry);

  return (e->mJSObjWrapper->mJSObj == objWrapperKey->mJSObj &&
          e->mJSObjWrapper->mNpp == objWrapperKey->mNpp);
}





NPObject *
nsJSObjWrapper::GetNewOrUsed(NPP npp, JSContext *cx, JSObject *obj)
{
  if (!npp) {
    NS_ERROR("Null NPP passed to nsJSObjWrapper::GetNewOrUsed()!");

    return nsnull;
  }

  if (!cx) {
    cx = GetJSContext(npp);

    if (!cx) {
      NS_ERROR("Unable to find a JSContext in nsJSObjWrapper::GetNewOrUsed()!");

      return nsnull;
    }
  }

  
  
  

  JSClass *clazz = JS_GET_CLASS(cx, obj);

  if (clazz == &sNPObjectJSWrapperClass) {
    
    

    NPObject *npobj = (NPObject *)::JS_GetPrivate(cx, obj);

    if (LookupNPP(npobj) == npp)
      return _retainobject(npobj);
  }

  if (!sJSObjWrappers.ops) {
    

    static PLDHashTableOps ops =
      {
        PL_DHashAllocTable,
        PL_DHashFreeTable,
        JSObjWrapperHash,
        JSObjWrapperHashMatchEntry,
        PL_DHashMoveEntryStub,
        PL_DHashClearEntryStub,
        PL_DHashFinalizeStub
      };

    if (!PL_DHashTableInit(&sJSObjWrappers, &ops, nsnull,
                           sizeof(JSObjWrapperHashEntry), 16)) {
      NS_ERROR("Error initializing PLDHashTable!");

      return nsnull;
    }
  }

  nsJSObjWrapperKey key(obj, npp);

  JSObjWrapperHashEntry *entry = static_cast<JSObjWrapperHashEntry *>
    (PL_DHashTableOperate(&sJSObjWrappers, &key, PL_DHASH_ADD));

  if (!entry) {
    
    return nsnull;
  }

  if (PL_DHASH_ENTRY_IS_BUSY(entry) && entry->mJSObjWrapper) {
    

    return _retainobject(entry->mJSObjWrapper);
  }

  

  nsJSObjWrapper *wrapper =
    (nsJSObjWrapper *)_createobject(npp, &sJSObjWrapperNPClass);

  if (!wrapper) {
    

    PL_DHashTableRawRemove(&sJSObjWrappers, entry);

    return nsnull;
  }

  wrapper->mJSObj = obj;

  entry->mJSObjWrapper = wrapper;

  NS_ASSERTION(wrapper->mNpp == npp, "nsJSObjWrapper::mNpp not initialized!");

  JSAutoRequest ar(cx);

  
  
  if (!::JS_AddNamedObjectRoot(cx, &wrapper->mJSObj, "nsJSObjWrapper::mJSObject")) {
    NS_ERROR("Failed to root JSObject!");

    _releaseobject(wrapper);

    PL_DHashTableRawRemove(&sJSObjWrappers, entry);

    return nsnull;
  }

  return wrapper;
}

static NPObject *
GetNPObject(JSContext *cx, JSObject *obj)
{
  while (obj && JS_GET_CLASS(cx, obj) != &sNPObjectJSWrapperClass) {
    obj = ::JS_GetPrototype(cx, obj);
  }

  if (!obj) {
    return nsnull;
  }

  return (NPObject *)::JS_GetPrivate(cx, obj);
}




static JSBool
NPObjWrapper_AddProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
  NPObject *npobj = GetNPObject(cx, obj);

  if (!npobj || !npobj->_class || !npobj->_class->hasProperty ||
      !npobj->_class->hasMethod) {
    ThrowJSException(cx, "Bad NPObject as private data!");

    return JS_FALSE;
  }

  if (NPObjectIsOutOfProcessProxy(npobj)) {
    return JS_TRUE;
  }

  PluginDestructionGuard pdg(LookupNPP(npobj));

  NPIdentifier identifier = JSIdToNPIdentifier(id);
  JSBool hasProperty = npobj->_class->hasProperty(npobj, identifier);
  if (!ReportExceptionIfPending(cx))
    return JS_FALSE;

  if (hasProperty)
    return JS_TRUE;

  
  
  JSBool hasMethod = npobj->_class->hasMethod(npobj, identifier);
  if (!ReportExceptionIfPending(cx))
    return JS_FALSE;

  if (!hasMethod) {
    ThrowJSException(cx, "Trying to add unsupported property on NPObject!");

    return JS_FALSE;
  }

  return JS_TRUE;
}

static JSBool
NPObjWrapper_DelProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
  NPObject *npobj = GetNPObject(cx, obj);

  if (!npobj || !npobj->_class || !npobj->_class->hasProperty ||
      !npobj->_class->removeProperty) {
    ThrowJSException(cx, "Bad NPObject as private data!");

    return JS_FALSE;
  }

  PluginDestructionGuard pdg(LookupNPP(npobj));

  NPIdentifier identifier = JSIdToNPIdentifier(id);

  if (!NPObjectIsOutOfProcessProxy(npobj)) {
    JSBool hasProperty = npobj->_class->hasProperty(npobj, identifier);
    if (!ReportExceptionIfPending(cx))
      return JS_FALSE;

    if (!hasProperty)
      return JS_TRUE;
  }

  if (!npobj->_class->removeProperty(npobj, identifier))
    *vp = JSVAL_FALSE;

  return ReportExceptionIfPending(cx);
}

static JSBool
NPObjWrapper_SetProperty(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
  NPObject *npobj = GetNPObject(cx, obj);

  if (!npobj || !npobj->_class || !npobj->_class->hasProperty ||
      !npobj->_class->setProperty) {
    ThrowJSException(cx, "Bad NPObject as private data!");

    return JS_FALSE;
  }

  
  
  NPP npp = LookupNPP(npobj);

  if (!npp) {
    ThrowJSException(cx, "No NPP found for NPObject!");

    return JS_FALSE;
  }

  PluginDestructionGuard pdg(npp);

  NPIdentifier identifier = JSIdToNPIdentifier(id);

  if (!NPObjectIsOutOfProcessProxy(npobj)) {
    JSBool hasProperty = npobj->_class->hasProperty(npobj, identifier);
    if (!ReportExceptionIfPending(cx))
      return JS_FALSE;

    if (!hasProperty) {
      ThrowJSException(cx, "Trying to set unsupported property on NPObject!");

      return JS_FALSE;
    }
  }

  NPVariant npv;
  if (!JSValToNPVariant(npp, cx, *vp, &npv)) {
    ThrowJSException(cx, "Error converting jsval to NPVariant!");

    return JS_FALSE;
  }

  JSBool ok = npobj->_class->setProperty(npobj, identifier, &npv);
  _releasevariantvalue(&npv); 
  if (!ReportExceptionIfPending(cx))
    return JS_FALSE;

  if (!ok) {
    ThrowJSException(cx, "Error setting property on NPObject!");

    return JS_FALSE;
  }

  return JS_TRUE;
}

static JSBool
NPObjWrapper_GetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
  NPObject *npobj = GetNPObject(cx, obj);

  if (!npobj || !npobj->_class || !npobj->_class->hasProperty ||
      !npobj->_class->hasMethod || !npobj->_class->getProperty) {
    ThrowJSException(cx, "Bad NPObject as private data!");

    return JS_FALSE;
  }

  
  
  NPP npp = LookupNPP(npobj);
  if (!npp) {
    ThrowJSException(cx, "No NPP found for NPObject!");

    return JS_FALSE;
  }

  PluginDestructionGuard pdg(npp);

  PRBool hasProperty, hasMethod;

  NPVariant npv;
  VOID_TO_NPVARIANT(npv);

  NPIdentifier identifier = JSIdToNPIdentifier(id);

  if (NPObjectIsOutOfProcessProxy(npobj)) {
    PluginScriptableObjectParent* actor =
      static_cast<ParentNPObject*>(npobj)->parent;

    
    if (!actor)
      return JS_FALSE;

    JSBool success = actor->GetPropertyHelper(identifier, &hasProperty,
                                              &hasMethod, &npv);
    if (!ReportExceptionIfPending(cx)) {
      if (success)
        _releasevariantvalue(&npv);
      return JS_FALSE;
    }

    if (success) {
      
      if (hasProperty && hasMethod)
        return CreateNPObjectMember(npp, cx, obj, npobj, id, &npv, vp);

      if (hasProperty) {
        *vp = NPVariantToJSVal(npp, cx, &npv);
        _releasevariantvalue(&npv);

        if (!ReportExceptionIfPending(cx))
          return JS_FALSE;
      }
    }
    return JS_TRUE;
  }

  hasProperty = npobj->_class->hasProperty(npobj, identifier);
  if (!ReportExceptionIfPending(cx))
    return JS_FALSE;

  hasMethod = npobj->_class->hasMethod(npobj, identifier);
  if (!ReportExceptionIfPending(cx))
    return JS_FALSE;

  
  if (hasProperty && hasMethod)
    return CreateNPObjectMember(npp, cx, obj, npobj, id, nsnull, vp);

  if (hasProperty) {
    if (npobj->_class->getProperty(npobj, identifier, &npv))
      *vp = NPVariantToJSVal(npp, cx, &npv);

    _releasevariantvalue(&npv);

    if (!ReportExceptionIfPending(cx))
      return JS_FALSE;
  }

  return JS_TRUE;
}

static JSBool
CallNPMethodInternal(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                     jsval *rval, PRBool ctorCall)
{
  while (obj && JS_GET_CLASS(cx, obj) != &sNPObjectJSWrapperClass) {
    obj = ::JS_GetPrototype(cx, obj);
  }

  if (!obj) {
    ThrowJSException(cx, "NPMethod called on non-NPObject wrapped JSObject!");

    return JS_FALSE;
  }

  NPObject *npobj = (NPObject *)::JS_GetPrivate(cx, obj);

  if (!npobj || !npobj->_class) {
    ThrowJSException(cx, "Bad NPObject as private data!");

    return JS_FALSE;
  }

  
  
  NPP npp = LookupNPP(npobj);

  if (!npp) {
    ThrowJSException(cx, "Error finding NPP for NPObject!");

    return JS_FALSE;
  }

  PluginDestructionGuard pdg(npp);

  NPVariant npargs_buf[8];
  NPVariant *npargs = npargs_buf;

  if (argc > (sizeof(npargs_buf) / sizeof(NPVariant))) {
    
    
    npargs = (NPVariant *)PR_Malloc(argc * sizeof(NPVariant));

    if (!npargs) {
      ThrowJSException(cx, "Out of memory!");

      return JS_FALSE;
    }
  }

  
  PRUint32 i;
  for (i = 0; i < argc; ++i) {
    if (!JSValToNPVariant(npp, cx, argv[i], npargs + i)) {
      ThrowJSException(cx, "Error converting jsvals to NPVariants!");

      if (npargs != npargs_buf) {
        PR_Free(npargs);
      }

      return JS_FALSE;
    }
  }

  NPVariant v;
  VOID_TO_NPVARIANT(v);

  JSObject *funobj = JSVAL_TO_OBJECT(argv[-2]);
  JSBool ok;
  const char *msg = "Error calling method on NPObject!";

  if (ctorCall) {
    
    

    if (NP_CLASS_STRUCT_VERSION_HAS_CTOR(npobj->_class) &&
        npobj->_class->construct) {
      ok = npobj->_class->construct(npobj, npargs, argc, &v);
    } else {
      ok = JS_FALSE;

      msg = "Attempt to construct object from class with no constructor.";
    }
  } else if (funobj != obj) {
    
    

    if (npobj->_class->invoke) {
      JSFunction *fun = (JSFunction *)::JS_GetPrivate(cx, funobj);
      JSString *name = ::JS_GetFunctionId(fun);
      NPIdentifier id = StringToNPIdentifier(name);

      ok = npobj->_class->invoke(npobj, id, npargs, argc, &v);
    } else {
      ok = JS_FALSE;

      msg = "Attempt to call a method on object with no invoke method.";
    }
  } else {
    if (npobj->_class->invokeDefault) {
      
      

      ok = npobj->_class->invokeDefault(npobj, npargs, argc, &v);
    } else {
      ok = JS_FALSE;

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

    return JS_FALSE;
  }

  *rval = NPVariantToJSVal(npp, cx, &v);

  
  _releasevariantvalue(&v);

  return ReportExceptionIfPending(cx);
}

static JSBool
CallNPMethod(JSContext *cx, uintN argc, jsval *vp)
{
  JSObject *obj = JS_THIS_OBJECT(cx, vp);
  if (!obj)
      return JS_FALSE;

  return CallNPMethodInternal(cx, obj, argc, JS_ARGV(cx, vp), vp, PR_FALSE);
}

struct NPObjectEnumerateState {
  PRUint32     index;
  PRUint32     length;
  NPIdentifier *value;
};

static JSBool
NPObjWrapper_newEnumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
                          jsval *statep, jsid *idp)
{
  NPObject *npobj = GetNPObject(cx, obj);
  NPIdentifier *enum_value;
  uint32_t length;
  NPObjectEnumerateState *state;

  if (!npobj || !npobj->_class) {
    ThrowJSException(cx, "Bad NPObject as private data!");
    return JS_FALSE;
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

      return JS_FALSE;
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

      return JS_FALSE;
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
      return JS_TRUE;
    }

    

  case JSENUMERATE_DESTROY:
    state = (NPObjectEnumerateState *)JSVAL_TO_PRIVATE(*statep);
    if (state->value)
      PR_Free(state->value);
    delete state;
    *statep = JSVAL_NULL;

    break;
  }

  return JS_TRUE;
}

static JSBool
NPObjWrapper_NewResolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
                        JSObject **objp)
{
  NPObject *npobj = GetNPObject(cx, obj);

  if (!npobj || !npobj->_class || !npobj->_class->hasProperty ||
      !npobj->_class->hasMethod) {
    ThrowJSException(cx, "Bad NPObject as private data!");

    return JS_FALSE;
  }

  PluginDestructionGuard pdg(LookupNPP(npobj));

  NPIdentifier identifier = JSIdToNPIdentifier(id);

  PRBool hasProperty = npobj->_class->hasProperty(npobj, identifier);
  if (!ReportExceptionIfPending(cx))
    return JS_FALSE;

  if (hasProperty) {
    NS_ASSERTION(JSID_IS_STRING(id) || JSID_IS_INT(id),
                 "id must be either string or int!\n");
    if (!::JS_DefinePropertyById(cx, obj, id, JSVAL_VOID, nsnull,
                                 nsnull, JSPROP_ENUMERATE)) {
        return JS_FALSE;
    }

    *objp = obj;

    return JS_TRUE;
  }

  PRBool hasMethod = npobj->_class->hasMethod(npobj, identifier);
  if (!ReportExceptionIfPending(cx))
    return JS_FALSE;

  if (hasMethod) {
    NS_ASSERTION(JSID_IS_STRING(id) || JSID_IS_INT(id),
                 "id must be either string or int!\n");

    JSFunction *fnc = ::JS_DefineFunctionById(cx, obj, id, CallNPMethod, 0,
                                              JSPROP_ENUMERATE);

    *objp = obj;

    return fnc != nsnull;
  }

  
  return JS_TRUE;
}

static JSBool
NPObjWrapper_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
  
  
  
  
  

  return JS_TRUE;
}

static void
NPObjWrapper_Finalize(JSContext *cx, JSObject *obj)
{
  NPObject *npobj = (NPObject *)::JS_GetPrivate(cx, obj);
  if (npobj) {
    if (sNPObjWrappers.ops) {
      PL_DHashTableOperate(&sNPObjWrappers, npobj, PL_DHASH_REMOVE);
    }
  }

  if (!sDelayedReleases)
    sDelayedReleases = new nsTArray<NPObject*>;
  sDelayedReleases->AppendElement(npobj);
}

static JSBool
NPObjWrapper_Call(JSContext *cx, uintN argc, jsval *vp)
{
  return CallNPMethodInternal(cx, JSVAL_TO_OBJECT(JS_CALLEE(cx, vp)), argc,
                              JS_ARGV(cx, vp), vp, PR_FALSE);
}

static JSBool
NPObjWrapper_Construct(JSContext *cx, uintN argc, jsval *vp)
{
  return CallNPMethodInternal(cx, JSVAL_TO_OBJECT(JS_CALLEE(cx, vp)), argc,
                              JS_ARGV(cx, vp), vp, PR_TRUE);
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
    
    

    JSContext *cx = GetJSContext(entry->mNpp);

    if (cx) {
      ::JS_SetPrivate(cx, entry->mJSObj, nsnull);
    }

    
    PL_DHashTableRawRemove(&sNPObjWrappers, entry);

    OnWrapperDestroyed();
  }
}




JSObject *
nsNPObjWrapper::GetNewOrUsed(NPP npp, JSContext *cx, NPObject *npobj)
{
  if (!npobj) {
    NS_ERROR("Null NPObject passed to nsNPObjWrapper::GetNewOrUsed()!");

    return nsnull;
  }

  if (npobj->_class == &nsJSObjWrapper::sJSObjWrapperNPClass) {
    

    return ((nsJSObjWrapper *)npobj)->mJSObj;
  }

  if (!npp) {
    NS_ERROR("No npp passed to nsNPObjWrapper::GetNewOrUsed()!");

    return nsnull;
  }

  if (!sNPObjWrappers.ops) {
    

    if (!PL_DHashTableInit(&sNPObjWrappers, PL_DHashGetStubOps(), nsnull,
                           sizeof(NPObjWrapperHashEntry), 16)) {
      NS_ERROR("Error initializing PLDHashTable!");

      return nsnull;
    }
  }

  NPObjWrapperHashEntry *entry = static_cast<NPObjWrapperHashEntry *>
    (PL_DHashTableOperate(&sNPObjWrappers, npobj, PL_DHASH_ADD));

  if (!entry) {
    
    JS_ReportOutOfMemory(cx);

    return nsnull;
  }

  if (PL_DHASH_ENTRY_IS_BUSY(entry) && entry->mJSObj) {
    
    return entry->mJSObj;
  }

  entry->mNPObj = npobj;
  entry->mNpp = npp;

  JSAutoRequest ar(cx);

  PRUint32 generation = sNPObjWrappers.generation;

  

  JSObject *obj = ::JS_NewObject(cx, &sNPObjectJSWrapperClass, nsnull, nsnull);

  if (generation != sNPObjWrappers.generation) {
      
      

      entry = static_cast<NPObjWrapperHashEntry *>
        (PL_DHashTableOperate(&sNPObjWrappers, npobj, PL_DHASH_LOOKUP));
      NS_ASSERTION(entry && PL_DHASH_ENTRY_IS_BUSY(entry),
                   "Hashtable didn't find what we just added?");
  }

  if (!obj) {
    

    PL_DHashTableRawRemove(&sNPObjWrappers, entry);

    return nsnull;
  }

  OnWrapperCreated();

  entry->mJSObj = obj;

  
  ::JS_SetPrivate(cx, obj, npobj);

  
  _retainobject(npobj);

  return obj;
}



static PLDHashOperator
JSObjWrapperPluginDestroyedCallback(PLDHashTable *table, PLDHashEntryHdr *hdr,
                                    PRUint32 number, void *arg)
{
  JSObjWrapperHashEntry *entry = (JSObjWrapperHashEntry *)hdr;

  nsJSObjWrapper *npobj = entry->mJSObjWrapper;

  if (npobj->mNpp == arg) {
    
    
    const PLDHashTableOps *ops = table->ops;
    table->ops = nsnull;

    if (npobj->_class && npobj->_class->invalidate) {
      npobj->_class->invalidate(npobj);
    }

    _releaseobject(npobj);

    table->ops = ops;

    return PL_DHASH_REMOVE;
  }

  return PL_DHASH_NEXT;
}



struct NppAndCx
{
  NPP npp;
  JSContext *cx;
};

static PLDHashOperator
NPObjWrapperPluginDestroyedCallback(PLDHashTable *table, PLDHashEntryHdr *hdr,
                                    PRUint32 number, void *arg)
{
  NPObjWrapperHashEntry *entry = (NPObjWrapperHashEntry *)hdr;
  NppAndCx *nppcx = reinterpret_cast<NppAndCx *>(arg);

  if (entry->mNpp == nppcx->npp) {
    
    
    const PLDHashTableOps *ops = table->ops;
    table->ops = nsnull;

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

    JSAutoEnterCompartment ac;
    if (ac.enter(nppcx->cx, entry->mJSObj)) {
      ::JS_SetPrivate(nppcx->cx, entry->mJSObj, nsnull);
    }

    table->ops = ops;    

    return PL_DHASH_REMOVE;
  }

  return PL_DHASH_NEXT;
}


void
nsJSNPRuntime::OnPluginDestroy(NPP npp)
{
  if (sJSObjWrappers.ops) {
    PL_DHashTableEnumerate(&sJSObjWrappers,
                           JSObjWrapperPluginDestroyedCallback, npp);
  }

  
  

  nsCOMPtr<nsIThreadJSContextStack> stack =
    do_GetService("@mozilla.org/js/xpc/ContextStack;1");
  if (!stack) {
    NS_ERROR("No context stack available!");

    return;
  }

  JSContext *cx;
  stack->GetSafeJSContext(&cx);
  if (!cx) {
    NS_ERROR("No safe JS context available!");

    return;
  }

  JSAutoRequest ar(cx);

  if (sNPObjWrappers.ops) {
    NppAndCx nppcx = { npp, cx };
    PL_DHashTableEnumerate(&sNPObjWrappers,
                           NPObjWrapperPluginDestroyedCallback, &nppcx);
  }

  
  
  
  
  if (!npp) {
    return;
  }

  
  
  nsNPAPIPluginInstance *inst = (nsNPAPIPluginInstance *)npp->ndata;
  if (!inst)
    return;

  nsCOMPtr<nsIDOMElement> element;
  inst->GetDOMElement(getter_AddRefs(element));
  if (!element)
    return;

  
  nsCOMPtr<nsIXPConnect> xpc(do_GetService(nsIXPConnect::GetCID()));
  if (!xpc)
    return;

  
  
  
  
  
  
  nsCOMPtr<nsIContent> content(do_QueryInterface(element));
  if (!content) {
    return;
  }

  nsIDocument* doc = content->GetOwnerDoc();
  if (!doc) {
    return;
  }

  nsIScriptGlobalObject* sgo = doc->GetScriptGlobalObject();
  if (!sgo) {
    return;
  }

  nsCOMPtr<nsIXPConnectWrappedNative> holder;
  xpc->GetWrappedNativeOfNativeObject(cx, sgo->GetGlobalJSObject(), content,
                                      NS_GET_IID(nsISupports),
                                      getter_AddRefs(holder));
  if (!holder) {
    return;
  }

  JSObject *obj, *proto;
  holder->GetJSObject(&obj);

  JSAutoEnterCompartment ac;

  if (obj && !ac.enter(cx, obj)) {
    
    return;
  }

  
  
  
  
  
  while (obj && (proto = ::JS_GetPrototype(cx, obj))) {
    if (JS_GET_CLASS(cx, proto) == &sNPObjectJSWrapperClass) {
      
      proto = ::JS_GetPrototype(cx, proto);

      
      ::JS_SetPrototype(cx, obj, proto);
    }

    obj = proto;
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
    return nsnull;
  }

  NS_ASSERTION(entry->mNpp, "Live NPObject entry w/o an NPP!");

  return entry->mNpp;
}

JSBool
CreateNPObjectMember(NPP npp, JSContext *cx, JSObject *obj, NPObject* npobj,
                     jsid id,  NPVariant* getPropertyResult, jsval *vp)
{
  NS_ENSURE_TRUE(vp, JS_FALSE);

  if (!npobj || !npobj->_class || !npobj->_class->getProperty ||
      !npobj->_class->invoke) {
    ThrowJSException(cx, "Bad NPObject");

    return JS_FALSE;
  }

  NPObjectMemberPrivate *memberPrivate =
    (NPObjectMemberPrivate *)PR_Malloc(sizeof(NPObjectMemberPrivate));
  if (!memberPrivate)
    return JS_FALSE;

  
  
  memset(memberPrivate, 0, sizeof(NPObjectMemberPrivate));

  JSObject *memobj = ::JS_NewObject(cx, &sNPObjectMemberClass, nsnull, nsnull);
  if (!memobj) {
    PR_Free(memberPrivate);
    return JS_FALSE;
  }

  *vp = OBJECT_TO_JSVAL(memobj);
  ::JS_AddValueRoot(cx, vp);

  ::JS_SetPrivate(cx, memobj, (void *)memberPrivate);

  NPIdentifier identifier = JSIdToNPIdentifier(id);

  jsval fieldValue;
  NPVariant npv;
  NPBool hasProperty;

  if (getPropertyResult) {
    
    npv = *getPropertyResult;
    hasProperty = true;
  }
  else {
    VOID_TO_NPVARIANT(npv);

    NPBool hasProperty = npobj->_class->getProperty(npobj, identifier,
                                                    &npv);
    if (!ReportExceptionIfPending(cx)) {
      ::JS_RemoveValueRoot(cx, vp);
      return JS_FALSE;
    }

    if (!hasProperty) {
      ::JS_RemoveValueRoot(cx, vp);
      return JS_FALSE;
    }
  }

  fieldValue = NPVariantToJSVal(npp, cx, &npv);

  
  
  
  while (JS_GET_CLASS(cx, obj) != &sNPObjectJSWrapperClass) {
    obj = ::JS_GetPrototype(cx, obj);
  }

  memberPrivate->npobjWrapper = obj;

  memberPrivate->fieldValue = fieldValue;
  memberPrivate->methodName = identifier;
  memberPrivate->npp = npp;

  ::JS_RemoveValueRoot(cx, vp);

  return JS_TRUE;
}

static JSBool
NPObjectMember_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
  NPObjectMemberPrivate *memberPrivate =
    (NPObjectMemberPrivate *)::JS_GetInstancePrivate(cx, obj,
                                                     &sNPObjectMemberClass,
                                                     nsnull);
  if (!memberPrivate) {
    NS_ERROR("no Ambiguous Member Private data!");
    return JS_FALSE;
  }

  switch (type) {
  case JSTYPE_VOID:
  case JSTYPE_STRING:
  case JSTYPE_NUMBER:
  case JSTYPE_BOOLEAN:
  case JSTYPE_OBJECT:
    *vp = memberPrivate->fieldValue;
    return JS_TRUE;
  case JSTYPE_FUNCTION:
    
    return JS_TRUE;
  default:
    NS_ERROR("illegal operation on JSObject prototype object");
    return JS_FALSE;
  }
}

static void
NPObjectMember_Finalize(JSContext *cx, JSObject *obj)
{
  NPObjectMemberPrivate *memberPrivate;

  memberPrivate = (NPObjectMemberPrivate *)::JS_GetPrivate(cx, obj);
  if (!memberPrivate)
    return;

  PR_Free(memberPrivate);
}

static JSBool
NPObjectMember_Call(JSContext *cx, uintN argc, jsval *vp)
{
  JSObject *memobj = JSVAL_TO_OBJECT(JS_CALLEE(cx, vp));
  NS_ENSURE_TRUE(memobj, JS_FALSE);

  NPObjectMemberPrivate *memberPrivate =
    (NPObjectMemberPrivate *)::JS_GetInstancePrivate(cx, memobj,
                                                     &sNPObjectMemberClass,
                                                     JS_ARGV(cx, vp));
  if (!memberPrivate || !memberPrivate->npobjWrapper)
    return JS_FALSE;

  NPObject *npobj = GetNPObject(cx, memberPrivate->npobjWrapper);
  if (!npobj) {
    ThrowJSException(cx, "Call on invalid member object");

    return JS_FALSE;
  }

  NPVariant npargs_buf[8];
  NPVariant *npargs = npargs_buf;

  if (argc > (sizeof(npargs_buf) / sizeof(NPVariant))) {
    
    
    npargs = (NPVariant *)PR_Malloc(argc * sizeof(NPVariant));

    if (!npargs) {
      ThrowJSException(cx, "Out of memory!");

      return JS_FALSE;
    }
  }

  
  PRUint32 i;
  jsval *argv = JS_ARGV(cx, vp);
  for (i = 0; i < argc; ++i) {
    if (!JSValToNPVariant(memberPrivate->npp, cx, argv[i], npargs + i)) {
      ThrowJSException(cx, "Error converting jsvals to NPVariants!");

      if (npargs != npargs_buf) {
        PR_Free(npargs);
      }

      return JS_FALSE;
    }
  }


  NPVariant npv;
  JSBool ok;
  ok = npobj->_class->invoke(npobj, memberPrivate->methodName,
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

    return JS_FALSE;
  }

  JS_SET_RVAL(cx, vp, NPVariantToJSVal(memberPrivate->npp, cx, &npv));

  
  _releasevariantvalue(&npv);

  return ReportExceptionIfPending(cx);
}

static void
NPObjectMember_Trace(JSTracer *trc, JSObject *obj)
{
  NPObjectMemberPrivate *memberPrivate =
    (NPObjectMemberPrivate *)::JS_GetInstancePrivate(trc->context, obj,
                                                     &sNPObjectMemberClass,
                                                     nsnull);
  if (!memberPrivate)
    return;

  if (!JSVAL_IS_PRIMITIVE(memberPrivate->fieldValue)) {
    JS_CALL_VALUE_TRACER(trc, memberPrivate->fieldValue,
                         "NPObject Member => fieldValue");
  }

  
  
  
  if (memberPrivate->npobjWrapper) {
    JS_CALL_OBJECT_TRACER(trc, memberPrivate->npobjWrapper,
                          "NPObject Member => npobjWrapper");
  }
}
