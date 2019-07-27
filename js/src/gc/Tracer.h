





#ifndef js_Tracer_h
#define js_Tracer_h

#include "mozilla/DebugOnly.h"

#include "js/GCAPI.h"
#include "js/SliceBudget.h"
#include "js/TracingAPI.h"

namespace js {
class GCMarker;
class ObjectImpl;
namespace gc {
struct ArenaHeader;
}
namespace jit {
class JitCode;
}
namespace types {
struct TypeObject;
}

static const size_t NON_INCREMENTAL_MARK_STACK_BASE_CAPACITY = 4096;
static const size_t INCREMENTAL_MARK_STACK_BASE_CAPACITY = 32768;













class MarkStack
{
    friend class GCMarker;

    uintptr_t *stack_;
    uintptr_t *tos_;
    uintptr_t *end_;

    
    size_t baseCapacity_;
    size_t maxCapacity_;

  public:
    explicit MarkStack(size_t maxCapacity)
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

    void setStack(uintptr_t *stack, size_t tosIndex, size_t capacity) {
        stack_ = stack;
        tos_ = stack + tosIndex;
        end_ = stack + capacity;
    }

    bool init(JSGCMode gcMode);

    void setBaseCapacity(JSGCMode mode);
    size_t maxCapacity() const { return maxCapacity_; }
    void setMaxCapacity(size_t maxCapacity);

    bool push(uintptr_t item) {
        if (tos_ == end_) {
            if (!enlarge(1))
                return false;
        }
        JS_ASSERT(tos_ < end_);
        *tos_++ = item;
        return true;
    }

    bool push(uintptr_t item1, uintptr_t item2, uintptr_t item3) {
        uintptr_t *nextTos = tos_ + 3;
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

    uintptr_t pop() {
        JS_ASSERT(!isEmpty());
        return *--tos_;
    }

    void reset();

    
    bool enlarge(unsigned count);

    void setGCMode(JSGCMode gcMode);

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const;
};

class GCMarker : public JSTracer
{
  public:
    explicit GCMarker(JSRuntime *rt);
    bool init(JSGCMode gcMode);

    void setMaxCapacity(size_t maxCap) { stack.setMaxCapacity(maxCap); }
    size_t maxCapacity() const { return stack.maxCapacity(); }

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

#ifdef DEBUG
    bool shouldCheckCompartments() { return strictCompartmentChecking; }
#endif

    
    MarkStack stack;

  private:
#ifdef DEBUG
    void checkZone(void *p);
#else
    void checkZone(void *p) {}
#endif

    




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
    static_assert(StackTagMask >= uintptr_t(LastTag), "The tag mask must subsume the tags.");
    static_assert(StackTagMask <= gc::CellMask, "The tag mask must be embeddable in a Cell*.");

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

    void appendGrayRoot(void **thingp, JSGCTraceKind kind);

    
    uint32_t color;

    
    js::gc::ArenaHeader *unmarkedArenaStackTop;

    
    mozilla::DebugOnly<size_t> markLaterArenas;

    enum GrayBufferState {
        GRAY_BUFFER_UNUSED,
        GRAY_BUFFER_OK,
        GRAY_BUFFER_FAILED
    };
    GrayBufferState grayBufferState;

    
    mozilla::DebugOnly<bool> started;

    



    mozilla::DebugOnly<bool> strictCompartmentChecking;
};

void
SetMarkStackLimit(JSRuntime *rt, size_t limit);

} 




#define IS_GC_MARKING_TRACER(trc) \
    ((trc)->callback == nullptr || (trc)->callback == GCMarker::GrayCallback)

#endif 
