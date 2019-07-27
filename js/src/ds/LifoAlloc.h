





#ifndef ds_LifoAlloc_h
#define ds_LifoAlloc_h

#include "mozilla/DebugOnly.h"
#include "mozilla/MathAlgorithms.h"
#include "mozilla/MemoryChecking.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/PodOperations.h"
#include "mozilla/TemplateLib.h"
#include "mozilla/TypeTraits.h"






#include "jsutil.h"

namespace js {

namespace detail {

static const size_t LIFO_ALLOC_ALIGN = 8;

MOZ_ALWAYS_INLINE
char *
AlignPtr(void *orig)
{
    static_assert(mozilla::tl::FloorLog2<LIFO_ALLOC_ALIGN>::value ==
                  mozilla::tl::CeilingLog2<LIFO_ALLOC_ALIGN>::value,
                  "LIFO_ALLOC_ALIGN must be a power of two");

    char *result = (char *) ((uintptr_t(orig) + (LIFO_ALLOC_ALIGN - 1)) & (~LIFO_ALLOC_ALIGN + 1));
    JS_ASSERT(uintptr_t(result) % LIFO_ALLOC_ALIGN == 0);
    return result;
}


class BumpChunk
{
    char        *bump;          
    char        *limit;         
    BumpChunk   *next_;         
    size_t      bumpSpaceSize;  

    char *headerBase() { return reinterpret_cast<char *>(this); }
    char *bumpBase() const { return limit - bumpSpaceSize; }

    explicit BumpChunk(size_t bumpSpaceSize)
      : bump(reinterpret_cast<char *>(MOZ_THIS_IN_INITIALIZER_LIST()) + sizeof(BumpChunk)),
        limit(bump + bumpSpaceSize),
        next_(nullptr), bumpSpaceSize(bumpSpaceSize)
    {
        JS_ASSERT(bump == AlignPtr(bump));
    }

    void setBump(void *ptr) {
        JS_ASSERT(bumpBase() <= ptr);
        JS_ASSERT(ptr <= limit);
#if defined(DEBUG) || defined(MOZ_HAVE_MEM_CHECKS)
        char* prevBump = bump;
#endif
        bump = static_cast<char *>(ptr);
#ifdef DEBUG
        JS_ASSERT(contains(prevBump));

        
        if (prevBump > bump)
            memset(bump, 0xcd, prevBump - bump);
#endif

        
#if defined(MOZ_HAVE_MEM_CHECKS)
        if (prevBump > bump)
            MOZ_MAKE_MEM_NOACCESS(bump, prevBump - bump);
        else if (bump > prevBump)
            MOZ_MAKE_MEM_UNDEFINED(prevBump, bump - prevBump);
#endif
    }

  public:
    BumpChunk *next() const { return next_; }
    void setNext(BumpChunk *succ) { next_ = succ; }

    size_t used() const { return bump - bumpBase(); }

    void *start() const { return bumpBase(); }
    void *end() const { return limit; }

    size_t sizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) {
        return mallocSizeOf(this);
    }

    size_t computedSizeOfIncludingThis() {
        return limit - headerBase();
    }

    void resetBump() {
        setBump(headerBase() + sizeof(BumpChunk));
    }

    void *mark() const { return bump; }

    void release(void *mark) {
        JS_ASSERT(contains(mark));
        JS_ASSERT(mark <= bump);
        setBump(mark);
    }

    bool contains(void *mark) const {
        return bumpBase() <= mark && mark <= limit;
    }

    bool canAlloc(size_t n);

    size_t unused() {
        return limit - AlignPtr(bump);
    }

    
    MOZ_ALWAYS_INLINE
    void *tryAlloc(size_t n) {
        char *aligned = AlignPtr(bump);
        char *newBump = aligned + n;

        if (newBump > limit)
            return nullptr;

        
        if (MOZ_UNLIKELY(newBump < bump))
            return nullptr;

        JS_ASSERT(canAlloc(n)); 
        setBump(newBump);
        return aligned;
    }

    static BumpChunk *new_(size_t chunkSize);
    static void delete_(BumpChunk *chunk);
};

} 

void
CrashAtUnhandlableOOM(const char *reason);





class LifoAlloc
{
    typedef detail::BumpChunk BumpChunk;

    BumpChunk   *first;
    BumpChunk   *latest;
    BumpChunk   *last;
    size_t      markCount;
    size_t      defaultChunkSize_;
    size_t      curSize_;
    size_t      peakSize_;

    void operator=(const LifoAlloc &) MOZ_DELETE;
    LifoAlloc(const LifoAlloc &) MOZ_DELETE;

    
    
    
    
    
    BumpChunk *getOrCreateChunk(size_t n);

