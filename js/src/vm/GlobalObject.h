







































#ifndef GlobalObject_h___
#define GlobalObject_h___

#include "jsarray.h"
#include "jsbool.h"
#include "jsfun.h"
#include "jsiter.h"
#include "jsnum.h"
#include "jstypedarray.h"

#include "js/Vector.h"

#include "builtin/RegExp.h"

extern JSObject *
js_InitObjectClass(JSContext *cx, JSObject *obj);

extern JSObject *
js_InitFunctionClass(JSContext *cx, JSObject *obj);

namespace js {

class Debugger;




























class GlobalObject : public ::JSObject {
    



    static const uintN STANDARD_CLASS_SLOTS  = JSProto_LIMIT * 3;

    
    static const uintN THROWTYPEERROR          = STANDARD_CLASS_SLOTS;
    static const uintN GENERATOR_PROTO         = THROWTYPEERROR + 1;
    static const uintN REGEXP_STATICS          = GENERATOR_PROTO + 1;
    static const uintN FUNCTION_NS             = REGEXP_STATICS + 1;
    static const uintN RUNTIME_CODEGEN_ENABLED = FUNCTION_NS + 1;
    static const uintN EVAL                    = RUNTIME_CODEGEN_ENABLED + 1;
    static const uintN FLAGS                   = EVAL + 1;
    static const uintN DEBUGGERS               = FLAGS + 1;

    
    static const uintN RESERVED_SLOTS = DEBUGGERS + 1;

    void staticAsserts() {
        




        JS_STATIC_ASSERT(JSCLASS_GLOBAL_SLOT_COUNT == RESERVED_SLOTS);
    }

    static const int32 FLAGS_CLEARED = 0x1;

    inline void setFlags(int32 flags);
    inline void initFlags(int32 flags);

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

  public:
    static GlobalObject *create(JSContext *cx, Class *clasp);

    



    JSFunction *
    createConstructor(JSContext *cx, JSNative ctor, Class *clasp, JSAtom *name, uintN length,
                      gc::AllocKind kind = JSFunction::FinalizeKind);

    







    JSObject *createBlankPrototype(JSContext *cx, js::Class *clasp);

    



    JSObject *createBlankPrototypeInheriting(JSContext *cx, js::Class *clasp, JSObject &proto);

    JSObject *getOrCreateObjectPrototype(JSContext *cx) {
        if (!functionObjectClassesInitialized()) {
            if (!initFunctionAndObjectClasses(cx))
                return NULL;
        }
        return &getPrototype(JSProto_Object).toObject();
    }

    JSObject *getOrCreateFunctionPrototype(JSContext *cx) {
        if (!functionObjectClassesInitialized()) {
            if (!initFunctionAndObjectClasses(cx))
                return NULL;
        }
        return &getPrototype(JSProto_Function).toObject();
    }

    JSObject *getOrCreateArrayPrototype(JSContext *cx) {
        if (!arrayClassInitialized()) {
            if (!js_InitArrayClass(cx, this))
                return NULL;
        }
        return &getPrototype(JSProto_Array).toObject();
    }

    JSObject *getOrCreateBooleanPrototype(JSContext *cx) {
        if (!booleanClassInitialized()) {
            if (!js_InitBooleanClass(cx, this))
                return NULL;
        }
        return &getPrototype(JSProto_Boolean).toObject();
    }

    JSObject *getOrCreateNumberPrototype(JSContext *cx) {
        if (!numberClassInitialized()) {
            if (!js_InitNumberClass(cx, this))
                return NULL;
        }
        return &getPrototype(JSProto_Number).toObject();
    }

    JSObject *getOrCreateStringPrototype(JSContext *cx) {
        if (!stringClassInitialized()) {
            if (!js_InitStringClass(cx, this))
                return NULL;
        }
        return &getPrototype(JSProto_String).toObject();
    }

    JSObject *getOrCreateRegExpPrototype(JSContext *cx) {
        if (!regexpClassInitialized()) {
            if (!js_InitRegExpClass(cx, this))
                return NULL;
        }
        return &getPrototype(JSProto_RegExp).toObject();
    }

    JSObject *getOrCreateArrayBufferPrototype(JSContext *cx) {
        if (!arrayBufferClassInitialized()) {
            if (!js_InitTypedArrayClasses(cx, this))
                return NULL;
        }
        return &getPrototype(JSProto_ArrayBuffer).toObject();
    }

    JSObject *getOrCreateGeneratorPrototype(JSContext *cx) {
        HeapValue &v = getSlotRef(GENERATOR_PROTO);
        if (!v.isObject() && !js_InitIteratorClasses(cx, this))
            return NULL;
        return &v.toObject();
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

    bool initGeneratorClass(JSContext *cx);
    bool initStandardClasses(JSContext *cx);

    typedef js::Vector<js::Debugger *, 0, js::SystemAllocPolicy> DebuggerVector;

    



    DebuggerVector *getDebuggers();

    



    DebuggerVector *getOrCreateDebuggers(JSContext *cx);

    bool addDebugger(JSContext *cx, Debugger *dbg);
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

js::GlobalObject *
JSObject::asGlobal()
{
    JS_ASSERT(isGlobal());
    return static_cast<js::GlobalObject *>(this);
}

#endif 
