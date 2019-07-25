






#ifndef GlobalObject_h___
#define GlobalObject_h___

#include "mozilla/Attributes.h"

#include "jsarray.h"
#include "jsbool.h"
#include "jsexn.h"
#include "jsfun.h"
#include "jsiter.h"
#include "jsnum.h"

#include "js/Vector.h"

#include "builtin/RegExp.h"

extern JSObject *
js_InitObjectClass(JSContext *cx, JSObject *obj);

extern JSObject *
js_InitFunctionClass(JSContext *cx, JSObject *obj);

extern JSObject *
js_InitTypedArrayClasses(JSContext *cx, JSObject *obj);

namespace js {

class Debugger;




























class GlobalObject : public JSObject
{
    



    static const unsigned STANDARD_CLASS_SLOTS  = JSProto_LIMIT * 3;

    
    static const unsigned THROWTYPEERROR          = STANDARD_CLASS_SLOTS;
    static const unsigned GENERATOR_PROTO         = THROWTYPEERROR + 1;
    static const unsigned REGEXP_STATICS          = GENERATOR_PROTO + 1;
    static const unsigned FUNCTION_NS             = REGEXP_STATICS + 1;
    static const unsigned RUNTIME_CODEGEN_ENABLED = FUNCTION_NS + 1;
    static const unsigned EVAL                    = RUNTIME_CODEGEN_ENABLED + 1;
    static const unsigned FLAGS                   = EVAL + 1;
    static const unsigned DEBUGGERS               = FLAGS + 1;

    
    static const unsigned RESERVED_SLOTS = DEBUGGERS + 1;

    void staticAsserts() {
        




        JS_STATIC_ASSERT(JSCLASS_GLOBAL_SLOT_COUNT == RESERVED_SLOTS);
    }

    static const int32_t FLAGS_CLEARED = 0x1;

    inline void setFlags(int32_t flags);
    inline void initFlags(int32_t flags);

    friend JSObject *
    ::js_InitObjectClass(JSContext *cx, JSObject *obj);
    friend JSObject *
    ::js_InitFunctionClass(JSContext *cx, JSObject *obj);

    
    JSObject *
    initFunctionAndObjectClasses(JSContext *cx);

    inline void setDetailsForKey(JSProtoKey key, JSObject *ctor, JSObject *proto);
    inline void setObjectClassDetails(JSFunction *ctor, JSObject *proto);
    inline void setFunctionClassDetails(JSFunction *ctor, JSObject *proto);

    inline void setThrowTypeError(JSFunction *fun);

    inline void setOriginalEval(JSObject *evalobj);

    Value getConstructor(JSProtoKey key) const {
        JS_ASSERT(key <= JSProto_LIMIT);
        return getSlot(key);
    }

    Value getPrototype(JSProtoKey key) const {
        JS_ASSERT(key <= JSProto_LIMIT);
        return getSlot(JSProto_LIMIT + key);
    }

    bool classIsInitialized(JSProtoKey key) const {
        bool inited = !getConstructor(key).isUndefined();
        JS_ASSERT(inited == !getPrototype(key).isUndefined());
        return inited;
    }

    bool functionObjectClassesInitialized() const {
        bool inited = classIsInitialized(JSProto_Function);
        JS_ASSERT(inited == classIsInitialized(JSProto_Object));
        return inited;
    }

    bool arrayClassInitialized() const {
        return classIsInitialized(JSProto_Array);
    }

    bool booleanClassInitialized() const {
        return classIsInitialized(JSProto_Boolean);
    }
    bool numberClassInitialized() const {
        return classIsInitialized(JSProto_Number);
    }
    bool stringClassInitialized() const {
        return classIsInitialized(JSProto_String);
    }
    bool regexpClassInitialized() const {
        return classIsInitialized(JSProto_RegExp);
    }
    bool arrayBufferClassInitialized() const {
        return classIsInitialized(JSProto_ArrayBuffer);
    }
    bool errorClassesInitialized() const {
        return classIsInitialized(JSProto_Error);
    }

  public:
    static GlobalObject *create(JSContext *cx, Class *clasp);

    



    JSFunction *
    createConstructor(JSContext *cx, JSNative ctor, JSAtom *name, unsigned length,
                      gc::AllocKind kind = JSFunction::FinalizeKind);

    







    JSObject *createBlankPrototype(JSContext *cx, js::Class *clasp);

    



    JSObject *createBlankPrototypeInheriting(JSContext *cx, js::Class *clasp, JSObject &proto);

    JSObject *getOrCreateObjectPrototype(JSContext *cx) {
        if (functionObjectClassesInitialized())
            return &getPrototype(JSProto_Object).toObject();
        Rooted<GlobalObject*> self(cx, this);
        if (!initFunctionAndObjectClasses(cx))
            return NULL;
        return &self->getPrototype(JSProto_Object).toObject();
    }

    JSObject *getOrCreateFunctionPrototype(JSContext *cx) {
        if (functionObjectClassesInitialized())
            return &getPrototype(JSProto_Function).toObject();
        Rooted<GlobalObject*> self(cx, this);
        if (!initFunctionAndObjectClasses(cx))
            return NULL;
        return &self->getPrototype(JSProto_Function).toObject();
    }

