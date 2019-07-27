





#ifndef gc_Marking_h
#define gc_Marking_h

#include "mozilla/DebugOnly.h"

#include "jsfriendapi.h"

#include "gc/Heap.h"
#include "gc/Tracer.h"
#include "js/GCAPI.h"
#include "js/SliceBudget.h"
#include "js/TracingAPI.h"

class JSLinearString;
class JSRope;
namespace js {
class BaseShape;
class GCMarker;
class LazyScript;
class NativeObject;
class ObjectGroup;
namespace gc {
struct ArenaHeader;
}
namespace jit {
class JitCode;
}

static const size_t NON_INCREMENTAL_MARK_STACK_BASE_CAPACITY = 4096;
static const size_t INCREMENTAL_MARK_STACK_BASE_CAPACITY = 32768;













class MarkStack
{
    friend class GCMarker;

    uintptr_t* stack_;
    uintptr_t* tos_;
    uintptr_t* end_;

    
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

    void setStack(uintptr_t* stack, size_t tosIndex, size_t capacity) {
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
        MOZ_ASSERT(tos_ < end_);
        *tos_++ = item;
        return true;
    }

    bool push(uintptr_t item1, uintptr_t item2, uintptr_t item3) {
        uintptr_t* nextTos = tos_ + 3;
        if (nextTos > end_) {
            if (!enlarge(3))
                return false;
            nextTos = tos_ + 3;
        }
        MOZ_ASSERT(nextTos <= end_);
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
        MOZ_ASSERT(!isEmpty());
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
    explicit GCMarker(JSRuntime* rt);
    bool init(JSGCMode gcMode);

    void setMaxCapacity(size_t maxCap) { stack.setMaxCapacity(maxCap); }
    size_t maxCapacity() const { return stack.maxCapacity(); }

    void start();
    void stop();
    void reset();

    
    template <typename T> void traverse(T thing);

    
    template <typename S, typename T> void traverse(S source, T target);

    
    template <typename S> void traverse(S source, jsid target);

    






    void setMarkColorGray() {
        MOZ_ASSERT(isDrained());
        MOZ_ASSERT(color == gc::BLACK);
        color = gc::GRAY;
    }
    void setMarkColorBlack() {
        MOZ_ASSERT(isDrained());
        MOZ_ASSERT(color == gc::GRAY);
        color = gc::BLACK;
    }
    uint32_t markColor() const { return color; }

    void delayMarkingArena(gc::ArenaHeader* aheader);
    void delayMarkingChildren(const void* thing);
    void markDelayedChildren(gc::ArenaHeader* aheader);
    bool markDelayedChildren(SliceBudget& budget);
    bool hasDelayedChildren() const {
        return !!unmarkedArenaStackTop;
    }

    bool isDrained() {
        return isMarkStackEmpty() && !unmarkedArenaStackTop;
    }

    bool drainMarkStack(SliceBudget& budget);

    void setGCMode(JSGCMode mode) { stack.setGCMode(mode); }

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const;

#ifdef DEBUG
    bool shouldCheckCompartments() { return strictCompartmentChecking; }
#endif

    
    MarkStack stack;

  private:
#ifdef DEBUG
    void checkZone(void* p);
#else
    void checkZone(void* p) {}
#endif

    




    enum StackTag {
        ValueArrayTag,
        ObjectTag,
        GroupTag,
        SavedValueArrayTag,
        JitCodeTag,
        LastTag = JitCodeTag
    };

    static const uintptr_t StackTagMask = 7;
    static_assert(StackTagMask >= uintptr_t(LastTag), "The tag mask must subsume the tags.");
    static_assert(StackTagMask <= gc::CellMask, "The tag mask must be embeddable in a Cell*.");

    
    
    void repush(JSObject* obj) {
        MOZ_ASSERT(gc::TenuredCell::fromPointer(obj)->isMarked(markColor()));
        pushTaggedPtr(ObjectTag, obj);
    }

    template <typename T> void markAndTraceChildren(T* thing);
    template <typename T> void markAndPush(StackTag tag, T* thing);
    template <typename T> void markAndScan(T* thing);
    void eagerlyMarkChildren(JSLinearString* str);
    void eagerlyMarkChildren(JSRope* rope);
    void eagerlyMarkChildren(JSString* str);
    void eagerlyMarkChildren(LazyScript *thing);
    void eagerlyMarkChildren(Shape* shape);
    void lazilyMarkChildren(ObjectGroup* group);

    
    template <typename T>
    void dispatchToTraceChildren(T* thing);

    
    
    template <typename T>
    bool mark(T* thing);

    void pushTaggedPtr(StackTag tag, void* ptr) {
        checkZone(ptr);
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        MOZ_ASSERT(!(addr & StackTagMask));
        if (!stack.push(addr | uintptr_t(tag)))
            delayMarkingChildren(ptr);
    }

    void pushValueArray(JSObject* obj, void* start, void* end) {
        checkZone(obj);

        MOZ_ASSERT(start <= end);
        uintptr_t tagged = reinterpret_cast<uintptr_t>(obj) | GCMarker::ValueArrayTag;
        uintptr_t startAddr = reinterpret_cast<uintptr_t>(start);
        uintptr_t endAddr = reinterpret_cast<uintptr_t>(end);

        



        if (!stack.push(endAddr, startAddr, tagged))
            delayMarkingChildren(obj);
    }

    bool isMarkStackEmpty() {
        return stack.isEmpty();
    }

    bool restoreValueArray(JSObject* obj, void** vpp, void** endp);
    void saveValueRanges();
    inline void processMarkStackTop(SliceBudget& budget);

    
    uint32_t color;

    
    js::gc::ArenaHeader* unmarkedArenaStackTop;

    
    mozilla::DebugOnly<size_t> markLaterArenas;

    
    mozilla::DebugOnly<bool> started;

    



    mozilla::DebugOnly<bool> strictCompartmentChecking;
};

bool
IsBufferingGrayRoots(JSTracer* trc);

namespace gc {







void
MarkCycleCollectorChildren(JSTracer* trc, Shape* shape);
void
MarkCycleCollectorChildren(JSTracer* trc, ObjectGroup* group);

void
PushArena(GCMarker* gcmarker, ArenaHeader* aheader);



template <typename T>
bool
IsMarkedUnbarriered(T* thingp);

template <typename T>
bool
IsMarked(BarrieredBase<T>* thingp);

template <typename T>
bool
IsMarked(ReadBarriered<T>* thingp);

template <typename T>
bool
IsAboutToBeFinalizedUnbarriered(T* thingp);

template <typename T>
bool
IsAboutToBeFinalized(BarrieredBase<T>* thingp);

template <typename T>
bool
IsAboutToBeFinalized(ReadBarriered<T>* thingp);

inline Cell*
ToMarkable(const Value& v)
{
    if (v.isMarkable())
        return (Cell*)v.toGCThing();
    return nullptr;
}

inline Cell*
ToMarkable(Cell* cell)
{
    return cell;
}



MOZ_ALWAYS_INLINE bool
IsNullTaggedPointer(void* p)
{
    return uintptr_t(p) < 32;
}



template <typename Map, typename Key>
class HashKeyRef : public BufferableRef
{
    Map* map;
    Key key;

