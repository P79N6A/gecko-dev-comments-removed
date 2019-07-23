






































#ifndef jsiter_h___
#define jsiter_h___

JS_BEGIN_EXTERN_C




#include "jsprvtd.h"
#include "jspubtd.h"

#define JSITER_ENUMERATE  0x1   /* for-in compatible hidden default iterator */
#define JSITER_FOREACH    0x2   /* return [key, value] pair rather than key */
#define JSITER_KEYVALUE   0x4   /* destructuring for-in wants [key, value] */







extern JSBool
js_ValueToIterator(JSContext *cx, uintN flags, jsval *vp);

extern JSBool
js_CloseIterator(JSContext *cx, jsval v);





extern JSBool
js_CallIteratorNext(JSContext *cx, JSObject *iterobj, jsval *rval);




extern void
js_CloseNativeIterator(JSContext *cx, JSObject *iterobj);

#if JS_HAS_GENERATORS




typedef enum JSGeneratorState {
    JSGEN_NEWBORN,  
    JSGEN_OPEN,     
    JSGEN_RUNNING,  
    JSGEN_CLOSING,  
    JSGEN_CLOSED    
} JSGeneratorState;

struct JSGenerator {
    JSGenerator         *next;
    JSObject            *obj;
    JSGeneratorState    state;
    JSStackFrame        frame;
    JSArena             arena;
    jsval               stack[1];
};

#define FRAME_TO_GENERATOR(fp) \
    ((JSGenerator *) ((uint8 *)(fp) - offsetof(JSGenerator, frame)))

extern JSObject *
js_NewGenerator(JSContext *cx, JSStackFrame *fp);

#endif

extern JSClass          js_GeneratorClass;
extern JSClass          js_IteratorClass;
extern JSClass          js_StopIterationClass;

extern JSObject *
js_InitIteratorClasses(JSContext *cx, JSObject *obj);

JS_END_EXTERN_C

#endif 
