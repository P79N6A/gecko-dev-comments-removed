




































#include "nsJSNPRuntime.h"
#include "ns4xPlugin.h"
#include "ns4xPluginInstance.h"
#include "nsIPluginInstancePeer2.h"
#include "nsPIPluginInstancePeer.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptContext.h"
#include "nsIDocument.h"
#include "nsIJSRuntimeService.h"
#include "nsIJSContextStack.h"
#include "nsIXPConnect.h"
#include "nsIDOMElement.h"
#include "prmem.h"
#include "nsIContent.h"


#include "jscntxt.h"








static PLDHashTable sJSObjWrappers;


static PLDHashTable sNPObjWrappers;




static PRInt32 sWrapperCount;



static JSRuntime *sJSRuntime;



static nsIJSContextStack *sContextStack;

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
    nsJSObjWrapper::NP_Enumerate
  };

JS_STATIC_DLL_CALLBACK(JSBool)
NPObjWrapper_AddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

JS_STATIC_DLL_CALLBACK(JSBool)
NPObjWrapper_DelProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

JS_STATIC_DLL_CALLBACK(JSBool)
NPObjWrapper_SetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

JS_STATIC_DLL_CALLBACK(JSBool)
NPObjWrapper_GetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

JS_STATIC_DLL_CALLBACK(JSBool)
NPObjWrapper_newEnumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
                          jsval *statep, jsid *idp);

JS_STATIC_DLL_CALLBACK(JSBool)
NPObjWrapper_NewResolve(JSContext *cx, JSObject *obj, jsval id, uintN flags,
                        JSObject **objp);

JS_STATIC_DLL_CALLBACK(void)
NPObjWrapper_Finalize(JSContext *cx, JSObject *obj);

JS_STATIC_DLL_CALLBACK(JSBool)
NPObjWrapper_Call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                  jsval *rval);

static bool
CreateNPObjectMember(NPP npp, JSContext *cx, JSObject *obj,
                     NPObject *npobj, jsval id, jsval *vp);

static JSClass sNPObjectJSWrapperClass =
  {
    "NPObject JS wrapper class",
    JSCLASS_HAS_PRIVATE | JSCLASS_NEW_RESOLVE | JSCLASS_NEW_ENUMERATE,
    NPObjWrapper_AddProperty, NPObjWrapper_DelProperty,
    NPObjWrapper_GetProperty, NPObjWrapper_SetProperty,
    (JSEnumerateOp)NPObjWrapper_newEnumerate,
    (JSResolveOp)NPObjWrapper_NewResolve, JS_ConvertStub,
    NPObjWrapper_Finalize, nsnull, nsnull, NPObjWrapper_Call, nsnull, nsnull,
    nsnull
  };

typedef struct NPObjectMemberPrivate {
    JSObject *npobjWrapper;
    jsval fieldValue;
    jsval methodName;
    NPP   npp;
} NPObjectMemberPrivate;

JS_STATIC_DLL_CALLBACK(JSBool)
NPObjectMember_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp);

JS_STATIC_DLL_CALLBACK(void)
NPObjectMember_Finalize(JSContext *cx, JSObject *obj);

JS_STATIC_DLL_CALLBACK(JSBool)
NPObjectMember_Call(JSContext *cx, JSObject *obj, uintN argc,
                    jsval *argv, jsval *rval);

JS_STATIC_DLL_CALLBACK(uint32)
NPObjectMember_Mark(JSContext *cx, JSObject *obj, void *arg);

static JSClass sNPObjectMemberClass =
  {
    "NPObject Ambiguous Member class", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub,
    JS_PropertyStub, JS_PropertyStub, JS_EnumerateStub,
    JS_ResolveStub, NPObjectMember_Convert,
    NPObjectMember_Finalize, nsnull, nsnull, NPObjectMember_Call,
    nsnull, nsnull, nsnull, NPObjectMember_Mark, nsnull
  };

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
    sContextStack->Pop(nsnull);

    OnWrapperDestroyed();
  }
};

