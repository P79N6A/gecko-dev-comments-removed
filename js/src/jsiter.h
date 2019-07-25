






































#ifndef jsiter_h___
#define jsiter_h___




#include "jscntxt.h"
#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsversion.h"

#include "gc/Barrier.h"
#include "vm/Stack.h"





#define JSITER_ACTIVE       0x1000
#define JSITER_UNREUSABLE   0x2000

namespace js {

struct NativeIterator {
    HeapPtrObject obj;
    HeapId    *props_array;
    HeapId    *props_cursor;
    HeapId    *props_end;
    const Shape **shapes_array;
    uint32_t  shapes_length;
    uint32_t  shapes_key;
    uint32_t  flags;
    JSObject  *next;  

    bool isKeyIter() const { return (flags & JSITER_FOREACH) == 0; }

    inline HeapId *begin() const {
        return props_array;
    }

    inline HeapId *end() const {
        return props_end;
    }

    size_t numKeys() const {
        return end() - begin();
    }

    HeapId *current() const {
        JS_ASSERT(props_cursor < props_end);
        return props_cursor;
    }

    void incCursor() {
        props_cursor = props_cursor + 1;
    }

    static NativeIterator *allocateIterator(JSContext *cx, uint32_t slength,
                                            const js::AutoIdVector &props);
    void init(JSObject *obj, uintN flags, uint32_t slength, uint32_t key);

    void mark(JSTracer *trc);
};

bool
VectorToIdArray(JSContext *cx, js::AutoIdVector &props, JSIdArray **idap);

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

extern bool
js_SuppressDeletedProperty(JSContext *cx, JSObject *obj, jsid id);

extern bool
js_SuppressDeletedElement(JSContext *cx, JSObject *obj, uint32_t index);

extern bool
js_SuppressDeletedElements(JSContext *cx, JSObject *obj, uint32_t begin, uint32_t end);






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
    js::HeapPtrObject   obj;
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

extern JSObject *
js_InitIteratorClasses(JSContext *cx, JSObject *obj);

#endif 
