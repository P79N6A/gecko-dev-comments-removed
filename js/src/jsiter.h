






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

struct NativeIterator
{
    HeapPtrObject obj;
    HeapPtr<JSFlatString> *props_array;
    HeapPtr<JSFlatString> *props_cursor;
    HeapPtr<JSFlatString> *props_end;
    Shape **shapes_array;
    uint32_t shapes_length;
    uint32_t shapes_key;
    uint32_t flags;
    PropertyIteratorObject *next;  

    bool isKeyIter() const { return (flags & JSITER_FOREACH) == 0; }

    inline HeapPtr<JSFlatString> *begin() const {
        return props_array;
    }

    inline HeapPtr<JSFlatString> *end() const {
        return props_end;
    }

    size_t numKeys() const {
        return end() - begin();
    }

    HeapPtr<JSFlatString> *current() const {
        JS_ASSERT(props_cursor < props_end);
        return props_cursor;
    }

    void incCursor() {
        props_cursor = props_cursor + 1;
    }

    static NativeIterator *allocateIterator(JSContext *cx, uint32_t slength,
                                            const js::AutoIdVector &props);
    void init(JSObject *obj, unsigned flags, uint32_t slength, uint32_t key);

    void mark(JSTracer *trc);
};

class PropertyIteratorObject : public JSObject
{
  public:
    static Class class_;

    inline NativeIterator *getNativeIterator() const;
    inline void setNativeIterator(js::NativeIterator *ni);

  private:
    static void trace(JSTracer *trc, JSObject *obj);
    static void finalize(FreeOp *fop, JSObject *obj);
};












class ElementIteratorObject : public JSObject
{
  public:
    static JSObject *create(JSContext *cx, Handle<Value> target);
    static JSFunctionSpec methods[];

    enum {
        TargetSlot,
        IndexSlot,
        NumSlots
    };

    static JSBool next(JSContext *cx, unsigned argc, Value *vp);
    static bool next_impl(JSContext *cx, JS::CallArgs args);
};

bool
VectorToIdArray(JSContext *cx, AutoIdVector &props, JSIdArray **idap);

bool
GetIterator(JSContext *cx, HandleObject obj, unsigned flags, Value *vp);

JSObject *
GetIteratorObject(JSContext *cx, HandleObject obj, unsigned flags);

bool
VectorToKeyIterator(JSContext *cx, HandleObject obj, unsigned flags, AutoIdVector &props, Value *vp);

bool
VectorToValueIterator(JSContext *cx, HandleObject obj, unsigned flags, AutoIdVector &props, Value *vp);





bool
EnumeratedIdVectorToIterator(JSContext *cx, HandleObject obj, unsigned flags, AutoIdVector &props, Value *vp);







bool
ValueToIterator(JSContext *cx, unsigned flags, Value *vp);

bool
CloseIterator(JSContext *cx, JSObject *iterObj);

bool
UnwindIteratorForException(JSContext *cx, JSObject *obj);

void
UnwindIteratorForUncatchableException(JSContext *cx, JSObject *obj);

}

extern bool
js_SuppressDeletedProperty(JSContext *cx, js::HandleObject obj, jsid id);

extern bool
js_SuppressDeletedElement(JSContext *cx, js::HandleObject obj, uint32_t index);

extern bool
js_SuppressDeletedElements(JSContext *cx, js::HandleObject obj, uint32_t begin, uint32_t end);






extern bool
js_IteratorMore(JSContext *cx, js::HandleObject iterobj, js::Value *rval);

extern bool
js_IteratorNext(JSContext *cx, JSObject *iterobj, js::Value *rval);

extern JSBool
js_ThrowStopIteration(JSContext *cx);

namespace js {







inline bool
Next(JSContext *cx, HandleObject iter, Value *vp)
{
    if (!js_IteratorMore(cx, iter, vp))
        return false;
    if (vp->toBoolean())
        return js_IteratorNext(cx, iter, vp);
    vp->setMagic(JS_NO_ITER_VALUE);
    return true;
}



















class ForOfIterator
{
  private:
    JSContext *cx;
    RootedObject iterator;
    RootedValue currentValue;
    bool ok;
    bool closed;

    ForOfIterator(const ForOfIterator &) MOZ_DELETE;
    ForOfIterator &operator=(const ForOfIterator &) MOZ_DELETE;

  public:
    ForOfIterator(JSContext *cx, const Value &iterable)
        : cx(cx), iterator(cx, NULL), currentValue(cx), closed(false)
    {
        RootedValue iterv(cx, iterable);
        ok = ValueToIterator(cx, JSITER_FOR_OF, iterv.address());
        iterator = ok ? &iterv.get().toObject() : NULL;
    }

    ~ForOfIterator() {
        if (!closed)
            close();
    }

    bool next() {
        JS_ASSERT(!closed);
        ok = ok && Next(cx, iterator, currentValue.address());
        return ok && !currentValue.get().isMagic(JS_NO_ITER_VALUE);
    }

    Value &value() {
        JS_ASSERT(ok);
        JS_ASSERT(!closed);
        return currentValue.get();
    }

    bool close() {
        JS_ASSERT(!closed);
        closed = true;
        if (!iterator)
            return false;
        bool throwing = cx->isExceptionPending();
        RootedValue exc(cx);
        if (throwing) {
            exc = cx->getPendingException();
            cx->clearPendingException();
        }
        bool closedOK = CloseIterator(cx, iterator);
        if (throwing && closedOK)
            cx->setPendingException(exc);
        return ok && !throwing && closedOK;
    }
};

} 

#if JS_HAS_GENERATORS




enum JSGeneratorState
{
    JSGEN_NEWBORN,  
    JSGEN_OPEN,     
    JSGEN_RUNNING,  
    JSGEN_CLOSING,  
    JSGEN_CLOSED    
};

struct JSGenerator
{
    js::HeapPtrObject   obj;
    JSGeneratorState    state;
    js::FrameRegs       regs;
    js::PropertyIteratorObject *enumerators;
    JSGenerator         *prevGenerator;
    js::StackFrame      *fp;
    js::HeapValue       stackSnapshot[1];
};

extern JSObject *
js_NewGenerator(JSContext *cx);

namespace js {

bool
GeneratorHasMarkableFrame(JSGenerator *gen);

} 
#endif

extern JSObject *
js_InitIteratorClasses(JSContext *cx, JSObject *obj);

#endif 