    JSObject *getOrCreateArrayPrototype(JSContext *cx) {
        if (arrayClassInitialized())
            return &getPrototype(JSProto_Array).toObject();
        Rooted<GlobalObject*> self(cx, this);
        if (!js_InitArrayClass(cx, this))
            return NULL;
        return &self->getPrototype(JSProto_Array).toObject();
    }

    JSObject *getOrCreateBooleanPrototype(JSContext *cx) {
        if (booleanClassInitialized())
            return &getPrototype(JSProto_Boolean).toObject();
        Rooted<GlobalObject*> self(cx, this);
        if (!js_InitBooleanClass(cx, this))
            return NULL;
        return &self->getPrototype(JSProto_Boolean).toObject();
    }

    JSObject *getOrCreateNumberPrototype(JSContext *cx) {
        if (numberClassInitialized())
            return &getPrototype(JSProto_Number).toObject();
        Rooted<GlobalObject*> self(cx, this);
        if (!js_InitNumberClass(cx, this))
            return NULL;
        return &self->getPrototype(JSProto_Number).toObject();
    }

    JSObject *getOrCreateStringPrototype(JSContext *cx) {
        if (stringClassInitialized())
            return &getPrototype(JSProto_String).toObject();
        Rooted<GlobalObject*> self(cx, this);
        if (!js_InitStringClass(cx, this))
            return NULL;
        return &self->getPrototype(JSProto_String).toObject();
    }

    JSObject *getOrCreateRegExpPrototype(JSContext *cx) {
        if (regexpClassInitialized())
            return &getPrototype(JSProto_RegExp).toObject();
        Rooted<GlobalObject*> self(cx, this);
        if (!js_InitRegExpClass(cx, this))
            return NULL;
        return &self->getPrototype(JSProto_RegExp).toObject();
    }

    JSObject *getOrCreateArrayBufferPrototype(JSContext *cx) {
        if (arrayBufferClassInitialized())
            return &getPrototype(JSProto_ArrayBuffer).toObject();
        Rooted<GlobalObject*> self(cx, this);
        if (!js_InitTypedArrayClasses(cx, this))
            return NULL;
        return &self->getPrototype(JSProto_ArrayBuffer).toObject();
    }

    JSObject *getOrCreateCustomErrorPrototype(JSContext *cx, int exnType) {
        JSProtoKey key = GetExceptionProtoKey(exnType);
        if (errorClassesInitialized())
            return &getPrototype(key).toObject();
        Rooted<GlobalObject*> self(cx, this);
        if (!js_InitExceptionClasses(cx, this))
            return NULL;
        return &self->getPrototype(key).toObject();
    }

    JSObject *getOrCreateGeneratorPrototype(JSContext *cx) {
        Value v = getSlotRef(GENERATOR_PROTO);
        if (v.isObject())
            return &v.toObject();
        Rooted<GlobalObject*> self(cx, this);
        if (!js_InitIteratorClasses(cx, this))
            return NULL;
        return &self->getSlot(GENERATOR_PROTO).toObject();
    }

    inline RegExpStatics *getRegExpStatics() const;

    JSObject *getThrowTypeError() const {
        JS_ASSERT(functionObjectClassesInitialized());
        return &getSlot(THROWTYPEERROR).toObject();
    }

    void clear(JSContext *cx);

    bool isCleared() const {
        return getSlot(FLAGS).toInt32() & FLAGS_CLEARED;
    }

    bool isRuntimeCodeGenEnabled(JSContext *cx);

    const Value &getOriginalEval() const {
        JS_ASSERT(getSlot(EVAL).isObject());
        return getSlot(EVAL);
    }

    bool getFunctionNamespace(JSContext *cx, Value *vp);

    static bool initGeneratorClass(JSContext *cx, Handle<GlobalObject*> global);
    static bool initStandardClasses(JSContext *cx, Handle<GlobalObject*> global);

    typedef js::Vector<js::Debugger *, 0, js::SystemAllocPolicy> DebuggerVector;

    



    DebuggerVector *getDebuggers();

    



    static DebuggerVector *getOrCreateDebuggers(JSContext *cx, Handle<GlobalObject*> global);

    static bool addDebugger(JSContext *cx, Handle<GlobalObject*> global, Debugger *dbg);
};






extern bool
LinkConstructorAndPrototype(JSContext *cx, JSObject *ctor, JSObject *proto);





extern bool
DefinePropertiesAndBrand(JSContext *cx, JSObject *obj, JSPropertySpec *ps, JSFunctionSpec *fs);

typedef HashSet<GlobalObject *, DefaultHasher<GlobalObject *>, SystemAllocPolicy> GlobalObjectSet;

} 

inline bool
JSObject::isGlobal() const
{
    return !!(js::GetObjectClass(this)->flags & JSCLASS_IS_GLOBAL);
}

js::GlobalObject &
JSObject::asGlobal()
{
    JS_ASSERT(isGlobal());
    return *static_cast<js::GlobalObject *>(this);
}

#endif 