static JSContext *
GetJSContext(NPP npp)
{
  NS_ENSURE_TRUE(npp, nsnull);

  ns4xPluginInstance *inst = (ns4xPluginInstance *)npp->ndata;
  NS_ENSURE_TRUE(inst, nsnull);

  nsCOMPtr<nsPIPluginInstancePeer> pp(do_QueryInterface(inst->Peer()));
  NS_ENSURE_TRUE(pp, nsnull);

  nsCOMPtr<nsIPluginInstanceOwner> owner;
  pp->GetOwner(getter_AddRefs(owner));
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
    return INT_TO_JSVAL(NPVARIANT_TO_INT32(*variant));
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
      NS_ConvertUTF8toUTF16 utf16String(s->utf8characters, s->utf8length);

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
      DOUBLE_TO_NPVARIANT(*JSVAL_TO_DOUBLE(val), *variant);
    } else if (JSVAL_IS_STRING(val)) {
      JSString *jsstr = JSVAL_TO_STRING(val);
      nsDependentString str((PRUnichar *)::JS_GetStringChars(jsstr),
                            ::JS_GetStringLength(jsstr));

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
  OnWrapperCreated();
}

nsJSObjWrapper::~nsJSObjWrapper()
{
  
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
    
    ::JS_RemoveRootRT(sJSRuntime, &jsnpobj->mJSObj);

    if (sJSObjWrappers.ops) {
      

      nsJSObjWrapperKey key(jsnpobj->mJSObj, jsnpobj->mNpp);
      PL_DHashTableOperate(&sJSObjWrappers, &key, PL_DHASH_REMOVE);
    }

    
    jsnpobj->mJSObj = nsnull;
  }
}

static JSBool
GetProperty(JSContext *cx, JSObject *obj, NPIdentifier identifier, jsval *rval)
{
  jsval id = (jsval)identifier;

  AutoCXPusher pusher(cx);

  if (JSVAL_IS_STRING(id)) {
    JSString *str = JSVAL_TO_STRING(id);

    return ::JS_GetUCProperty(cx, obj, ::JS_GetStringChars(str),
                              ::JS_GetStringLength(str), rval);
  }

  NS_ASSERTION(JSVAL_IS_INT(id), "id must be either string or int!\n");

  return ::JS_GetElement(cx, obj, JSVAL_TO_INT(id), rval);
}


bool
nsJSObjWrapper::NP_HasMethod(NPObject *npobj, NPIdentifier identifier)
{
  NPP npp = NPPStack::Peek();
  JSContext *cx = GetJSContext(npp);

  if (!cx) {
    NS_ERROR("Null cx in nsJSObjWrapper::NP_HasMethod!");
    return PR_FALSE;
  }

  if (!npobj) {
    ThrowJSException(cx,
                     "Null npobj in nsJSObjWrapper::NP_HasMethod!");

    return PR_FALSE;
  }

  nsJSObjWrapper *npjsobj = (nsJSObjWrapper *)npobj;
  jsval v;
  JSAutoRequest ar(cx);
  JSBool ok = GetProperty(cx, npjsobj->mJSObj, identifier, &v);

  return ok && !JSVAL_IS_PRIMITIVE(v) &&
    ::JS_ObjectIsFunction(cx, JSVAL_TO_OBJECT(v));
}

