







































#ifndef mozilla_jetpack_HandleParent_h
#define mozilla_jetpack_HandleParent_h

#include "mozilla/jetpack/PHandleParent.h"
#include "mozilla/jetpack/PHandleChild.h"

#include "jsapi.h"
#include "jsobj.h"
#include "jscntxt.h"

#include "mozilla/unused.h"

namespace mozilla {
namespace jetpack {





template <class BaseType>
class Handle
  : public BaseType
{
  Handle(Handle* parent)
    : mParent(parent)
    , mObj(NULL)
    , mCx(NULL)
    , mRooted(false)
  {}

  BaseType* AllocPHandle() {
    return new Handle(this);
  }

  bool DeallocPHandle(BaseType* actor) {
    delete actor;
    return true;
  }

public:

  Handle()
    : mParent(NULL)
    , mObj(NULL)
    , mCx(NULL)
    , mRooted(false)
  {}

  ~Handle() { TearDown(); }

  static Handle* FromJSObject(JSContext* cx, JSObject* obj) {
    
    return Unwrap(cx, obj);
  }

  static Handle* FromJSVal(JSContext* cx, jsval val) {
    if (!JSVAL_IS_OBJECT(val))
      return NULL;
    return Unwrap(cx, JSVAL_TO_OBJECT(val));
  }

  void Root() {
    NS_ASSERTION(mObj && mCx, "Rooting with no object unexpected.");
    if (mRooted)
      return;

    if (!JS_AddNamedObjectRoot(mCx, &mObj, "Jetpack Handle")) {
      NS_RUNTIMEABORT("Failed to add root.");
    }
    mRooted = true;
  }

  void Unroot() {
    NS_ASSERTION(mCx, "Unrooting with no JSContext unexpected.");
    if (!mRooted)
      return;

    JS_RemoveObjectRoot(mCx, &mObj);
    mRooted = false;
  }

  JSObject* ToJSObject(JSContext* cx) {
    if (!mObj && !mCx) {
      JSAutoRequest request(cx);

      JSClass* clasp = const_cast<JSClass*>(&sHandle_JSClass);
      JSObject* obj = JS_NewObject(cx, clasp, NULL, NULL);
      if (!obj)
        return NULL;
      js::AutoObjectRooter root(cx, obj);

      JSPropertySpec* ps = const_cast<JSPropertySpec*>(sHandle_Properties);
      JSFunctionSpec* fs = const_cast<JSFunctionSpec*>(sHandle_Functions);

      if (JS_SetPrivate(cx, obj, (void*)this) &&
          JS_DefineProperties(cx, obj, ps) &&
          JS_DefineFunctions(cx, obj, fs)) {
        mObj = obj;
        mCx = cx;
        Root();
      }
    }
    return mObj;
  }

protected:

  void ActorDestroy(typename Handle::ActorDestroyReason why) {
    TearDown();
  }

private:

  static bool IsParent(const PHandleParent* handle) { return true; }
  static bool IsParent(const PHandleChild* handle) { return false; }

  void TearDown() {
    if (mCx) {
      JSAutoRequest ar(mCx);

      if (mObj) {
        mObj->setPrivate(NULL);

        js::AutoObjectRooter obj(mCx, mObj);
        mObj = NULL;

        JSBool hasOnInvalidate;
        if (JS_HasProperty(mCx, obj.object(), "onInvalidate",
                           &hasOnInvalidate) && hasOnInvalidate) {
          js::AutoValueRooter r(mCx);
          JSBool ok = JS_CallFunctionName(mCx, obj.object(), "onInvalidate", 0,
                                          NULL, r.jsval_addr());
          if (!ok)
            JS_ReportPendingException(mCx);
        }

        
        
      }

      
      
      
      Unroot();
    }
  }

  static const JSClass        sHandle_JSClass;
  static const JSPropertySpec sHandle_Properties[];
  static const JSFunctionSpec sHandle_Functions[];

  Handle* const mParent;

  
  
  JSObject*  mObj;
  JSContext* mCx;
  bool mRooted;

  static Handle*
  Unwrap(JSContext* cx, JSObject* obj) {
    while (obj && obj->getJSClass() != &sHandle_JSClass)
      obj = obj->getProto();

    if (!obj)
      return NULL;

    Handle* self = static_cast<Handle*>(JS_GetPrivate(cx, obj));

    NS_ASSERTION(!self || self->ToJSObject(cx) == obj,
                 "Wrapper and wrapped object disagree?");

    return self;
  }

  static JSBool
  GetParent(JSContext* cx, JSObject* obj, jsid, jsval* vp) {
    JS_SET_RVAL(cx, vp, JSVAL_NULL);

    Handle* self = Unwrap(cx, obj);
    if (!self)
      return JS_TRUE;

    Handle* parent = self->mParent;
    if (!parent)
      return JS_TRUE;

    JSObject* pobj = parent->ToJSObject(cx);
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(pobj));

    return JS_TRUE;
  }

