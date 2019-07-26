





#ifndef js_Tracer_h
#define js_Tracer_h

#include "mozilla/DebugOnly.h"

#include "jsutil.h"

#include "js/GCAPI.h"
#include "js/SliceBudget.h"
#include "js/TracingAPI.h"

namespace js {
class ObjectImpl;
namespace gc {
class ArenaHeader;
}
namespace jit {
class JitCode;
}
namespace types {
class TypeObject;
}

static const size_t NON_INCREMENTAL_MARK_STACK_BASE_CAPACITY = 4096;
static const size_t INCREMENTAL_MARK_STACK_BASE_CAPACITY = 32768;













template<class T>
struct MarkStack {
    T *stack_;
    T *tos_;
    T *end_;

    
    size_t baseCapacity_;
    size_t maxCapacity_;

    MarkStack(size_t maxCapacity)
      : stack_(nullptr),
        tos_(nullptr),
        end_(nullptr),
        baseCapacity_(0),
        maxCapacity_(maxCapacity)
    {}

    ~MarkStack() {
        js_free(stack_);
    }

    size_t capacity() { return end_ - stack_; }

    ptrdiff_t position() const { return tos_ - stack_; }

    void setStack(T *stack, size_t tosIndex, size_t capacity) {
        stack_ = stack;
        tos_ = stack + tosIndex;
        end_ = stack + capacity;
    }

    void setBaseCapacity(JSGCMode mode) {
        switch (mode) {
          case JSGC_MODE_GLOBAL:
          case JSGC_MODE_COMPARTMENT:
            baseCapacity_ = NON_INCREMENTAL_MARK_STACK_BASE_CAPACITY;
            break;
          case JSGC_MODE_INCREMENTAL:
            baseCapacity_ = INCREMENTAL_MARK_STACK_BASE_CAPACITY;
            break;
          default:
            MOZ_ASSUME_UNREACHABLE("bad gc mode");
        }

        if (baseCapacity_ > maxCapacity_)
            baseCapacity_ = maxCapacity_;
    }

    bool init(JSGCMode gcMode) {
        setBaseCapacity(gcMode);

        JS_ASSERT(!stack_);
        T *newStack = js_pod_malloc<T>(baseCapacity_);
        if (!newStack)
            return false;

        setStack(newStack, 0, baseCapacity_);
        return true;
    }

    void setMaxCapacity(size_t maxCapacity) {
        JS_ASSERT(isEmpty());
        maxCapacity_ = maxCapacity;
        if (baseCapacity_ > maxCapacity_)
            baseCapacity_ = maxCapacity_;

        reset();
    }

    bool push(T item) {
        if (tos_ == end_) {
            if (!enlarge(1))
                return false;
        }
        JS_ASSERT(tos_ < end_);
        *tos_++ = item;
        return true;
    }

    bool push(T item1, T item2, T item3) {
        T *nextTos = tos_ + 3;
        if (nextTos > end_) {
            if (!enlarge(3))
                return false;
            nextTos = tos_ + 3;
        }
        JS_ASSERT(nextTos <= end_);
        tos_[0] = item1;
        tos_[1] = item2;
        tos_[2] = item3;
        tos_ = nextTos;
        return true;
    }

    bool isEmpty() const {
        return tos_ == stack_;
    }

    T pop() {
        JS_ASSERT(!isEmpty());
        return *--tos_;
    }

    void reset() {
        if (capacity() == baseCapacity_) {
            
            setStack(stack_, 0, baseCapacity_);
            return;
        }

        T *newStack = (T *)js_realloc(stack_, sizeof(T) * baseCapacity_);
        if (!newStack) {
            
            
            newStack = stack_;
            baseCapacity_ = capacity();
        }
        setStack(newStack, 0, baseCapacity_);
    }

    
    bool enlarge(unsigned count) {
        size_t newCapacity = Min(maxCapacity_, capacity() * 2);
        if (newCapacity < capacity() + count)
            return false;

        size_t tosIndex = position();

        T *newStack = (T *)js_realloc(stack_, sizeof(T) * newCapacity);
        if (!newStack)
            return false;

        setStack(newStack, tosIndex, newCapacity);
        return true;
    }

    void setGCMode(JSGCMode gcMode) {
        
        
        setBaseCapacity(gcMode);
    }

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const {
        return mallocSizeOf(stack_);
    }
};

struct GCMarker : public JSTracer {
  private:
    




