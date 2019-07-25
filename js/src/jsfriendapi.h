






































#ifndef jsfriendapi_h___
#define jsfriendapi_h___

#include "jsclass.h"
#include "jspubtd.h"
#include "jsprvtd.h"

JS_BEGIN_EXTERN_C

extern JS_FRIEND_API(JSString *)
JS_GetAnonymousString(JSRuntime *rt);

extern JS_FRIEND_API(JSObject *)
JS_FindCompilationScope(JSContext *cx, JSObject *obj);

extern JS_FRIEND_API(JSFunction *)
JS_GetObjectFunction(JSObject *obj);

extern JS_FRIEND_API(JSObject *)
JS_GetFrameScopeChainRaw(JSStackFrame *fp);

extern JS_FRIEND_API(JSBool)
JS_SplicePrototype(JSContext *cx, JSObject *obj, JSObject *proto);

extern JS_FRIEND_API(JSObject *)
JS_NewObjectWithUniqueType(JSContext *cx, JSClass *clasp, JSObject *proto, JSObject *parent);

extern JS_FRIEND_API(uint32)
JS_ObjectCountDynamicSlots(JSObject *obj);

extern JS_FRIEND_API(size_t)
JS_GetE4XObjectsCreated(JSContext *cx);

extern JS_FRIEND_API(size_t)
JS_SetProtoCalled(JSContext *cx);

extern JS_FRIEND_API(size_t)
JS_GetCustomIteratorCount(JSContext *cx);


typedef struct TypeInferenceMemoryStats
{
    int64 scripts;
    int64 objects;
    int64 tables;
    int64 temporary;
    int64 emptyShapes;
} TypeInferenceMemoryStats;

extern JS_FRIEND_API(void)
JS_GetTypeInferenceMemoryStats(JSContext *cx, JSCompartment *compartment,
                               TypeInferenceMemoryStats *stats);

extern JS_FRIEND_API(void)
JS_GetTypeInferenceObjectStats( void *object,
                               TypeInferenceMemoryStats *stats);

extern JS_FRIEND_API(JSPrincipals *)
JS_GetCompartmentPrincipals(JSCompartment *compartment);


extern JS_FRIEND_API(JSObject *)
JS_ObjectToInnerObject(JSContext *cx, JSObject *obj);


extern JS_FRIEND_API(JSObject *)
JS_ObjectToOuterObject(JSContext *cx, JSObject *obj);

extern JS_FRIEND_API(JSObject *)
JS_CloneObject(JSContext *cx, JSObject *obj, JSObject *proto, JSObject *parent);

extern JS_FRIEND_API(JSBool)
js_GetterOnlyPropertyStub(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp);

#ifdef __cplusplus

extern JS_FRIEND_API(bool)
JS_CopyPropertiesFrom(JSContext *cx, JSObject *target, JSObject *obj);

extern JS_FRIEND_API(JSBool)
JS_WrapPropertyDescriptor(JSContext *cx, js::PropertyDescriptor *desc);

extern JS_FRIEND_API(JSBool)
JS_EnumerateState(JSContext *cx, JSObject *obj, JSIterateOp enum_op, js::Value *statep, jsid *idp);

#endif

JS_END_EXTERN_C

#ifdef __cplusplus

