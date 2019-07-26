





#include "mozilla/dom/HTMLAllCollection.h"

#include "jsapi.h"
#include "mozilla/HoldDropJSObjects.h"
#include "nsContentUtils.h"
#include "nsDOMClassInfo.h"
#include "nsHTMLDocument.h"
#include "nsJSUtils.h"
#include "nsWrapperCacheInlines.h"
#include "xpcpublic.h"

using namespace mozilla;
using namespace mozilla::dom;

class nsHTMLDocumentSH
{
public:
  static bool DocumentAllGetProperty(JSContext *cx, JS::Handle<JSObject*> obj, JS::Handle<jsid> id,
                                     JS::MutableHandle<JS::Value> vp);
  static bool DocumentAllNewResolve(JSContext *cx, JS::Handle<JSObject*> obj, JS::Handle<jsid> id,
                                    unsigned flags, JS::MutableHandle<JSObject*> objp);
  static void ReleaseDocument(JSFreeOp *fop, JSObject *obj);
  static bool CallToGetPropMapper(JSContext *cx, unsigned argc, JS::Value *vp);
};

const JSClass sHTMLDocumentAllClass = {
  "HTML document.all class",
  JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS | JSCLASS_NEW_RESOLVE |
  JSCLASS_EMULATES_UNDEFINED,
  JS_PropertyStub,                                         
  JS_DeletePropertyStub,                                   
  nsHTMLDocumentSH::DocumentAllGetProperty,                
  JS_StrictPropertyStub,                                   
  JS_EnumerateStub,
  (JSResolveOp)nsHTMLDocumentSH::DocumentAllNewResolve,
  JS_ConvertStub,
  nsHTMLDocumentSH::ReleaseDocument,
  nsHTMLDocumentSH::CallToGetPropMapper
};

namespace mozilla {
namespace dom {

HTMLAllCollection::HTMLAllCollection(nsHTMLDocument* aDocument)
  : mDocument(aDocument)
{
  MOZ_ASSERT(mDocument);
  mozilla::HoldJSObjects(this);
}

HTMLAllCollection::~HTMLAllCollection()
{
  mObject = nullptr;
  mozilla::DropJSObjects(this);
}

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(HTMLAllCollection, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(HTMLAllCollection, Release)

NS_IMPL_CYCLE_COLLECTION_CLASS(HTMLAllCollection)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(HTMLAllCollection)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDocument)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mCollection)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(HTMLAllCollection)
  tmp->mObject = nullptr;
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mDocument)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mCollection)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(HTMLAllCollection)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mObject)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

JSObject*
HTMLAllCollection::GetObject(JSContext* aCx, ErrorResult& aRv)
{
  MOZ_ASSERT(aCx);

  if (!mObject) {
    JS::Rooted<JSObject*> wrapper(aCx, mDocument->GetWrapper());
    MOZ_ASSERT(wrapper);

    JSAutoCompartment ac(aCx, wrapper);
    JS::Rooted<JSObject*> global(aCx, JS_GetGlobalForObject(aCx, wrapper));
    mObject = JS_NewObject(aCx, &sHTMLDocumentAllClass, JS::NullPtr(), global);
    if (!mObject) {
      aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
      return nullptr;
    }

    
    JS_SetPrivate(mObject, ToSupports(mDocument));
    NS_ADDREF(mDocument);
  }

  JS::ExposeObjectToActiveJS(mObject);
  return mObject;
}

nsContentList*
HTMLAllCollection::Collection()
{
  if (!mCollection) {
    nsIDocument* document = mDocument;
    mCollection = document->GetElementsByTagName(NS_LITERAL_STRING("*"));
    MOZ_ASSERT(mCollection);
  }
  return mCollection;
}

} 
} 

static nsHTMLDocument*
GetDocument(JSObject *obj)
{
  MOZ_ASSERT(js::GetObjectJSClass(obj) == &sHTMLDocumentAllClass);
  return static_cast<nsHTMLDocument*>(
    static_cast<nsINode*>(JS_GetPrivate(obj)));
}

static inline nsresult
WrapNative(JSContext *cx, JSObject *scope, nsISupports *native,
           nsWrapperCache *cache, const nsIID* aIID, JS::MutableHandle<JS::Value> vp,
           bool aAllowWrapping)
{
  if (!native) {
    vp.setNull();

    return NS_OK;
  }

  JSObject *wrapper = xpc_FastGetCachedWrapper(cache, scope, vp);
  if (wrapper) {
    return NS_OK;
  }

  return nsDOMClassInfo::XPConnect()->WrapNativeToJSVal(cx, scope, native,
                                                        cache, aIID,
                                                        aAllowWrapping, vp);
}

static inline nsresult
WrapNative(JSContext *cx, JSObject *scope, nsISupports *native,
           nsWrapperCache *cache, bool aAllowWrapping,
           JS::MutableHandle<JS::Value> vp)
{
  return WrapNative(cx, scope, native, cache, nullptr, vp, aAllowWrapping);
}