static bool
doInvoke(NPObject *npobj, NPIdentifier method, const NPVariant *args,
         uint32_t argCount, NPVariant *result)
{
  NPP npp = NPPStack::Peek();
  JSContext *cx = GetJSContext(npp);

  if (!cx) {
    NS_ERROR("Null cx in doInvoke!");
    return PR_FALSE;
  }

  if (!npobj || !result) {
    ThrowJSException(cx, "Null npobj, or result in doInvoke!");

    return PR_FALSE;
  }

  JSAutoRequest ar(cx);

  
  VOID_TO_NPVARIANT(*result);

  nsJSObjWrapper *npjsobj = (nsJSObjWrapper *)npobj;
  jsval fv;

  AutoCXPusher pusher(cx);

  if ((jsval)method != JSVAL_VOID) {
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

  JSTempValueRooter tvr;
  JS_PUSH_TEMP_ROOT(cx, 0, jsargs, &tvr);

  
  for (PRUint32 i = 0; i < argCount; ++i) {
    jsargs[i] = NPVariantToJSVal(npp, cx, args + i);
    ++tvr.count;
  }

  jsval v;
  JSBool ok = ::JS_CallFunctionValue(cx, npjsobj->mJSObj, fv, argCount, jsargs,
                                     &v);

  JS_POP_TEMP_ROOT(cx, &tvr);

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
  if ((jsval)method == JSVAL_VOID) {
    return PR_FALSE;
  }

  return doInvoke(npobj, method, args, argCount, result);
}


bool
nsJSObjWrapper::NP_InvokeDefault(NPObject *npobj, const NPVariant *args,
                                 uint32_t argCount, NPVariant *result)
{
  return doInvoke(npobj, (NPIdentifier)JSVAL_VOID, args, argCount, result);
}


bool
nsJSObjWrapper::NP_HasProperty(NPObject *npobj, NPIdentifier identifier)
{
  NPP npp = NPPStack::Peek();
  JSContext *cx = GetJSContext(npp);

  if (!cx) {
    NS_ERROR("Null cx in nsJSObjWrapper::NP_HasProperty!");
    return PR_FALSE;
  }

  if (!npobj) {
    ThrowJSException(cx,
                     "Null npobj in nsJSObjWrapper::NP_HasProperty!");

    return PR_FALSE;
  }

  nsJSObjWrapper *npjsobj = (nsJSObjWrapper *)npobj;
  jsval id = (jsval)identifier;
  JSBool found, ok = JS_FALSE;

  JSAutoRequest ar(cx);

  if (JSVAL_IS_STRING(id)) {
    JSString *str = JSVAL_TO_STRING(id);

    ok = ::JS_HasUCProperty(cx, npjsobj->mJSObj, ::JS_GetStringChars(str),
                            ::JS_GetStringLength(str), &found);
  } else {
    NS_ASSERTION(JSVAL_IS_INT(id), "id must be either string or int!\n");

    ok = ::JS_HasElement(cx, npjsobj->mJSObj, JSVAL_TO_INT(id), &found);
  }

  return ok && found;
}


bool
nsJSObjWrapper::NP_GetProperty(NPObject *npobj, NPIdentifier identifier,
                               NPVariant *result)
{
  NPP npp = NPPStack::Peek();
  JSContext *cx = GetJSContext(npp);

  if (!cx) {
    NS_ERROR("Null cx in nsJSObjWrapper::NP_GetProperty!");
    return PR_FALSE;
  }

  if (!npobj) {
    ThrowJSException(cx,
                     "Null npobj in nsJSObjWrapper::NP_GetProperty!");

    return PR_FALSE;
  }

  nsJSObjWrapper *npjsobj = (nsJSObjWrapper *)npobj;

  AutoCXPusher pusher(cx);

  jsval v;
  JSAutoRequest ar(cx);
  return (GetProperty(cx, npjsobj->mJSObj, identifier, &v) &&
          JSValToNPVariant(npp, cx, v, result));
}


bool
nsJSObjWrapper::NP_SetProperty(NPObject *npobj, NPIdentifier identifier,
                               const NPVariant *value)
{
  NPP npp = NPPStack::Peek();
  JSContext *cx = GetJSContext(npp);

  if (!cx) {
    NS_ERROR("Null cx in nsJSObjWrapper::NP_SetProperty!");
    return PR_FALSE;
  }

  if (!npobj) {
    ThrowJSException(cx,
                     "Null npobj in nsJSObjWrapper::NP_SetProperty!");

    return PR_FALSE;
  }

  nsJSObjWrapper *npjsobj = (nsJSObjWrapper *)npobj;
  jsval id = (jsval)identifier;
  JSBool ok = JS_FALSE;

  AutoCXPusher pusher(cx);
  JSAutoRequest ar(cx);

  jsval v = NPVariantToJSVal(npp, cx, value);
  JSAutoTempValueRooter tvr(cx, v);

  if (JSVAL_IS_STRING(id)) {
    JSString *str = JSVAL_TO_STRING(id);

    ok = ::JS_SetUCProperty(cx, npjsobj->mJSObj, ::JS_GetStringChars(str),
                            ::JS_GetStringLength(str), &v);
  } else {
    NS_ASSERTION(JSVAL_IS_INT(id), "id must be either string or int!\n");

    ok = ::JS_SetElement(cx, npjsobj->mJSObj, JSVAL_TO_INT(id), &v);
  }

  
  
  return ok == JS_TRUE;
}


bool
nsJSObjWrapper::NP_RemoveProperty(NPObject *npobj, NPIdentifier identifier)
{
  NPP npp = NPPStack::Peek();
  JSContext *cx = GetJSContext(npp);

  if (!cx) {
    NS_ERROR("Null cx in nsJSObjWrapper::NP_RemoveProperty!");
    return PR_FALSE;
  }

  if (!npobj) {
    ThrowJSException(cx,
                     "Null npobj in nsJSObjWrapper::NP_RemoveProperty!");

    return PR_FALSE;
  }

  nsJSObjWrapper *npjsobj = (nsJSObjWrapper *)npobj;
  jsval id = (jsval)identifier;
  JSBool ok = JS_FALSE;

  AutoCXPusher pusher(cx);
  JSAutoRequest ar(cx);

  if (JSVAL_IS_STRING(id)) {
    JSString *str = JSVAL_TO_STRING(id);

    jsval unused;
    ok = ::JS_DeleteUCProperty2(cx, npjsobj->mJSObj, ::JS_GetStringChars(str),
                                ::JS_GetStringLength(str), &unused);
  } else {
    NS_ASSERTION(JSVAL_IS_INT(id), "id must be either string or int!\n");

    ok = ::JS_DeleteElement(cx, npjsobj->mJSObj, JSVAL_TO_INT(id));
  }

  
  
  return ok == JS_TRUE;
}


bool
nsJSObjWrapper::NP_Enumerate(NPObject *npobj, NPIdentifier **identifier,
                             uint32_t *count)
{
  NPP npp = NPPStack::Peek();
  JSContext *cx = GetJSContext(npp);

  *identifier = 0;
  *count = 0;

  if (!cx) {
    NS_ERROR("Null cx in nsJSObjWrapper::NP_Enumerate!");
    return PR_FALSE;
  }

  if (!npobj) {
    ThrowJSException(cx,
                     "Null npobj in nsJSObjWrapper::NP_Enumerate!");

    return PR_FALSE;
  }

  nsJSObjWrapper *npjsobj = (nsJSObjWrapper *)npobj;

  AutoCXPusher pusher(cx);

  JSIdArray *ida = ::JS_Enumerate(cx, npjsobj->mJSObj);
  if (!ida) {
    return PR_FALSE;
  }

  *count = ida->length;
  *identifier = (NPIdentifier *)PR_Malloc(*count * sizeof(NPIdentifier));
  if (!*identifier) {
    ThrowJSException(cx, "Memory allocation failed for NPIdentifier!");

    ::JS_DestroyIdArray(cx, ida);

    return PR_FALSE;
  }

  for (PRUint32 i = 0; i < *count; i++) {
    jsval v;
    if (!::JS_IdToValue(cx, ida->vector[i], &v)) {
      ::JS_DestroyIdArray(cx, ida);
      PR_Free(*identifier);
      return PR_FALSE;
    }

    if (JSVAL_IS_STRING(v)) {
      JSString *str = JSVAL_TO_STRING(v);

      if (!JS_InternUCStringN(cx, ::JS_GetStringChars(str),
                              ::JS_GetStringLength(str))) {
        ::JS_DestroyIdArray(cx, ida);
        PR_Free(*identifier);

        return PR_FALSE;
      }
    } else {
      NS_ASSERTION(JSVAL_IS_INT(v),
                   "The element in ida must be either string or int!\n");
    }

    (*identifier)[i] = (NPIdentifier)v;
  }

  ::JS_DestroyIdArray(cx, ida);

  return PR_TRUE;
}

class JSObjWrapperHashEntry : public PLDHashEntryHdr
{
public:
  nsJSObjWrapper *mJSObjWrapper;
};


PR_STATIC_CALLBACK(PLDHashNumber)
JSObjWrapperHash(PLDHashTable *table, const void *key)
{
  const nsJSObjWrapperKey *e = static_cast<const nsJSObjWrapperKey *>(key);

  return (PLDHashNumber)((PRWord)e->mJSObj ^ (PRWord)e->mNpp) >> 2;
}

PR_STATIC_CALLBACK(PRBool)
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
      NS_ERROR("Unable to find a JSContext in "
               "nsJSObjWrapper::GetNewOrUsed()!");

      return nsnull;
    }
  }

  JSClass *clazz = JS_GET_CLASS(cx, obj);

  if (clazz == &sNPObjectJSWrapperClass) {
    
    

    NPObject *npobj = (NPObject *)::JS_GetPrivate(cx, obj);

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

  JSObjWrapperHashEntry *entry =
    static_cast<JSObjWrapperHashEntry *>
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

  
  
  if (!::JS_AddNamedRoot(cx, &wrapper->mJSObj, "nsJSObjWrapper::mJSObject")) {
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

JS_STATIC_DLL_CALLBACK(JSBool)
NPObjWrapper_AddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  NPObject *npobj = GetNPObject(cx, obj);

  if (!npobj || !npobj->_class || !npobj->_class->hasProperty ||
      !npobj->_class->hasMethod) {
    ThrowJSException(cx, "Bad NPObject as private data!");

    return JS_FALSE;
  }

  
  
  if (!npobj->_class->hasProperty(npobj, (NPIdentifier)id) &&
      !npobj->_class->hasMethod(npobj, (NPIdentifier)id)) {
    ThrowJSException(cx, "Trying to add unsupported property on scriptable "
                     "plugin object!");

    return JS_FALSE;
  }

  return ReportExceptionIfPending(cx);
}

JS_STATIC_DLL_CALLBACK(JSBool)
NPObjWrapper_DelProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  NPObject *npobj = GetNPObject(cx, obj);

  if (!npobj || !npobj->_class || !npobj->_class->hasProperty) {
    ThrowJSException(cx, "Bad NPObject as private data!");

    return JS_FALSE;
  }

  if (!npobj->_class->hasProperty(npobj, (NPIdentifier)id)) {
    ThrowJSException(cx, "Trying to remove unsupported property on scriptable "
                     "plugin object!");

    return JS_FALSE;
  }

  return ReportExceptionIfPending(cx);
}

