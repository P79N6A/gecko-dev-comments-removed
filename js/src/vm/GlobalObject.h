







































#ifndef GlobalObject_h___
#define GlobalObject_h___

#include "jsfun.h"
#include "jsiter.h"

#include "js/Vector.h"

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

    void setFlags(int32 flags) {
        setSlot(FLAGS, Int32Value(flags));
    }

    friend JSObject *
    ::js_InitObjectClass(JSContext *cx, JSObject *obj);
    friend JSObject *
    ::js_InitFunctionClass(JSContext *cx, JSObject *obj);

    
    JSObject *
    initFunctionAndObjectClasses(JSContext *cx);

    void setDetailsForKey(JSProtoKey key, JSObject *ctor, JSObject *proto) {
        Value &ctorVal = getSlotRef(key);
        Value &protoVal = getSlotRef(JSProto_LIMIT + key);
        Value &visibleVal = getSlotRef(2 * JSProto_LIMIT + key);
        JS_ASSERT(ctorVal.isUndefined());
        JS_ASSERT(protoVal.isUndefined());
        JS_ASSERT(visibleVal.isUndefined());
        ctorVal = ObjectValue(*ctor);
        protoVal = ObjectValue(*proto);
        visibleVal = ctorVal;
    }

    void setObjectClassDetails(JSFunction *ctor, JSObject *proto) {
        setDetailsForKey(JSProto_Object, ctor, proto);
    }

    void setFunctionClassDetails(JSFunction *ctor, JSObject *proto) {
        setDetailsForKey(JSProto_Function, ctor, proto);
    }

    void setThrowTypeError(JSFunction *fun) {
        Value &v = getSlotRef(THROWTYPEERROR);
        JS_ASSERT(v.isUndefined());
        v.setObject(*fun);
    }

    void setOriginalEval(JSObject *evalobj) {
        Value &v = getSlotRef(EVAL);
        JS_ASSERT(v.isUndefined());
        v.setObject(*evalobj);
    }

  public:
    static GlobalObject *create(JSContext *cx, Class *clasp);

    



    JSFunction *
    createConstructor(JSContext *cx, JSNative ctor, Class *clasp, JSAtom *name, uintN length);

    







    JSObject *createBlankPrototype(JSContext *cx, js::Class *clasp);

    



    JSObject *createBlankPrototypeInheriting(JSContext *cx, js::Class *clasp, JSObject &proto);

    bool functionObjectClassesInitialized() const {
        bool inited = !getSlot(JSProto_Function).isUndefined();
        JS_ASSERT(inited == !getSlot(JSProto_LIMIT + JSProto_Function).isUndefined());
        JS_ASSERT(inited == !getSlot(JSProto_Object).isUndefined());
        JS_ASSERT(inited == !getSlot(JSProto_LIMIT + JSProto_Object).isUndefined());
        return inited;
    }

    JSObject *getFunctionPrototype() const {
        JS_ASSERT(functionObjectClassesInitialized());
        return &getSlot(JSProto_LIMIT + JSProto_Function).toObject();
    }

    JSObject *getObjectPrototype() const {
        JS_ASSERT(functionObjectClassesInitialized());
        return &getSlot(JSProto_LIMIT + JSProto_Object).toObject();
    }

    JSObject *getThrowTypeError() const {
        JS_ASSERT(functionObjectClassesInitialized());
        return &getSlot(THROWTYPEERROR).toObject();
    }

    JSObject *getOrCreateGeneratorPrototype(JSContext *cx) {
        Value &v = getSlotRef(GENERATOR_PROTO);
        if (!v.isObject() && !js_InitIteratorClasses(cx, this))
            return NULL;
        JS_ASSERT(v.toObject().isGenerator());
        return &v.toObject();
    }

    RegExpStatics *getRegExpStatics() const {
        JSObject &resObj = getSlot(REGEXP_STATICS).toObject();
        return static_cast<RegExpStatics *>(resObj.getPrivate());
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

js::GlobalObject *
JSObject::asGlobal()
{
    JS_ASSERT(isGlobal());
    return reinterpret_cast<js::GlobalObject *>(this);
}

#endif 