    enum StackTag {
        ValueArrayTag,
        ObjectTag,
        TypeTag,
        XmlTag,
        SavedValueArrayTag,
        JitCodeTag,
        LastTag = JitCodeTag
    };

    static const uintptr_t StackTagMask = 7;

    static void staticAsserts() {
        JS_STATIC_ASSERT(StackTagMask >= uintptr_t(LastTag));
        JS_STATIC_ASSERT(StackTagMask <= gc::CellMask);
    }

  public:
    explicit GCMarker(JSRuntime *rt);
    bool init(JSGCMode gcMode);

    void setMaxCapacity(size_t maxCap) { stack.setMaxCapacity(maxCap); }
    size_t maxCapacity() const { return stack.maxCapacity_; }

    void start();
    void stop();
    void reset();

    void pushObject(ObjectImpl *obj) {
        pushTaggedPtr(ObjectTag, obj);
    }

    void pushType(types::TypeObject *type) {
        pushTaggedPtr(TypeTag, type);
    }

    void pushJitCode(jit::JitCode *code) {
        pushTaggedPtr(JitCodeTag, code);
    }

    uint32_t getMarkColor() const {
        return color;
    }

    






    void setMarkColorGray() {
        JS_ASSERT(isDrained());
        JS_ASSERT(color == gc::BLACK);
        color = gc::GRAY;
    }

    void setMarkColorBlack() {
        JS_ASSERT(isDrained());
        JS_ASSERT(color == gc::GRAY);
        color = gc::BLACK;
    }

    inline void delayMarkingArena(gc::ArenaHeader *aheader);
    void delayMarkingChildren(const void *thing);
    void markDelayedChildren(gc::ArenaHeader *aheader);
    bool markDelayedChildren(SliceBudget &budget);
    bool hasDelayedChildren() const {
        return !!unmarkedArenaStackTop;
    }

    bool isDrained() {
        return isMarkStackEmpty() && !unmarkedArenaStackTop;
    }

    bool drainMarkStack(SliceBudget &budget);

    








    bool hasBufferedGrayRoots() const;
    void startBufferingGrayRoots();
    void endBufferingGrayRoots();
    void resetBufferedGrayRoots();
    void markBufferedGrayRoots(JS::Zone *zone);

    static void GrayCallback(JSTracer *trc, void **thing, JSGCTraceKind kind);

    void setGCMode(JSGCMode mode) { stack.setGCMode(mode); }

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const;

    MarkStack<uintptr_t> stack;

  private:
#ifdef DEBUG
    void checkZone(void *p);
#else
    void checkZone(void *p) {}
#endif

    void pushTaggedPtr(StackTag tag, void *ptr) {
        checkZone(ptr);
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        JS_ASSERT(!(addr & StackTagMask));
        if (!stack.push(addr | uintptr_t(tag)))
            delayMarkingChildren(ptr);
    }

    void pushValueArray(JSObject *obj, void *start, void *end) {
        checkZone(obj);

        JS_ASSERT(start <= end);
        uintptr_t tagged = reinterpret_cast<uintptr_t>(obj) | GCMarker::ValueArrayTag;
        uintptr_t startAddr = reinterpret_cast<uintptr_t>(start);
        uintptr_t endAddr = reinterpret_cast<uintptr_t>(end);

        



        if (!stack.push(endAddr, startAddr, tagged))
            delayMarkingChildren(obj);
    }

    bool isMarkStackEmpty() {
        return stack.isEmpty();
    }

    bool restoreValueArray(JSObject *obj, void **vpp, void **endp);
    void saveValueRanges();
    inline void processMarkStackTop(SliceBudget &budget);
    void processMarkStackOther(uintptr_t tag, uintptr_t addr);

    void appendGrayRoot(void *thing, JSGCTraceKind kind);

    
    uint32_t color;

    mozilla::DebugOnly<bool> started;

    
    js::gc::ArenaHeader *unmarkedArenaStackTop;
    
    mozilla::DebugOnly<size_t> markLaterArenas;

    enum GrayBufferState
    {
        GRAY_BUFFER_UNUSED,
        GRAY_BUFFER_OK,
        GRAY_BUFFER_FAILED
    };

    GrayBufferState grayBufferState;
};

void
SetMarkStackLimit(JSRuntime *rt, size_t limit);

} 




#define IS_GC_MARKING_TRACER(trc) \
    ((trc)->callback == nullptr || (trc)->callback == GCMarker::GrayCallback)

#endif 