JS_STATIC_DLL_CALLBACK(JSBool)
NPObjWrapper_SetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  NPObject *npobj = GetNPObject(cx, obj);

  if (!npobj || !npobj->_class || !npobj->_class->hasProperty ||
      !npobj->_class->setProperty) {
    ThrowJSException(cx, "Bad NPObject as private data!");

    return JS_FALSE;
  }

  if (!npobj->_class->hasProperty(npobj, (NPIdentifier)id)) {
    ThrowJSException(cx, "Trying to set unsupported property on scriptable "
                     "plugin object!");

    return JS_FALSE;
  }

  
  
  NPP npp = LookupNPP(npobj);

  if (!npp) {
    ThrowJSException(cx, "No NPP found for NPObject!");

    return JS_FALSE;
  }

  NPVariant npv;
  if (!JSValToNPVariant(npp, cx, *vp, &npv)) {
    ThrowJSException(cx, "Error converting jsval to NPVariant!");

    return JS_FALSE;
  }

  JSBool ok = npobj->_class->setProperty(npobj, (NPIdentifier)id, &npv);

  
  _releasevariantvalue(&npv);

  if (!ok) {
    ThrowJSException(cx, "Error setting property on scriptable plugin "
                     "object!");

    return JS_FALSE;
  }

  return ReportExceptionIfPending(cx);
}