  public:
    HashKeyRef(Map* m, const Key& k) : map(m), key(k) {}

    void mark(JSTracer* trc) {
        Key prior = key;
        typename Map::Ptr p = map->lookup(key);
        if (!p)
            return;
        TraceManuallyBarrieredEdge(trc, &key, "HashKeyRef");
        map->rekeyIfMoved(prior, key);
    }
};



template <typename S, typename T>
struct RewrapValueOrId {};
#define DECLARE_REWRAP(S, T, method, prefix) \
    template <> struct RewrapValueOrId<S, T> { \
        static S wrap(T thing) { return method ( prefix thing ); } \
    }
DECLARE_REWRAP(JS::Value, JSObject*, JS::ObjectOrNullValue, );
DECLARE_REWRAP(JS::Value, JSString*, JS::StringValue, );
DECLARE_REWRAP(JS::Value, JS::Symbol*, JS::SymbolValue, );
DECLARE_REWRAP(jsid, JSString*, NON_INTEGER_ATOM_TO_JSID, (JSAtom*));
DECLARE_REWRAP(jsid, JS::Symbol*, SYMBOL_TO_JSID, );

} 

bool
UnmarkGrayShapeRecursively(Shape* shape);

template<typename T>
void
CheckTracedThing(JSTracer* trc, T thing);

} 

#endif 
