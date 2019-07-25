






































#ifndef jsiter_h___
#define jsiter_h___




#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsversion.h"






#define JSITER_ENUMERATE  0x1   /* for-in compatible hidden default iterator */
#define JSITER_FOREACH    0x2   /* return [key, value] pair rather than key */
#define JSITER_KEYVALUE   0x4   /* destructuring for-in wants [key, value] */
#define JSITER_OWNONLY    0x8   /* iterate over obj's own properties only */

struct NativeIterator {
    jsboxedword *props_array;
    jsboxedword *props_cursor;
    jsboxedword *props_end;
    uint32      *shapes_array;
    uint32      shapes_length;
    uint32      shapes_key;
    uintN       flags;
    JSObject    *next;

    void mark(JSTracer *trc);
};

bool
EnumerateOwnProperties(JSContext *cx, JSObject *obj, JSIdArray **idap);







extern JS_FRIEND_API(JSBool)
js_ValueToIterator(JSContext *cx, uintN flags, js::Value *vp);

extern JS_FRIEND_API(JSBool)
js_CloseIterator(JSContext *cx, const js::Value &v);






extern JSBool
js_IteratorMore(JSContext *cx, JSObject *iterobj, js::Value *rval);

extern JSBool
js_IteratorNext(JSContext *cx, JSObject *iterobj, js::Value *rval);

extern JSBool
js_ThrowStopIteration(JSContext *cx);

#if JS_HAS_GENERATORS




typedef enum JSGeneratorState {
    JSGEN_NEWBORN,  
    JSGEN_OPEN,     
    JSGEN_RUNNING,  
    JSGEN_CLOSING,  
    JSGEN_CLOSED    
} JSGeneratorState;

struct JSGenerator {
    JSObject            *obj;
    JSGeneratorState    state;
    JSFrameRegs         savedRegs;
    uintN               vplen;
    JSStackFrame        *liveFrame;
    js::Value           floatingStack[1];

    JSStackFrame *getFloatingFrame() {
        return reinterpret_cast<JSStackFrame *>(floatingStack + vplen);
    }

    JSStackFrame *getLiveFrame() {
        JS_ASSERT((state == JSGEN_RUNNING || state == JSGEN_CLOSING) ==
                  (liveFrame != getFloatingFrame()));
        return liveFrame;
    }
};

extern JSObject *
js_NewGenerator(JSContext *cx);












inline JSStackFrame *
js_FloatingFrameIfGenerator(JSContext *cx, JSStackFrame *fp)
{
    JS_ASSERT(cx->stack().contains(fp));
    if (JS_UNLIKELY(fp->isGenerator()))
        return cx->generatorFor(fp)->getFloatingFrame();
    return fp;
}


extern JSGenerator *
js_FloatingFrameToGenerator(JSStackFrame *fp);

inline JSStackFrame *
js_LiveFrameIfGenerator(JSStackFrame *fp)
{
    if (fp->flags & JSFRAME_GENERATOR)
        return js_FloatingFrameToGenerator(fp)->getLiveFrame();
    return fp;
}

#endif

extern js::ExtendedClass js_GeneratorClass;
extern js::ExtendedClass js_IteratorClass;
extern js::Class         js_StopIterationClass;

static inline bool
js_ValueIsStopIteration(const js::Value &v)
{
    return v.isObject() && v.asObject().getClass() == &js_StopIterationClass;
}

extern JSObject *
js_InitIteratorClasses(JSContext *cx, JSObject *obj);

#endif 