JS_STATIC_DLL_CALLBACK(JSBool)
NPObjWrapper_GetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  NPObject *npobj = GetNPObject(cx, obj);

  if (!npobj || !npobj->_class || !npobj->_class->hasProperty ||
      !npobj->_class->hasMethod || !npobj->_class->getProperty) {
    ThrowJSException(cx, "Bad NPObject as private data!");

    return JS_FALSE;
  }

  PRBool hasProperty = npobj->_class->hasProperty(npobj, (NPIdentifier)id);
  PRBool hasMethod = npobj->_class->hasMethod(npobj, (NPIdentifier)id);
  NPP npp = nsnull;
  if (hasProperty) {
    
    
    
    npp = LookupNPP(npobj);
    if (!npp) {
      ThrowJSException(cx, "No NPP found for NPObject!");

      return JS_FALSE;
    }
  }

  
  if (hasProperty && hasMethod)
    return CreateNPObjectMember(npp, cx, obj, npobj, id, vp);

  if (hasProperty) {
    NPVariant npv;
    VOID_TO_NPVARIANT(npv);

    if (!npobj->_class->getProperty(npobj, (NPIdentifier)id, &npv)) {
      ThrowJSException(cx, "Error setting property on scriptable plugin "
                       "object!");

      return JS_FALSE;
    }

    *vp = NPVariantToJSVal(npp, cx, &npv);

    
    _releasevariantvalue(&npv);

    return JS_TRUE;
  }

  if (!hasMethod) {
    ThrowJSException(cx, "Trying to get unsupported property on scriptable "
                     "plugin object!");

    return JS_FALSE;
  }

  return ReportExceptionIfPending(cx);
}