bool
nsHTMLDocumentSH::DocumentAllGetProperty(JSContext *cx, JS::Handle<JSObject*> obj_,
                                         JS::Handle<jsid> id, JS::MutableHandle<JS::Value> vp)
{
  JS::Rooted<JSObject*> obj(cx, obj_);

  
  
  
  
  if (nsDOMClassInfo::sItem_id == id || nsDOMClassInfo::sNamedItem_id == id) {
    return true;
  }

  JS::Rooted<JSObject*> proto(cx);
  while (js::GetObjectJSClass(obj) != &sHTMLDocumentAllClass) {
    if (!js::GetObjectProto(cx, obj, &proto)) {
      return false;
    }

    if (!proto) {
      NS_ERROR("The JS engine lies!");
      return true;
    }

    obj = proto;
  }

  nsHTMLDocument *doc = GetDocument(obj);
  nsISupports *result;
  nsWrapperCache *cache;
  nsresult rv = NS_OK;

  if (JSID_IS_STRING(id)) {
    if (nsDOMClassInfo::sLength_id == id) {
      
      
      

      nsRefPtr<nsContentList> nodeList = doc->All()->Collection();

      uint32_t length;
      rv = nodeList->GetLength(&length);

      if (NS_FAILED(rv)) {
        xpc::Throw(cx, rv);

        return false;
      }

      vp.set(INT_TO_JSVAL(length));

      return true;
    }

    
    nsDependentJSString str(id);
    result = doc->GetDocumentAllResult(str, &cache, &rv);

    if (NS_FAILED(rv)) {
      xpc::Throw(cx, rv);
      return false;
    }
  } else if (JSID_IS_INT(id) && JSID_TO_INT(id) >= 0) {
    
    

    nsRefPtr<nsContentList> nodeList = doc->All()->Collection();

    nsIContent *node = nodeList->Item(JSID_TO_INT(id));

    result = node;
    cache = node;
  } else {
    result = nullptr;
  }

  if (result) {
    rv = WrapNative(cx, JS::CurrentGlobalOrNull(cx), result, cache, true, vp);
    if (NS_FAILED(rv)) {
      xpc::Throw(cx, rv);

      return false;
    }
  } else {
    vp.setUndefined();
  }

  return true;
}

bool
nsHTMLDocumentSH::DocumentAllNewResolve(JSContext *cx, JS::Handle<JSObject*> obj,
                                        JS::Handle<jsid> id, unsigned flags,
                                        JS::MutableHandle<JSObject*> objp)
{
  JS::Rooted<JS::Value> v(cx);

  if (nsDOMClassInfo::sItem_id == id || nsDOMClassInfo::sNamedItem_id == id) {
    

    JSFunction *fnc = ::JS_DefineFunctionById(cx, obj, id, CallToGetPropMapper,
                                              0, JSPROP_ENUMERATE);
    objp.set(obj);

    return fnc != nullptr;
  }

  if (nsDOMClassInfo::sLength_id == id) {
    
    
    
    

    v = JSVAL_ONE;
  } else {
    if (!DocumentAllGetProperty(cx, obj, id, &v)) {
      return false;
    }
  }

  bool ok = true;

  if (v.get() != JSVAL_VOID) {
    ok = ::JS_DefinePropertyById(cx, obj, id, v, nullptr, nullptr, 0);
    objp.set(obj);
  }

  return ok;
}

void
nsHTMLDocumentSH::ReleaseDocument(JSFreeOp *fop, JSObject *obj)
{
  nsIHTMLDocument* doc = GetDocument(obj);
  if (doc) {
    nsContentUtils::DeferredFinalize(doc);
  }
}

bool
nsHTMLDocumentSH::CallToGetPropMapper(JSContext *cx, unsigned argc, jsval *vp)
{
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  

  if (args.length() != 1) {
    
    
    
    xpc::Throw(cx, NS_ERROR_INVALID_ARG);

    return false;
  }

  
  JS::Rooted<JSString*> str(cx, JS::ToString(cx, args[0]));
  if (!str) {
    return false;
  }

  
  
  JS::Rooted<JSObject*> self(cx);
  if (args.calleev().isObject() &&
      JS_GetClass(&args.calleev().toObject()) == &sHTMLDocumentAllClass) {
    self = &args.calleev().toObject();
  } else {
    self = JS_THIS_OBJECT(cx, vp);
    if (!self)
      return false;
  }

  size_t length;
  JS::Anchor<JSString *> anchor(str);
  const jschar *chars = ::JS_GetStringCharsAndLength(cx, str, &length);
  if (!chars) {
    return false;
  }

  return ::JS_GetUCProperty(cx, self, chars, length, args.rval());
}
