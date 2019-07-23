






































#ifndef jsiter_h___
#define jsiter_h___




#include "jsprvtd.h"
#include "jspubtd.h"

JS_BEGIN_EXTERN_C






#define JSITER_ENUMERATE  0x1   /* for-in compatible hidden default iterator */
#define JSITER_FOREACH    0x2   /* return [key, value] pair rather than key */
#define JSITER_KEYVALUE   0x4   /* destructuring for-in wants [key, value] */




const uint32 JSSLOT_ITER_STATE  = JSSLOT_PRIVATE;
const uint32 JSSLOT_ITER_FLAGS  = JSSLOT_PRIVATE + 1;







extern JS_FRIEND_API(JSBool)
js_ValueToIterator(JSContext *cx, uintN flags, jsval *vp);

extern JS_FRIEND_API(JSBool) JS_FASTCALL
js_CloseIterator(JSContext *cx, jsval v);





extern JS_FRIEND_API(JSBool)
js_CallIteratorNext(JSContext *cx, JSObject *iterobj, jsval *rval);




extern void
js_CloseNativeIterator(JSContext *cx, JSObject *iterobj);

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
    JSStackFrame        frame;
    JSFrameRegs         savedRegs;
    JSArena             arena;
    jsval               slots[1];
};

#define FRAME_TO_GENERATOR(fp) \
    ((JSGenerator *) ((uint8 *)(fp) - offsetof(JSGenerator, frame)))

extern JSObject *
js_NewGenerator(JSContext *cx, JSStackFrame *fp);

#endif

extern JS_FRIEND_API(JSClass) js_GeneratorClass;
extern JSClass                js_IteratorClass;
extern JSClass                js_StopIterationClass;

static inline bool
js_ValueIsStopIteration(jsval v)
{
    return !JSVAL_IS_PRIMITIVE(v) &&
           STOBJ_GET_CLASS(JSVAL_TO_OBJECT(v)) == &js_StopIterationClass;
}

extern JSObject *
js_InitIteratorClasses(JSContext *cx, JSObject *obj);

JS_END_EXTERN_C

#endif 