JS_STATIC_DLL_CALLBACK(JSBool)
CallNPMethod(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
             jsval *rval)
{
  while (obj && JS_GET_CLASS(cx, obj) != &sNPObjectJSWrapperClass) {
    obj = ::JS_GetPrototype(cx, obj);
  }

  if (!obj) {
    ThrowJSException(cx, "NPMethod called on non-NPObject wrapped JSObject!");

    return JS_FALSE;
  }

  NPObject *npobj = (NPObject *)::JS_GetPrivate(cx, obj);

  if (!npobj || !npobj->_class || !npobj->_class->invoke) {
    ThrowJSException(cx, "Bad NPObject as private data!");

    return JS_FALSE;
  }

  
  
  NPP npp = LookupNPP(npobj);

  if (!npp) {
    ThrowJSException(cx, "Error finding NPP for NPObject!");

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

  if (funobj != obj) {
    
    

    JSFunction *fun = (JSFunction *)::JS_GetPrivate(cx, funobj);
    jsval method = STRING_TO_JSVAL(::JS_GetFunctionId(fun));

    ok = npobj->_class->invoke(npobj, (NPIdentifier)method, npargs, argc, &v);
  } else {
    
    

    ok = npobj->_class->invokeDefault(npobj, npargs, argc, &v);
  }

  
  for (i = 0; i < argc; ++i) {
    _releasevariantvalue(npargs + i);
  }

  if (npargs != npargs_buf) {
    PR_Free(npargs);
  }

  if (!ok) {
    ThrowJSException(cx, "Error calling method on NPObject!");

    return JS_FALSE;
  }

  *rval = NPVariantToJSVal(npp, cx, &v);

  
  _releasevariantvalue(&v);

  return ReportExceptionIfPending(cx);
}

struct NPObjectEnumerateState {
  PRUint32     index;
  PRUint32     length;
  NPIdentifier *value;
};

JS_STATIC_DLL_CALLBACK(JSBool)
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

  NS_ASSERTION(statep, "Must have a statep to enumerate!");

  switch(enum_op) {
  case JSENUMERATE_INIT:
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
      ThrowJSException(cx, "Error enumerating properties on scriptable "
                       "plugin object");
      delete state;

      return JS_FALSE;
    }

    state->value = enum_value;
    state->length = length;
    state->index = 0;
    *statep = PRIVATE_TO_JSVAL(state);
    if (idp) {
      *idp = INT_TO_JSVAL(length);
    }

    break;

  case JSENUMERATE_NEXT:
    state = (NPObjectEnumerateState *)JSVAL_TO_PRIVATE(*statep);
    enum_value = state->value;
    length = state->length;
    if (state->index != length) {
      return ::JS_ValueToId(cx, (jsval)enum_value[state->index++], idp);
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

JS_STATIC_DLL_CALLBACK(JSBool)
NPObjWrapper_NewResolve(JSContext *cx, JSObject *obj, jsval id, uintN flags,
                        JSObject **objp)
{
  NPObject *npobj = GetNPObject(cx, obj);

  if (!npobj || !npobj->_class || !npobj->_class->hasProperty ||
      !npobj->_class->hasMethod) {
    ThrowJSException(cx, "Bad NPObject as private data!");

    return JS_FALSE;
  }

  if (npobj->_class->hasProperty(npobj, (NPIdentifier)id)) {
    JSBool ok;

    if (JSVAL_IS_STRING(id)) {
      JSString *str = JSVAL_TO_STRING(id);

      ok = ::JS_DefineUCProperty(cx, obj, ::JS_GetStringChars(str),
                                 ::JS_GetStringLength(str), JSVAL_VOID, nsnull,
                                 nsnull, JSPROP_ENUMERATE);
    } else {
      ok = ::JS_DefineElement(cx, obj, JSVAL_TO_INT(id), JSVAL_VOID, nsnull,
                              nsnull, JSPROP_ENUMERATE);
    }

    if (!ok) {
      return JS_FALSE;
    }

    *objp = obj;
  } else if (npobj->_class->hasMethod(npobj, (NPIdentifier)id)) {
    JSString *str = nsnull;

    if (JSVAL_IS_STRING(id)) {
      str = JSVAL_TO_STRING(id);
    } else {
      NS_ASSERTION(JSVAL_IS_INT(id), "id must be either string or int!\n");

      str = ::JS_ValueToString(cx, id);

      if (!str) {
        

        return JS_FALSE;
      }
    }

    JSFunction *fnc =
      ::JS_DefineUCFunction(cx, obj, ::JS_GetStringChars(str),
                            ::JS_GetStringLength(str), CallNPMethod, 0,
                            JSPROP_ENUMERATE);

    *objp = obj;

    return fnc != nsnull;
  }

  return ReportExceptionIfPending(cx);
}

JS_STATIC_DLL_CALLBACK(void)
NPObjWrapper_Finalize(JSContext *cx, JSObject *obj)
{
  NPObject *npobj = (NPObject *)::JS_GetPrivate(cx, obj);

  if (npobj) {
    if (sNPObjWrappers.ops) {
      PL_DHashTableOperate(&sNPObjWrappers, npobj, PL_DHASH_REMOVE);
    }

    
    _releaseobject(npobj);
  }

  OnWrapperDestroyed();
}

JS_STATIC_DLL_CALLBACK(JSBool)
NPObjWrapper_Call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                  jsval *rval)
{
  return CallNPMethod(cx, JSVAL_TO_OBJECT(argv[-2]), argc, argv, rval);
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

  NPObjWrapperHashEntry *entry =
    NS_STATIC_CAST(NPObjWrapperHashEntry *,
                   PL_DHashTableOperate(&sNPObjWrappers, npobj,
                                        PL_DHASH_LOOKUP));

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

  NPObjWrapperHashEntry *entry =
    static_cast<NPObjWrapperHashEntry *>
               (PL_DHashTableOperate(&sNPObjWrappers, npobj,
                                        PL_DHASH_ADD));
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

  

  JSObject *obj = ::JS_NewObject(cx, &sNPObjectJSWrapperClass, nsnull, nsnull);

  if (!obj) {
    

    PL_DHashTableRawRemove(&sJSObjWrappers, entry);

    return nsnull;
  }

  OnWrapperCreated();

  entry->mJSObj = obj;

  if (!::JS_SetPrivate(cx, obj, npobj)) {
    NS_ERROR("Error setting private NPObject data in JS wrapper!");

    PL_DHashTableRawRemove(&sJSObjWrappers, entry);

    return nsnull;
  }

  
  _retainobject(npobj);

  return obj;
}



PR_STATIC_CALLBACK(PLDHashOperator)
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

PR_STATIC_CALLBACK(PLDHashOperator)
NPObjWrapperPluginDestroyedCallback(PLDHashTable *table, PLDHashEntryHdr *hdr,
                                    PRUint32 number, void *arg)
{
  NPObjWrapperHashEntry *entry = (NPObjWrapperHashEntry *)hdr;
  NppAndCx *nppcx = reinterpret_cast<NppAndCx *>(arg);

  if (entry->mNpp == nppcx->npp) {
    NPObject *npobj = entry->mNPObj;

    if (npobj->_class && npobj->_class->invalidate) {
      npobj->_class->invalidate(npobj);
    }

    
    
    if (npobj->_class && npobj->_class->deallocate) {
      npobj->_class->deallocate(npobj);
    } else {
      PR_Free(npobj);
    }

    ::JS_SetPrivate(nppcx->cx, entry->mJSObj, nsnull);

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

  if (sNPObjWrappers.ops) {
    NppAndCx nppcx = { npp, cx };
    PL_DHashTableEnumerate(&sNPObjWrappers,
                           NPObjWrapperPluginDestroyedCallback, &nppcx);
  }

  
  
  
  
  if (!npp) {
    return;
  }

  
  
  ns4xPluginInstance *inst = (ns4xPluginInstance *)npp->ndata;
  if (!inst) {
    return;
  }

  nsCOMPtr<nsIPluginInstancePeer> pip;
  inst->GetPeer(getter_AddRefs(pip));
  nsCOMPtr<nsIPluginTagInfo2> pti2(do_QueryInterface(pip));
  if (!pti2) {
    return;
  }

  nsCOMPtr<nsIDOMElement> element;
  pti2->GetDOMElement(getter_AddRefs(element));
  if (!element) {
    return;
  }

  
  nsCOMPtr<nsIXPConnect> xpc(do_GetService(nsIXPConnect::GetCID()));
  if (!xpc) {
    return;
  }

  
  
  
  
  
  
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

  JSObject* globalObj = sgo->GetGlobalJSObject();

#ifdef DEBUG
  nsIScriptContext* scx = sgo->GetContext();
  if (scx) {
    NS_ASSERTION((JSContext *)scx->GetNativeContext() == cx,
                 "Unexpected JS context");
  }
#endif

  nsCOMPtr<nsISupports> supp(do_QueryInterface(element));
  nsCOMPtr<nsIXPConnectWrappedNative> holder;
  xpc->GetWrappedNativeOfNativeObject(cx, globalObj, supp,
                                      NS_GET_IID(nsISupports),
                                      getter_AddRefs(holder));
  if (!holder) {
    return;
  }

  JSObject *obj, *proto;
  holder->GetJSObject(&obj);

  
  
  
  
  
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
    NS_ERROR("NPP requested for NPObject of class "
             "nsJSObjWrapper::sJSObjWrapperNPClass!\n");

    return nsnull;
  }



  NPObjWrapperHashEntry *entry =
    static_cast<NPObjWrapperHashEntry *>
               (PL_DHashTableOperate(&sNPObjWrappers, npobj,
                                        PL_DHASH_ADD));

  if (PL_DHASH_ENTRY_IS_FREE(entry)) {
    return nsnull;
  }

  NS_ASSERTION(entry->mNpp, "Live NPObject entry w/o an NPP!");

  return entry->mNpp;
}

bool
CreateNPObjectMember(NPP npp, JSContext *cx, JSObject *obj,
                     NPObject* npobj, jsval id, jsval *vp)
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

  JSObject *memobj = ::JS_NewObject(cx, &sNPObjectMemberClass, nsnull, nsnull);
  if (!memobj) {
    PR_Free(memberPrivate);
    return false;
  }

  *vp = OBJECT_TO_JSVAL(memobj);
  ::JS_AddRoot(cx, vp);

  ::JS_SetPrivate(cx, memobj, (void *)memberPrivate);

  jsval fieldValue;
  NPVariant npv;
  VOID_TO_NPVARIANT(npv);
  if (!npobj->_class->getProperty(npobj, (NPIdentifier)id, &npv)) {
    ::JS_RemoveRoot(cx, vp);
    return false;
  }

  fieldValue = NPVariantToJSVal(npp, cx, &npv);

  
  
  
  while (JS_GET_CLASS(cx, obj) != &sNPObjectJSWrapperClass) {
    obj = ::JS_GetPrototype(cx, obj);
  }

  memberPrivate->npobjWrapper = obj;

  memberPrivate->fieldValue = fieldValue;
  memberPrivate->methodName = id;
  memberPrivate->npp = npp;

  ::JS_RemoveRoot(cx, vp);

  return true;
}