  static JSBool
  GetIsValid(JSContext* cx, JSObject* obj, jsid, jsval* vp) {
    Handle* self = Unwrap(cx, obj);
    JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(!!self));
    return JS_TRUE;
  }

  static JSBool
  GetIsRooted(JSContext* cx, JSObject* obj, jsid, jsval* vp) {
    Handle* self = Unwrap(cx, obj);
    bool rooted = self ? self->mRooted : false;
    JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(rooted));
    return JS_TRUE;
  }

  static JSBool
  SetIsRooted(JSContext* cx, JSObject* obj, jsid, jsval* vp) {
    Handle* self = Unwrap(cx, obj);
    JSBool v;
    if (!JS_ValueToBoolean(cx, *vp, &v))
      return JS_FALSE;

    if (!self) {
      if (v) {
        JS_ReportError(cx, "Cannot root invalidated handle.");
        return JS_FALSE;
      }
      return JS_TRUE;
    }

    if (v)
      self->Root();
    else
      self->Unroot();

    *vp = BOOLEAN_TO_JSVAL(v);
    return JS_TRUE;
  }

  static JSBool
  Invalidate(JSContext* cx, uintN argc, jsval* vp) {
    if (argc > 0) {
      JS_ReportError(cx, "invalidate takes zero arguments");
      return JS_FALSE;
    }

    Handle* self = Unwrap(cx, JS_THIS_OBJECT(cx, vp));
    if (self)
      unused << BaseType::Send__delete__(self);

    JS_SET_RVAL(cx, vp, JSVAL_VOID);

    return JS_TRUE;
  }

  static JSBool
  CreateHandle(JSContext* cx, uintN argc, jsval* vp) {
    if (argc > 0) {
      JS_ReportError(cx, "createHandle takes zero arguments");
      return JS_FALSE;
    }

    Handle* self = Unwrap(cx, JS_THIS_OBJECT(cx, vp));
    if (!self) {
      JS_ReportError(cx, "Tried to create child from invalid handle");
      return JS_FALSE;
    }

    BaseType* child = self->SendPHandleConstructor();
    if (!child) {
      JS_ReportError(cx, "Failed to construct child");
      return JS_FALSE;
    }

    JSObject* obj = static_cast<Handle*>(child)->ToJSObject(cx);
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj));

    return JS_TRUE;
  }

  static void
  Finalize(JSContext* cx, JSObject* obj) {
    Handle* self = Unwrap(cx, obj);
    if (self) {
      NS_ASSERTION(!self->mRooted, "Finalizing a rooted object?");
      self->mCx = NULL;
      self->mObj = NULL;
      unused << BaseType::Send__delete__(self);
    }
  }
};

template <class BaseType>
const JSClass
Handle<BaseType>::sHandle_JSClass = {
  "IPDL Handle", JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub,
  JS_PropertyStub, JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub,
  JS_ConvertStub, Handle::Finalize,
  JSCLASS_NO_OPTIONAL_MEMBERS
};

#define HANDLE_PROP_FLAGS (JSPROP_PERMANENT | JSPROP_SHARED)

template <class BaseType>
const JSPropertySpec
Handle<BaseType>::sHandle_Properties[] = {
  { "parent",  0, HANDLE_PROP_FLAGS | JSPROP_READONLY, GetParent,  NULL },
  { "isValid", 0, HANDLE_PROP_FLAGS | JSPROP_READONLY, GetIsValid, NULL },
  { "isRooted", 0, HANDLE_PROP_FLAGS, GetIsRooted, SetIsRooted },
  { 0, 0, 0, NULL, NULL }
};

#undef HANDLE_PROP_FLAGS

#define HANDLE_FUN_FLAGS (JSPROP_READONLY |     \
                          JSPROP_PERMANENT)

template <class BaseType>
const JSFunctionSpec
Handle<BaseType>::sHandle_Functions[] = {
  JS_FN("invalidate",   Invalidate,   0, HANDLE_FUN_FLAGS),
  JS_FN("createHandle", CreateHandle, 0, HANDLE_FUN_FLAGS),
  JS_FS_END
};

#undef HANDLE_FUN_FLAGS



typedef Handle<PHandleParent> HandleParent;
typedef Handle<PHandleChild> HandleChild;

} 
} 

#endif
