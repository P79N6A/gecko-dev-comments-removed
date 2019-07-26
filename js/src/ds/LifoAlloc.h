






#ifndef LifoAlloc_h__
#define LifoAlloc_h__

#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/GuardObjects.h"
#include "mozilla/MemoryChecking.h"
#include "mozilla/TypeTraits.h"






#include "jsutil.h"

#include "js/TemplateLib.h"

namespace js {

namespace detail {

static const size_t LIFO_ALLOC_ALIGN = 8;

JS_ALWAYS_INLINE
char *
AlignPtr(void *orig)
{
    typedef tl::StaticAssert<
        tl::FloorLog2<LIFO_ALLOC_ALIGN>::result == tl::CeilingLog2<LIFO_ALLOC_ALIGN>::result
    >::result _;

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

    BumpChunk *thisDuringConstruction() { return this; }

    explicit BumpChunk(size_t bumpSpaceSize)
      : bump(reinterpret_cast<char *>(thisDuringConstruction()) + sizeof(BumpChunk)),
        limit(bump + bumpSpaceSize),
        next_(NULL), bumpSpaceSize(bumpSpaceSize)
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

    size_t sizeOfIncludingThis(JSMallocSizeOfFun mallocSizeOf) {
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

    
    JS_ALWAYS_INLINE
    void *tryAlloc(size_t n) {
        char *aligned = AlignPtr(bump);
        char *newBump = aligned + n;

        if (newBump > limit)
            return NULL;

        
        if (JS_UNLIKELY(newBump < bump))
            return NULL;

        JS_ASSERT(canAlloc(n)); 
        setBump(newBump);
        return aligned;
    }

    void *allocInfallible(size_t n) {
        void *result = tryAlloc(n);
        JS_ASSERT(result);
        return result;
    }

    static BumpChunk *new_(size_t chunkSize);
    static void delete_(BumpChunk *chunk);
};

} 





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
        JS_ASSERT(RoundUpPow2(defaultChunkSize) == defaultChunkSize);
        first = latest = last = NULL;
        defaultChunkSize_ = defaultChunkSize;
        markCount = 0;
        curSize_ = 0;
    }

    void append(BumpChunk *start, BumpChunk *end) {
        JS_ASSERT(start && end);
        if (last)
            last->setNext(start);
        else
            first = latest = start;
        last = end;
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
        PodCopy((char *) this, (char *) other, sizeof(*this));
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

    JS_ALWAYS_INLINE
    void *alloc(size_t n) {
        JS_OOM_POSSIBLY_FAIL();

        void *result;
        if (latest && (result = latest->tryAlloc(n)))
            return result;

        if (!getOrCreateChunk(n))
            return NULL;

        return latest->allocInfallible(n);
    }

    JS_ALWAYS_INLINE
    void *allocInfallible(size_t n) {
        void *result;
        if (latest && (result = latest->tryAlloc(n)))
            return result;

        mozilla::DebugOnly<BumpChunk *> chunk = getOrCreateChunk(n);
        JS_ASSERT(chunk);

        return latest->allocInfallible(n);
    }

    
    
    
    JS_ALWAYS_INLINE
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
        void *mem = alloc(sizeof(T) * count);
        if (!mem)
            return NULL;
        JS_STATIC_ASSERT(mozilla::IsPod<T>::value);
        return (T *) mem;
    }

    
    
    template <typename T>
    T *newArrayUninitialized(size_t count) {
        return static_cast<T *>(alloc(sizeof(T) * count));
    }

    void *mark() {
        markCount++;

        return latest ? latest->mark() : NULL;
    }

    void release(void *mark) {
        markCount--;

        if (!mark) {
            latest = first;
            if (latest)
                latest->resetBump();
            return;
        }

        
        
        
        BumpChunk *container;
        for (container = first; !container->contains(mark); container = container->next())
            JS_ASSERT(container != latest);

        latest = container;
        latest->release(mark);
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

    
    size_t sizeOfExcludingThis(JSMallocSizeOfFun mallocSizeOf) const {
        size_t n = 0;
        for (BumpChunk *chunk = first; chunk; chunk = chunk->next())
            n += chunk->sizeOfIncludingThis(mallocSizeOf);
        return n;
    }

    
    size_t sizeOfIncludingThis(JSMallocSizeOfFun mallocSizeOf) const {
        return mallocSizeOf(this) + sizeOfExcludingThis(mallocSizeOf);
    }

    
    
    size_t peakSizeOfExcludingThis() const { return peakSize_; }

    
    template <typename T>
    JS_ALWAYS_INLINE
    T *newPod() {
        return static_cast<T *>(alloc(sizeof(T)));
    }

    JS_DECLARE_NEW_METHODS(new_, alloc, JS_ALWAYS_INLINE)
};

class LifoAllocScope
{
    LifoAlloc   *lifoAlloc;
    void        *mark;
    bool        shouldRelease;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    explicit LifoAllocScope(LifoAlloc *lifoAlloc
                            MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : lifoAlloc(lifoAlloc), shouldRelease(true)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        mark = lifoAlloc->mark();
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

} 

#endif
