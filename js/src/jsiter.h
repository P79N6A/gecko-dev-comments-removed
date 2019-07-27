





#ifndef jsiter_h
#define jsiter_h





#include "mozilla/MemoryReporting.h"

#include "jscntxt.h"

#include "gc/Barrier.h"
#include "vm/Stack.h"





#define JSITER_ACTIVE       0x1000
#define JSITER_UNREUSABLE   0x2000

namespace js {

struct NativeIterator
{
    HeapPtrObject obj;                  
    JSObject* iterObj_;                 
    HeapPtrFlatString* props_array;
    HeapPtrFlatString* props_cursor;
    HeapPtrFlatString* props_end;
    Shape** shapes_array;
    uint32_t shapes_length;
    uint32_t shapes_key;
    uint32_t flags;

  private:
    
    NativeIterator* next_;
    NativeIterator* prev_;

  public:
    bool isKeyIter() const {
        return (flags & JSITER_FOREACH) == 0;
    }

    inline HeapPtrFlatString* begin() const {
        return props_array;
    }

    inline HeapPtrFlatString* end() const {
        return props_end;
    }

    size_t numKeys() const {
        return end() - begin();
    }

    JSObject* iterObj() const {
        return iterObj_;
    }
    HeapPtrFlatString* current() const {
        MOZ_ASSERT(props_cursor < props_end);
        return props_cursor;
    }

    NativeIterator* next() {
        return next_;
    }

    static inline size_t offsetOfNext() {
        return offsetof(NativeIterator, next_);
    }
    static inline size_t offsetOfPrev() {
        return offsetof(NativeIterator, prev_);
    }

    void incCursor() {
        props_cursor = props_cursor + 1;
    }
    void link(NativeIterator* other) {
        
        MOZ_ASSERT(!next_ && !prev_);
        MOZ_ASSERT(flags & JSITER_ENUMERATE);

        this->next_ = other;
        this->prev_ = other->prev_;
        other->prev_->next_ = this;
        other->prev_ = this;
    }
    void unlink() {
        MOZ_ASSERT(flags & JSITER_ENUMERATE);

        next_->prev_ = prev_;
        prev_->next_ = next_;
        next_ = nullptr;
        prev_ = nullptr;
    }

    static NativeIterator* allocateSentinel(JSContext* maybecx);
    static NativeIterator* allocateIterator(JSContext* cx, uint32_t slength,
                                            const js::AutoIdVector& props);
    void init(JSObject* obj, JSObject* iterObj, unsigned flags, uint32_t slength, uint32_t key);

    void mark(JSTracer* trc);

    static void destroy(NativeIterator* iter) {
        js_free(iter);
    }
};

class PropertyIteratorObject : public NativeObject
{
  public:
    static const Class class_;

    NativeIterator* getNativeIterator() const {
        return static_cast<js::NativeIterator*>(getPrivate());
    }
    void setNativeIterator(js::NativeIterator* ni) {
        setPrivate(ni);
    }

    size_t sizeOfMisc(mozilla::MallocSizeOf mallocSizeOf) const;

  private:
    static void trace(JSTracer* trc, JSObject* obj);
    static void finalize(FreeOp* fop, JSObject* obj);
};

class ArrayIteratorObject : public JSObject
{
  public:
    static const Class class_;
};

class StringIteratorObject : public JSObject
{
  public:
    static const Class class_;
};

bool
VectorToIdArray(JSContext* cx, AutoIdVector& props, JSIdArray** idap);

bool
GetIterator(JSContext* cx, HandleObject obj, unsigned flags, MutableHandleObject objp);

JSObject*
GetIteratorObject(JSContext* cx, HandleObject obj, unsigned flags);





bool
EnumeratedIdVectorToIterator(JSContext* cx, HandleObject obj, unsigned flags, AutoIdVector& props,
                             MutableHandleObject objp);

bool
NewEmptyPropertyIterator(JSContext* cx, unsigned flags, MutableHandleObject objp);







bool
ValueToIterator(JSContext* cx, unsigned flags, MutableHandleValue vp);

bool
CloseIterator(JSContext* cx, HandleObject iterObj);

bool
UnwindIteratorForException(JSContext* cx, HandleObject obj);

void
UnwindIteratorForUncatchableException(JSContext* cx, JSObject* obj);

bool
IteratorConstructor(JSContext* cx, unsigned argc, Value* vp);

extern bool
SuppressDeletedProperty(JSContext* cx, HandleObject obj, jsid id);

extern bool
SuppressDeletedElement(JSContext* cx, HandleObject obj, uint32_t index);

extern bool
SuppressDeletedElements(JSContext* cx, HandleObject obj, uint32_t begin, uint32_t end);





extern bool
IteratorMore(JSContext* cx, HandleObject iterobj, MutableHandleValue rval);

extern bool
ThrowStopIteration(JSContext* cx);





extern JSObject*
CreateItrResultObject(JSContext* cx, HandleValue value, bool done);

extern JSObject*
InitIteratorClasses(JSContext* cx, HandleObject obj);

} 

#endif 
