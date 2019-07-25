







































#ifndef GlobalObject_h___
#define GlobalObject_h___

#include "jsfun.h"

extern JSObject *
js_InitFunctionAndObjectClasses(JSContext *cx, JSObject *obj);

namespace js {




























class GlobalObject : public ::JSObject {
    



    static const uintN STANDARD_CLASS_SLOTS  = JSProto_LIMIT * 3;

    
    static const uintN THROWTYPEERROR          = STANDARD_CLASS_SLOTS;
    static const uintN REGEXP_STATICS          = THROWTYPEERROR + 1;
    static const uintN FUNCTION_NS             = REGEXP_STATICS + 1;
    static const uintN RUNTIME_CODEGEN_ENABLED = FUNCTION_NS + 1;
    static const uintN EVAL                    = RUNTIME_CODEGEN_ENABLED + 1;
    static const uintN FLAGS                   = EVAL + 1;

    
    static const uintN RESERVED_SLOTS = FLAGS + 1;

    void staticAsserts() {
        




        JS_STATIC_ASSERT(JSCLASS_GLOBAL_SLOT_COUNT == RESERVED_SLOTS);
    }

    static const int32 FLAGS_CLEARED = 0x1;

    void setFlags(int32 flags) {
        setSlot(FLAGS, Int32Value(flags));
    }

  public:
    static GlobalObject *create(JSContext *cx, Class *clasp);

    



    JSFunction *
    createConstructor(JSContext *cx, Native ctor, Class *clasp, JSAtom *name, uintN length);

    







    JSObject *createBlankPrototype(JSContext *cx, js::Class *clasp);

    void setThrowTypeError(JSFunction *fun) {
        
        
        
        
        setSlot(THROWTYPEERROR, ObjectValue(*fun));
    }

    JSObject *getThrowTypeError() const {
        return &getSlot(THROWTYPEERROR).toObject();
    }

    Value getRegExpStatics() const {
        return getSlot(REGEXP_STATICS);
    }

    void clear(JSContext *cx);

    bool isCleared() const {
        return getSlot(FLAGS).toInt32() & FLAGS_CLEARED;
    }

    bool isRuntimeCodeGenEnabled(JSContext *cx);

    const Value &getOriginalEval() const {
        return getSlot(EVAL);
    }

    void setOriginalEval(JSObject *evalobj) {
        
        
        
        
        setSlot(EVAL, ObjectValue(*evalobj));
    }

    bool getFunctionNamespace(JSContext *cx, Value *vp);

    bool initStandardClasses(JSContext *cx);
};






extern bool
LinkConstructorAndPrototype(JSContext *cx, JSObject *ctor, JSObject *proto);





extern bool
DefinePropertiesAndBrand(JSContext *cx, JSObject *obj, JSPropertySpec *ps, JSFunctionSpec *fs);

} 

js::GlobalObject *
JSObject::asGlobal()
{
    JS_ASSERT(isGlobal());
    return reinterpret_cast<js::GlobalObject *>(this);
}

#endif 