    void reset(size_t defaultChunkSize) {
        JS_ASSERT(mozilla::RoundUpPow2(defaultChunkSize) == defaultChunkSize);
        first = latest = last = nullptr;
        defaultChunkSize_ = defaultChunkSize;
        markCount = 0;
        curSize_ = 0;
    }

    
    void appendUnused(BumpChunk *start, BumpChunk *end) {
        JS_ASSERT(start && end);
        if (last)
            last->setNext(start);
        else
            first = latest = start;
        last = end;
    }

    
    
    void appendUsed(BumpChunk *start, BumpChunk *latest, BumpChunk *end) {
        JS_ASSERT(start && latest &&  end);
        if (last)
            last->setNext(start);
        else
            first = latest = start;
        last = end;
        this->latest = latest;
    }

    void incrementCurSize(size_t size) {
        curSize_ += size;
        if (curSize_ > peakSize_)
            peakSize_ = curSize_;
    }
    void decrementCurSize(size_t size) {
        JS_ASSERT(curSize_ >= size);
        curSize_ -= size;
    }

  public:
    explicit LifoAlloc(size_t defaultChunkSize)
      : peakSize_(0)
    {
        reset(defaultChunkSize);
    }

    
    void steal(LifoAlloc *other) {
        JS_ASSERT(!other->markCount);

        
        
        size_t oldPeakSize = peakSize_;
        mozilla::PodAssign(this, other);
        peakSize_ = Max(oldPeakSize, curSize_);

        other->reset(defaultChunkSize_);
    }

    
    void transferFrom(LifoAlloc *other);

    
    void transferUnusedFrom(LifoAlloc *other);

    ~LifoAlloc() { freeAll(); }

    size_t defaultChunkSize() const { return defaultChunkSize_; }

    
    void freeAll();

    static const unsigned HUGE_ALLOCATION = 50 * 1024 * 1024;
    void freeAllIfHugeAndUnused() {
        if (markCount == 0 && curSize_ > HUGE_ALLOCATION)
            freeAll();
    }

    MOZ_ALWAYS_INLINE
    void *alloc(size_t n) {
        JS_OOM_POSSIBLY_FAIL();

        void *result;
        if (latest && (result = latest->tryAlloc(n)))
            return result;

        if (!getOrCreateChunk(n))
            return nullptr;

        
        result = latest->tryAlloc(n);
        MOZ_ASSERT(result);
        return result;
    }

    MOZ_ALWAYS_INLINE
    void *allocInfallible(size_t n) {
        if (void *result = alloc(n))
            return result;
        CrashAtUnhandlableOOM("LifoAlloc::allocInfallible");
        return nullptr;
    }

    
    
    
    MOZ_ALWAYS_INLINE
    bool ensureUnusedApproximate(size_t n) {
        size_t total = 0;
        for (BumpChunk *chunk = latest; chunk; chunk = chunk->next()) {
            total += chunk->unused();
            if (total >= n)
                return true;
        }
        BumpChunk *latestBefore = latest;
        if (!getOrCreateChunk(n))
            return false;
        if (latestBefore)
            latest = latestBefore;
        return true;
    }

    template <typename T>
    T *newArray(size_t count) {
        JS_STATIC_ASSERT(mozilla::IsPod<T>::value);
        return newArrayUninitialized<T>(count);
    }

    
    
    template <typename T>
    T *newArrayUninitialized(size_t count) {
        if (count & mozilla::tl::MulOverflowMask<sizeof(T)>::value)
            return nullptr;
        return static_cast<T *>(alloc(sizeof(T) * count));
    }

    class Mark {
        BumpChunk *chunk;
        void *markInChunk;
        friend class LifoAlloc;
        Mark(BumpChunk *chunk, void *markInChunk) : chunk(chunk), markInChunk(markInChunk) {}
      public:
        Mark() : chunk(nullptr), markInChunk(nullptr) {}
    };

    Mark mark() {
        markCount++;
        return latest ? Mark(latest, latest->mark()) : Mark();
    }

    void release(Mark mark) {
        markCount--;
        if (!mark.chunk) {
            latest = first;
            if (latest)
                latest->resetBump();
        } else {
            latest = mark.chunk;
            latest->release(mark.markInChunk);
        }
    }

    void releaseAll() {
        JS_ASSERT(!markCount);
        latest = first;
        if (latest)
            latest->resetBump();
    }

    
    size_t used() const {
        size_t accum = 0;
        for (BumpChunk *chunk = first; chunk; chunk = chunk->next()) {
            accum += chunk->used();
            if (chunk == latest)
                break;
        }
        return accum;
    }

    
    bool isEmpty() const {
        return !latest || !latest->used();
    }

    
    
