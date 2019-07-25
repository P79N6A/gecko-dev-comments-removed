







































#ifndef GlobalObject_h___
#define GlobalObject_h___

#include "jsfun.h"

extern JSObject *
js_InitFunctionAndObjectClasses(JSContext *cx, JSObject *obj);

namespace js {




























class GlobalObject : public ::JSObject {
    



    static const uintN STANDARD_CLASS_SLOTS  = JSProto_LIMIT * 3;

    
    static const uintN THROWTYPEERROR        = STANDARD_CLASS_SLOTS;
    static const uintN REGEXP_STATICS        = THROWTYPEERROR + 1;
    static const uintN FUNCTION_NS           = REGEXP_STATICS + 1;
    static const uintN EVAL_ALLOWED          = FUNCTION_NS + 1;
    static const uintN EVAL                  = EVAL_ALLOWED + 1;
    static const uintN FLAGS                 = EVAL + 1;

    
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

    void setThrowTypeError(JSFunction *fun) {
        Value &v = getSlotRef(THROWTYPEERROR);
        
        
        
        v.setObject(*fun);
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

    bool isEvalAllowed(JSContext *cx);

    const Value &getOriginalEval() const {
        return getSlot(EVAL);
    }

    void setOriginalEval(JSObject *evalobj) {
        Value &v = getSlotRef(EVAL);
        
        
        
        v.setObject(*evalobj);
    }

    bool getFunctionNamespace(JSContext *cx, Value *vp);

    bool initStandardClasses(JSContext *cx);
};

} 

js::GlobalObject *
JSObject::asGlobal()
{
    JS_ASSERT(isGlobal());
    return reinterpret_cast<js::GlobalObject *>(this);
}

#endif 
