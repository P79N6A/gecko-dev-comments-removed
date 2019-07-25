






































#ifndef jsiter_h___
#define jsiter_h___




#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsversion.h"

JS_BEGIN_EXTERN_C






#define JSITER_ENUMERATE  0x1   /* for-in compatible hidden default iterator */
#define JSITER_FOREACH    0x2   /* return [key, value] pair rather than key */
#define JSITER_KEYVALUE   0x4   /* destructuring for-in wants [key, value] */
#define JSITER_OWNONLY    0x8   /* iterate over obj's own properties only */
#define JSITER_HIDDEN     0x10  /* also enumerate non-enumerable properties */

struct NativeIterator {
    JSObject  *obj;
    jsval     *props_array;
    jsval     *props_cursor;
    jsval     *props_end;
    uint32    *shapes_array;
    uint32    shapes_length;
    uint32    shapes_key;
    uintN     flags;
    JSObject  *next;

    static NativeIterator *allocate(JSContext *cx, JSObject *obj, uintN flags,
                                    uint32 *sarray, uint32 slength, uint32 key,
                                    js::AutoValueVector &props);

    void mark(JSTracer *trc);
};






static const jsval JSVAL_NATIVE_ENUMERATE_COOKIE = SPECIAL_TO_JSVAL(0x220576);

bool
VectorToIdArray(JSContext *cx, js::AutoValueVector &props, JSIdArray **idap);

bool
GetPropertyNames(JSContext *cx, JSObject *obj, uintN flags, js::AutoValueVector &props);

bool
GetIterator(JSContext *cx, JSObject *obj, uintN flags, jsval *vp);

bool
IdVectorToIterator(JSContext *cx, JSObject *obj, uintN flags, js::AutoValueVector &props, jsval *vp);







extern JS_FRIEND_API(JSBool)
js_ValueToIterator(JSContext *cx, uintN flags, jsval *vp);

extern JS_FRIEND_API(JSBool)
js_CloseIterator(JSContext *cx, jsval v);

bool
js_SuppressDeletedProperty(JSContext *cx, JSObject *obj, jsid id);






extern JSBool
js_IteratorMore(JSContext *cx, JSObject *iterobj, jsval *rval);

extern JSBool
js_IteratorNext(JSContext *cx, JSObject *iterobj, jsval *rval);

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
    JSObject            *enumerators;
    jsval               floatingStack[1];

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

extern JSExtendedClass js_GeneratorClass;
extern JSExtendedClass js_IteratorClass;
extern JSClass         js_StopIterationClass;

static inline bool
js_ValueIsStopIteration(jsval v)
{
    return !JSVAL_IS_PRIMITIVE(v) &&
           JSVAL_TO_OBJECT(v)->getClass() == &js_StopIterationClass;
}

extern JSObject *
js_InitIteratorClasses(JSContext *cx, JSObject *obj);

JS_END_EXTERN_C

#endif 