JS_STATIC_DLL_CALLBACK(JSBool)
NPObjectMember_Convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
  NPObjectMemberPrivate *memberPrivate =
    (NPObjectMemberPrivate *)::JS_GetInstancePrivate(cx, obj,
                                                     &sNPObjectMemberClass,
                                                     nsnull);
  NS_ASSERTION(memberPrivate, "no Ambiguous Member Private data!");

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

JS_STATIC_DLL_CALLBACK(void)
NPObjectMember_Finalize(JSContext *cx, JSObject *obj)
{
  NPObjectMemberPrivate *memberPrivate;

  memberPrivate = (NPObjectMemberPrivate *)::JS_GetPrivate(cx, obj);
  if (!memberPrivate)
    return;

  PR_Free(memberPrivate);
}

JS_STATIC_DLL_CALLBACK(JSBool)
NPObjectMember_Call(JSContext *cx, JSObject *obj,
                    uintN argc, jsval *argv, jsval *rval)
{
  JSObject *memobj = JSVAL_TO_OBJECT(argv[-2]);
  NS_ENSURE_TRUE(memobj, JS_FALSE);

  NPObjectMemberPrivate *memberPrivate =
    (NPObjectMemberPrivate *)::JS_GetInstancePrivate(cx, memobj,
                                                     &sNPObjectMemberClass,
                                                     argv);
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
  ok = npobj->_class->invoke(npobj, (NPIdentifier)memberPrivate->methodName,
                             npargs, argc, &npv);

  
  for (i = 0; i < argc; ++i) {
    _releasevariantvalue(npargs + i);
  }

  if (npargs != npargs_buf) {
    PR_Free(npargs);
  }

  if (!ok) {
    ThrowJSException(cx, "Error calling method on NPObject!");

    return JS_FALSE;
  }

  *rval = NPVariantToJSVal(memberPrivate->npp, cx, &npv);

  
  _releasevariantvalue(&npv);

  return ReportExceptionIfPending(cx);
}

JS_STATIC_DLL_CALLBACK(uint32)
NPObjectMember_Mark(JSContext *cx, JSObject *obj, void *arg)
{
  NPObjectMemberPrivate *memberPrivate =
    (NPObjectMemberPrivate *)::JS_GetInstancePrivate(cx, obj,
                                                     &sNPObjectMemberClass,
                                                     nsnull);
  if (!memberPrivate)
    return 0;

  if (!JSVAL_IS_PRIMITIVE(memberPrivate->fieldValue)) {
    ::JS_MarkGCThing(cx, JSVAL_TO_OBJECT(memberPrivate->fieldValue),
                     "NPObject Member => fieldValue", arg);
  }

  
  
  
  if (memberPrivate->npobjWrapper) {
    ::JS_MarkGCThing(cx, memberPrivate->npobjWrapper,
                     "NPObject Member => npobjWrapper", arg);
  }

  return 0;
}
