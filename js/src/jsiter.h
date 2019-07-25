






































#ifndef jsiter_h___
#define jsiter_h___




#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsversion.h"






#define JSITER_ENUMERATE  0x1   /* for-in compatible hidden default iterator */
#define JSITER_FOREACH    0x2   /* return [key, value] pair rather than key */
#define JSITER_KEYVALUE   0x4   /* destructuring for-in wants [key, value] */
#define JSITER_OWNONLY    0x8   /* iterate over obj's own properties only */
#define JSITER_HIDDEN     0x10  /* also enumerate non-enumerable properties */

struct NativeIterator {
    JSObject  *obj;
    void      *props_array;
    void      *props_cursor;
    void      *props_end;
    uint32    *shapes_array;
    uint32    shapes_length;
    uint32    shapes_key;
    uintN     flags;
    JSObject  *next;

    bool isKeyIter() const { return (flags & JSITER_FOREACH) == 0; }

    inline jsid *beginKey() const {
        JS_ASSERT(isKeyIter());
        return (jsid *)props_array;
    }

    inline jsid *endKey() const {
        JS_ASSERT(isKeyIter());
        return (jsid *)props_end;
    }

    size_t numKeys() const {
        return endKey() - beginKey();
    }

    jsid *currentKey() const {
        JS_ASSERT(isKeyIter());
        return reinterpret_cast<jsid *>(props_cursor);
    }

    void incKeyCursor() {
        JS_ASSERT(isKeyIter());
        props_cursor = reinterpret_cast<jsid *>(props_cursor) + 1;
    }

    inline js::Value *beginValue() const {
        JS_ASSERT(!isKeyIter());
        return (js::Value *)props_array;
    }

    inline js::Value *endValue() const {
        JS_ASSERT(!isKeyIter());
        return (js::Value *)props_end;
    }

    size_t numValues() const {
        return endValue() - beginValue();
    }

    js::Value *currentValue() const {
        JS_ASSERT(!isKeyIter());
        return reinterpret_cast<js::Value *>(props_cursor);
    }

    void incValueCursor() {
        JS_ASSERT(!isKeyIter());
        props_cursor = reinterpret_cast<js::Value *>(props_cursor) + 1;
    }

    static NativeIterator *allocateKeyIterator(JSContext *cx, uint32 slength,
                                               const js::AutoIdVector &props);
    static NativeIterator *allocateValueIterator(JSContext *cx, uint32 slength,
                                                 const js::AutoValueVector &props);
    void init(JSObject *obj, uintN flags, const uint32 *sarray, uint32 slength, uint32 key);

    void mark(JSTracer *trc);
};

bool
VectorToIdArray(JSContext *cx, js::AutoIdVector &props, JSIdArray **idap);

bool
GetPropertyNames(JSContext *cx, JSObject *obj, uintN flags, js::AutoIdVector &props);

bool
GetIterator(JSContext *cx, JSObject *obj, uintN flags, js::Value *vp);

bool
VectorToKeyIterator(JSContext *cx, JSObject *obj, uintN flags, js::AutoIdVector &props, js::Value *vp);

bool
VectorToValueIterator(JSContext *cx, JSObject *obj, uintN flags, js::AutoValueVector &props, js::Value *vp);





bool
EnumeratedIdVectorToIterator(JSContext *cx, JSObject *obj, uintN flags, js::AutoIdVector &props, js::Value *vp);







extern JS_FRIEND_API(JSBool)
js_ValueToIterator(JSContext *cx, uintN flags, js::Value *vp);

extern JS_FRIEND_API(JSBool)
js_CloseIterator(JSContext *cx, JSObject *iterObj);

bool
js_SuppressDeletedProperty(JSContext *cx, JSObject *obj, jsid id);






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
    JSObject            *enumerators;
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
    return v.isObject() && v.toObject().getClass() == &js_StopIterationClass;
}

extern JSObject *
js_InitIteratorClasses(JSContext *cx, JSObject *obj);

#endif 