namespace js {

class JS_FRIEND_API(AutoPreserveCompartment) {
  private:
    JSContext *cx;
    JSCompartment *oldCompartment;
  public:
    AutoPreserveCompartment(JSContext *cx JS_GUARD_OBJECT_NOTIFIER_PARAM);
    ~AutoPreserveCompartment();
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class JS_FRIEND_API(AutoSwitchCompartment) {
  private:
    JSContext *cx;
    JSCompartment *oldCompartment;
  public:
    AutoSwitchCompartment(JSContext *cx, JSCompartment *newCompartment
                          JS_GUARD_OBJECT_NOTIFIER_PARAM);
    AutoSwitchCompartment(JSContext *cx, JSObject *target JS_GUARD_OBJECT_NOTIFIER_PARAM);
    ~AutoSwitchCompartment();
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

#ifdef OLD_GETTER_SETTER_METHODS
JS_FRIEND_API(JSBool) obj_defineGetter(JSContext *cx, uintN argc, js::Value *vp);
JS_FRIEND_API(JSBool) obj_defineSetter(JSContext *cx, uintN argc, js::Value *vp);
#endif







extern JS_FRIEND_API(bool)
CheckUndeclaredVarAssignment(JSContext *cx, JSString *propname);







namespace shadow {

struct TypeObject {
    JSObject    *proto;
};

struct BaseShape {
    js::Class   *clasp;
};

struct Shape {
    BaseShape   *base;
};

struct Object {
    Shape       *lastProp;
    uint32      flags;
    void        *_2;
    JSObject    *parent;
    void        *privateData;
    jsuword     _3;
    js::Value   *slots;
    TypeObject  *type;

    static const uint32 FIXED_SLOTS_SHIFT = 27;

    js::Value &slotRef(size_t slot) const {
        size_t nfixed = flags >> FIXED_SLOTS_SHIFT;
        if (slot < nfixed)
            return ((Value *)((jsuword) this + sizeof(shadow::Object)))[slot];
        return slots[slot - nfixed];
    }
};

} 

extern JS_FRIEND_DATA(js::Class) AnyNameClass;
extern JS_FRIEND_DATA(js::Class) AttributeNameClass;
extern JS_FRIEND_DATA(js::Class) CallClass;
extern JS_FRIEND_DATA(js::Class) DeclEnvClass;
extern JS_FRIEND_DATA(js::Class) FunctionClass;
extern JS_FRIEND_DATA(js::Class) FunctionProxyClass;
extern JS_FRIEND_DATA(js::Class) NamespaceClass;
extern JS_FRIEND_DATA(js::Class) OuterWindowProxyClass;
extern JS_FRIEND_DATA(js::Class) ObjectProxyClass;
extern JS_FRIEND_DATA(js::Class) QNameClass;
extern JS_FRIEND_DATA(js::Class) ScriptClass;
extern JS_FRIEND_DATA(js::Class) XMLClass;

inline js::Class *
GetObjectClass(const JSObject *obj)
{
    return reinterpret_cast<const shadow::Object*>(obj)->lastProp->base->clasp;
}

inline JSClass *
GetObjectJSClass(const JSObject *obj)
{
    return js::Jsvalify(GetObjectClass(obj));
}

inline JSObject *
GetObjectParent(const JSObject *obj)
{
    return reinterpret_cast<const shadow::Object*>(obj)->parent;
}

inline JSObject *
GetObjectProto(const JSObject *obj)
{
    return reinterpret_cast<const shadow::Object*>(obj)->type->proto;
}

inline void *
GetObjectPrivate(const JSObject *obj)
{
    return reinterpret_cast<const shadow::Object*>(obj)->privateData;
}

#ifdef DEBUG
extern JS_FRIEND_API(void) CheckReservedSlot(const JSObject *obj, size_t slot);
#else
inline void CheckReservedSlot(const JSObject *obj, size_t slot) {}
#endif





inline const Value &
GetReservedSlot(const JSObject *obj, size_t slot)
{
    CheckReservedSlot(obj, slot);
    return reinterpret_cast<const shadow::Object *>(obj)->slotRef(slot);
}

inline void
SetReservedSlot(JSObject *obj, size_t slot, const Value &value)
{
    CheckReservedSlot(obj, slot);
    reinterpret_cast<shadow::Object *>(obj)->slotRef(slot) = value;
}

static inline js::PropertyOp
CastAsJSPropertyOp(JSObject *object)
{
    return JS_DATA_TO_FUNC_PTR(js::PropertyOp, object);
}

static inline js::StrictPropertyOp
CastAsJSStrictPropertyOp(JSObject *object)
{
    return JS_DATA_TO_FUNC_PTR(js::StrictPropertyOp, object);
}

JS_FRIEND_API(bool)
GetPropertyNames(JSContext *cx, JSObject *obj, uintN flags, js::AutoIdVector *props);






#define JSITER_ENUMERATE  0x1   /* for-in compatible hidden default iterator */
#define JSITER_FOREACH    0x2   /* return [key, value] pair rather than key */
#define JSITER_KEYVALUE   0x4   /* destructuring for-in wants [key, value] */
#define JSITER_OWNONLY    0x8   /* iterate over obj's own properties only */
#define JSITER_HIDDEN     0x10  /* also enumerate non-enumerable properties */


#define JSFUN_TRCINFO     0x2000

} 
#endif

#endif