    size_t availableInCurrentChunk() const {
        if (!latest)
            return 0;
        return latest->unused();
    }

    
    size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const {
        size_t n = 0;
        for (BumpChunk *chunk = first; chunk; chunk = chunk->next())
            n += chunk->sizeOfIncludingThis(mallocSizeOf);
        return n;
    }

    
    size_t sizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const {
        return mallocSizeOf(this) + sizeOfExcludingThis(mallocSizeOf);
    }

    
    
    size_t peakSizeOfExcludingThis() const { return peakSize_; }

    
    template <typename T>
    MOZ_ALWAYS_INLINE
    T *newPod() {
        return static_cast<T *>(alloc(sizeof(T)));
    }

    JS_DECLARE_NEW_METHODS(new_, alloc, MOZ_ALWAYS_INLINE)
    JS_DECLARE_NEW_METHODS(newInfallible, allocInfallible, MOZ_ALWAYS_INLINE)

    
    class Enum
    {
        friend class LifoAlloc;
        friend class detail::BumpChunk;

        LifoAlloc *alloc_;  
        BumpChunk *chunk_;  
        char *position_;    

        
        
        void ensureSpaceAndAlignment(size_t size) {
            JS_ASSERT(!empty());
            char *aligned = detail::AlignPtr(position_);
            if (aligned + size > chunk_->end()) {
                chunk_ = chunk_->next();
                position_ = static_cast<char *>(chunk_->start());
            } else {
                position_ = aligned;
            }
            JS_ASSERT(uintptr_t(position_) + size <= uintptr_t(chunk_->end()));
        }

      public:
        explicit Enum(LifoAlloc &alloc)
          : alloc_(&alloc),
            chunk_(alloc.first),
            position_(static_cast<char *>(alloc.first ? alloc.first->start() : nullptr))
        {}

        
        bool empty() {
            return !chunk_ || (chunk_ == alloc_->latest && position_ >= chunk_->mark());
        }

        
        template <typename T>
        void popFront() {
            popFront(sizeof(T));
        }

        
        void popFront(size_t size) {
            ensureSpaceAndAlignment(size);
            position_ = position_ + size;
        }

        
        template <typename T>
        void updateFront(const T &t) {
            ensureSpaceAndAlignment(sizeof(T));
            memmove(position_, &t, sizeof(T));
        }

        
        
        template <typename T>
        T *get(size_t size = sizeof(T)) {
            ensureSpaceAndAlignment(size);
            return reinterpret_cast<T *>(position_);
        }

        
        Mark mark() {
            alloc_->markCount++;
            return Mark(chunk_, position_);
        }
    };
};

class LifoAllocScope
{
    LifoAlloc       *lifoAlloc;
    LifoAlloc::Mark mark;
    bool            shouldRelease;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    explicit LifoAllocScope(LifoAlloc *lifoAlloc
                            MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : lifoAlloc(lifoAlloc),
        mark(lifoAlloc->mark()),
        shouldRelease(true)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    ~LifoAllocScope() {
        if (shouldRelease)
            lifoAlloc->release(mark);
    }

    LifoAlloc &alloc() {
        return *lifoAlloc;
    }

    void releaseEarly() {
        JS_ASSERT(shouldRelease);
        lifoAlloc->release(mark);
        shouldRelease = false;
    }
};

enum Fallibility {
    Fallible,
    Infallible
};

template <Fallibility fb>
class LifoAllocPolicy
{
    LifoAlloc &alloc_;

  public:
    MOZ_IMPLICIT LifoAllocPolicy(LifoAlloc &alloc)
      : alloc_(alloc)
    {}
    template <typename T>
    T *pod_malloc(size_t numElems) {
        if (numElems & mozilla::tl::MulOverflowMask<sizeof(T)>::value)
            return nullptr;
        size_t bytes = numElems * sizeof(T);
        void *p = fb == Fallible ? alloc_.alloc(bytes) : alloc_.allocInfallible(bytes);
        return static_cast<T *>(p);
    }
    template <typename T>
    T *pod_calloc(size_t numElems) {
        T *p = pod_malloc<T>(numElems);
        if (fb == Fallible && !p)
            return nullptr;
        memset(p, 0, numElems * sizeof(T));
        return p;
    }
    template <typename T>
    T *pod_realloc(T *p, size_t oldSize, size_t newSize) {
        T *n = pod_malloc<T>(newSize);
        if (fb == Fallible && !n)
            return nullptr;
        JS_ASSERT(!(oldSize & mozilla::tl::MulOverflowMask<sizeof(T)>::value));
        memcpy(n, p, Min(oldSize * sizeof(T), newSize * sizeof(T)));
        return n;
    }
    void free_(void *p) {
    }
    void reportAllocOverflow() const {
    }
};

} 

#endif 
