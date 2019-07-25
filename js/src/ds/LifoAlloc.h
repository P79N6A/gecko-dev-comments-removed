







































#ifndef LifoAlloc_h__
#define LifoAlloc_h__








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
        DebugOnly<char *> prevBump = bump;
        bump = static_cast<char *>(ptr);
#ifdef DEBUG
        JS_ASSERT(contains(prevBump));

        
        if (prevBump > bump)
            memset(bump, 0xcd, prevBump - bump);
#endif
    }

  public:
    BumpChunk *next() const { return next_; }
    void setNext(BumpChunk *succ) { next_ = succ; }

    size_t used() const { return bump - bumpBase(); }

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
    bool canAllocUnaligned(size_t n);

    
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

    void *tryAllocUnaligned(size_t n);

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
    size_t      markCount;
    size_t      defaultChunkSize_;

    void operator=(const LifoAlloc &);
    LifoAlloc(const LifoAlloc &);

    






    BumpChunk *getOrCreateChunk(size_t n);

    void reset(size_t defaultChunkSize) {
        JS_ASSERT(RoundUpPow2(defaultChunkSize) == defaultChunkSize);
        first = latest = NULL;
        defaultChunkSize_ = defaultChunkSize;
        markCount = 0;
    }

  public:
    explicit LifoAlloc(size_t defaultChunkSize) { reset(defaultChunkSize); }

    
    void steal(LifoAlloc *other) {
        JS_ASSERT(!other->markCount);
        PodCopy((char *) this, (char *) other, sizeof(*this));
        other->reset(defaultChunkSize_);
    }

    ~LifoAlloc() { freeAll(); }

    size_t defaultChunkSize() const { return defaultChunkSize_; }

    
    void freeAll();

    
    void freeUnused();

    JS_ALWAYS_INLINE
    void *alloc(size_t n) {
        void *result;
        if (latest && (result = latest->tryAlloc(n)))
            return result;

        if (!getOrCreateChunk(n))
            return NULL;

        return latest->allocInfallible(n);
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

    
    size_t used() const {
        size_t accum = 0;
        BumpChunk *it = first;
        while (it) {
            accum += it->used();
            it = it->next();
        }
        return accum;
    }

    
    template <typename T>
    JS_ALWAYS_INLINE
    T *newPod() {
        return static_cast<T *>(alloc(sizeof(T)));
    }

    JS_DECLARE_NEW_METHODS(alloc, JS_ALWAYS_INLINE)

    

    void *allocUnaligned(size_t n);
    void *reallocUnaligned(void *origPtr, size_t origSize, size_t incr);
};

class LifoAllocScope {
    LifoAlloc   *lifoAlloc;
    void        *mark;
    bool        shouldRelease;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    explicit LifoAllocScope(LifoAlloc *lifoAlloc
                            JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : lifoAlloc(lifoAlloc), shouldRelease(true) {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        mark = lifoAlloc->mark();
    }

    ~LifoAllocScope() {
        if (shouldRelease)
            lifoAlloc->release(mark);
    }

    void releaseEarly() {
        JS_ASSERT(shouldRelease);
        lifoAlloc->release(mark);
        shouldRelease = false;
    }
};

} 

#endif
