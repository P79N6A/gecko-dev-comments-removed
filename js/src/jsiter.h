






































#ifndef jsiter_h___
#define jsiter_h___




#include "jscntxt.h"
#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsversion.h"






#define JSITER_ENUMERATE  0x1   /* for-in compatible hidden default iterator */
#define JSITER_FOREACH    0x2   /* return [key, value] pair rather than key */
#define JSITER_KEYVALUE   0x4   /* destructuring for-in wants [key, value] */
#define JSITER_OWNONLY    0x8   /* iterate over obj's own properties only */
#define JSITER_HIDDEN     0x10  /* also enumerate non-enumerable properties */





#define JSITER_ACTIVE       0x1000
#define JSITER_UNREUSABLE   0x2000

namespace js {

struct NativeIterator {
    JSObject  *obj;
    jsid      *props_array;
    jsid      *props_cursor;
    jsid      *props_end;
    uint32    *shapes_array;
    uint32    shapes_length;
    uint32    shapes_key;
    uint32    flags;
    JSObject  *next;  

    bool isKeyIter() const { return (flags & JSITER_FOREACH) == 0; }

    inline jsid *begin() const {
        return props_array;
    }

    inline jsid *end() const {
        return props_end;
    }

    size_t numKeys() const {
        return end() - begin();
    }

    jsid *current() const {
        JS_ASSERT(props_cursor < props_end);
        return props_cursor;
    }

    void incCursor() {
        props_cursor = props_cursor + 1;
    }

    static NativeIterator *allocateIterator(JSContext *cx, uint32 slength,
                                            const js::AutoIdVector &props);
    void init(JSObject *obj, uintN flags, uint32 slength, uint32 key);

    void mark(JSTracer *trc);
};

bool
VectorToIdArray(JSContext *cx, js::AutoIdVector &props, JSIdArray **idap);

JS_FRIEND_API(bool)
GetPropertyNames(JSContext *cx, JSObject *obj, uintN flags, js::AutoIdVector *props);

bool
GetIterator(JSContext *cx, JSObject *obj, uintN flags, js::Value *vp);

bool
VectorToKeyIterator(JSContext *cx, JSObject *obj, uintN flags, js::AutoIdVector &props, js::Value *vp);

bool
VectorToValueIterator(JSContext *cx, JSObject *obj, uintN flags, js::AutoIdVector &props, js::Value *vp);





bool
EnumeratedIdVectorToIterator(JSContext *cx, JSObject *obj, uintN flags, js::AutoIdVector &props, js::Value *vp);

}







extern JS_FRIEND_API(JSBool)
js_ValueToIterator(JSContext *cx, uintN flags, js::Value *vp);

extern JS_FRIEND_API(JSBool)
js_CloseIterator(JSContext *cx, JSObject *iterObj);

bool
js_SuppressDeletedProperty(JSContext *cx, JSObject *obj, jsid id);

bool
js_SuppressDeletedIndexProperties(JSContext *cx, JSObject *obj, jsint begin, jsint end);






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
    js::FrameRegs       regs;
    JSObject            *enumerators;
    js::StackFrame      *floating;
    js::Value           floatingStack[1];

    js::StackFrame *floatingFrame() {
        return floating;
    }

    js::StackFrame *liveFrame() {
        JS_ASSERT((state == JSGEN_RUNNING || state == JSGEN_CLOSING) ==
                  (regs.fp() != floatingFrame()));
        return regs.fp();
    }
};

extern JSObject *
js_NewGenerator(JSContext *cx);












inline js::StackFrame *
js_FloatingFrameIfGenerator(JSContext *cx, js::StackFrame *fp)
{
    JS_ASSERT(cx->stack.contains(fp));
    if (JS_UNLIKELY(fp->isGeneratorFrame()))
        return cx->generatorFor(fp)->floatingFrame();
    return fp;
}


extern JSGenerator *
js_FloatingFrameToGenerator(js::StackFrame *fp);

inline js::StackFrame *
js_LiveFrameIfGenerator(js::StackFrame *fp)
{
    return fp->isGeneratorFrame() ? js_FloatingFrameToGenerator(fp)->liveFrame() : fp;
}

#endif

extern js::Class js_GeneratorClass;
extern js::Class js_IteratorClass;
extern js::Class js_StopIterationClass;

static inline bool
js_ValueIsStopIteration(const js::Value &v)
{
    return v.isObject() && v.toObject().getClass() == &js_StopIterationClass;
}

extern JSObject *
js_InitIteratorClasses(JSContext *cx, JSObject *obj);

#endif 
