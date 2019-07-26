






#ifndef LifoAlloc_h__
#define LifoAlloc_h__

#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/GuardObjects.h"
#include "mozilla/ASan.h"

#if defined(MOZ_VALGRIND)
#include "valgrind/memcheck.h"
#endif








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
#if defined(DEBUG) || defined(MOZ_ASAN) || defined(MOZ_VALGRIND)
        char* prevBump = bump;
#endif
        bump = static_cast<char *>(ptr);
#ifdef DEBUG
        JS_ASSERT(contains(prevBump));

        
        if (prevBump > bump)
            memset(bump, 0xcd, prevBump - bump);
#endif

        
#if defined(MOZ_ASAN)
        if (prevBump > bump)
            ASAN_POISON_MEMORY_REGION(bump, prevBump - bump);
        else if (bump > prevBump)
            ASAN_UNPOISON_MEMORY_REGION(prevBump, bump - prevBump);
#elif defined(MOZ_VALGRIND)
        if (prevBump > bump)
            VALGRIND_MAKE_MEM_NOACCESS(bump, prevBump - bump);
        else if (bump > prevBump)
            VALGRIND_MAKE_MEM_UNDEFINED(prevBump, bump - prevBump);
#endif
    }

  public:
    BumpChunk *next() const { return next_; }
    void setNext(BumpChunk *succ) { next_ = succ; }

    size_t used() const { return bump - bumpBase(); }
    size_t sizeOfIncludingThis(JSMallocSizeOfFun mallocSizeOf) {
        return mallocSizeOf(this);
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

    void operator=(const LifoAlloc &) MOZ_DELETE;
    LifoAlloc(const LifoAlloc &) MOZ_DELETE;

    






    BumpChunk *getOrCreateChunk(size_t n);

    void reset(size_t defaultChunkSize) {
        JS_ASSERT(RoundUpPow2(defaultChunkSize) == defaultChunkSize);
        first = latest = last = NULL;
        defaultChunkSize_ = defaultChunkSize;
        markCount = 0;
    }

    void append(BumpChunk *start, BumpChunk *end) {
        JS_ASSERT(start && end);
        if (last)
            last->setNext(start);
        else
            first = latest = start;
        last = end;
    }

  public:
    explicit LifoAlloc(size_t defaultChunkSize) { reset(defaultChunkSize); }

    
    void steal(LifoAlloc *other) {
        JS_ASSERT(!other->markCount);
        PodCopy((char *) this, (char *) other, sizeof(*this));
        other->reset(defaultChunkSize_);
    }

    
    void transferFrom(LifoAlloc *other);

    
    void transferUnusedFrom(LifoAlloc *other);

    ~LifoAlloc() { freeAll(); }

    size_t defaultChunkSize() const { return defaultChunkSize_; }

    
    void freeAll();

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
        BumpChunk *chunk = latest;
        while (chunk) {
            total += chunk->unused();
            if (total >= n)
                return true;
            chunk = chunk->next();
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
        JS_STATIC_ASSERT(tl::IsPodType<T>::result);
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

        




        BumpChunk *container = first;
        while (true) {
            if (container->contains(mark))
                break;
            JS_ASSERT(container != latest);
            container = container->next();
        }
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
        BumpChunk *it = first;
        while (it) {
            accum += it->used();
            if (it == latest)
                break;
            it = it->next();
        }
        return accum;
    }

    
    size_t sizeOfExcludingThis(JSMallocSizeOfFun mallocSizeOf) const {
        size_t accum = 0;
        BumpChunk *it = first;
        while (it) {
            accum += it->sizeOfIncludingThis(mallocSizeOf);
            it = it->next();
        }
        return accum;
    }

    
    size_t sizeOfIncludingThis(JSMallocSizeOfFun mallocSizeOf) const {
        return mallocSizeOf(this) + sizeOfExcludingThis(mallocSizeOf);
    }

    
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
