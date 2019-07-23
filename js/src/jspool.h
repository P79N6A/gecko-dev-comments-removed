






































#ifndef jspool_h_
#define jspool_h_

#include "jstl.h"

#include <new>

namespace js {

























template <class T, size_t N, class AllocPolicy>
class Pool : AllocPolicy
{
    typedef typename tl::StaticAssert<sizeof(T) >= sizeof(void *)>::result _;

    
    Pool(const Pool &);
    void operator=(const Pool &);

    
    void addRangeToFreeList(char *begin, char *end);
    bool addChunk();
    void freeChunkChain();

    
    static const size_t sGrowthFactor = 2;
    static const size_t sInitialAllocElems = 32;

    

    static const size_t sInlineBytes =
        N * sizeof(T);

    static const size_t sInitialAllocBytes =
        tl::Max<2 * sInlineBytes, sInitialAllocElems * sizeof(T)>::result;

    

#ifdef DEBUG
    friend class ReentrancyGuard;
    bool entered;
    size_t allocCount;
#endif

    




    template <size_t NonZeroBytes, class>
    struct MemberData
    {
        
        char buf[sInlineBytes];

        
        void *chunkHead;

        
        size_t lastAlloc;

        
        void *freeHead;

        void init(Pool &p) {
            chunkHead = NULL;
            lastAlloc = 0;
            freeHead = NULL;
            p.addRangeToFreeList(buf, tl::ArrayEnd(buf));
        }
    };

    MemberData<sInlineBytes, void> m;

  public:

    Pool(AllocPolicy = AllocPolicy());
    ~Pool();

    AllocPolicy &allocPolicy() { return *this; }

    



    T *create();

    
    void destroy(T *);

    





    void lendUnusedMemory(void *begin, void *end);

    



    template <class Range>
    void clear(Range r);

    



    void freeRawMemory();

    






    template <class InputRange>
    T *consolidate(InputRange i, size_t count);
};








template <class T, size_t N, class AP>
template <class Unused>
struct Pool<T,N,AP>::MemberData<0,Unused>
{
    void *chunkHead;
    size_t lastAlloc;
    void *freeHead;

    void init(Pool<T,N,AP> &) {
        chunkHead = NULL;
        lastAlloc = 0;
        freeHead = NULL;
    }
};





template <class T, size_t N, class AP>
inline void
Pool<T,N,AP>::addRangeToFreeList(char *begin, char *end)
{
    JS_ASSERT((end - begin) % sizeof(T) == 0);
    void *oldHead = m.freeHead;
    void **last = &m.freeHead;
    for (char *p = begin; p != end; p += sizeof(T)) {
        *last = p;
        last = reinterpret_cast<void **>(p);
    }
    *last = oldHead;
}

template <class T, size_t N, class AP>
inline
Pool<T,N,AP>::Pool(AP ap)
  : AP(ap)
#ifdef DEBUG
    , entered(false), allocCount(0)
#endif
{
    m.init(*this);
}

template <class T, size_t N, class AP>
inline void
Pool<T,N,AP>::freeChunkChain()
{
    void *p = m.chunkHead;
    while (p) {
        void *next = *reinterpret_cast<void **>(p);
        this->free(p);
        p = next;
    }
}

template <class T, size_t N, class AP>
inline
Pool<T,N,AP>::~Pool()
{
    JS_ASSERT(allocCount == 0 && !entered);
    freeChunkChain();
}

template <class T, size_t N, class AP>
inline bool
Pool<T,N,AP>::addChunk()
{
    
    if (m.lastAlloc & tl::MulOverflowMask<2 * sGrowthFactor>::result) {
        this->reportAllocOverflow();
        return false;
    }

    if (!m.lastAlloc)
        m.lastAlloc = sInitialAllocBytes;
    else
        m.lastAlloc *= sGrowthFactor;
    char *bytes = (char *)this->malloc(m.lastAlloc);
    if (!bytes)
        return false;

    



    *reinterpret_cast<void **>(bytes) = m.chunkHead;
    m.chunkHead = bytes;
    addRangeToFreeList(bytes + sizeof(T), bytes + m.lastAlloc);
    return true;
}

template <class T, size_t N, class AP>
inline T *
Pool<T,N,AP>::create()
{
    ReentrancyGuard g(*this);
    if (!m.freeHead && !addChunk())
        return NULL;
    void *objMem = m.freeHead;
    m.freeHead = *reinterpret_cast<void **>(m.freeHead);
#ifdef DEBUG
    ++allocCount;
#endif
    return new(objMem) T();
}

template <class T, size_t N, class AP>
inline void
Pool<T,N,AP>::destroy(T *p)
{
    ReentrancyGuard g(*this);
    JS_ASSERT(p && allocCount-- > 0);
    p->~T();
    *reinterpret_cast<void **>(p) = m.freeHead;
    m.freeHead = p;
}

template <class T, size_t N, class AP>
inline void
Pool<T,N,AP>::lendUnusedMemory(void *vbegin, void *vend)
{
    JS_ASSERT(!entered);
    char *begin = (char *)vbegin, *end = (char *)vend;
    size_t mod = (end - begin) % sizeof(T);
    if (mod)
        end -= mod;
    addRangeToFreeList(begin, end);
}

template <class T, size_t N, class AP>
inline void
Pool<T,N,AP>::freeRawMemory()
{
    JS_ASSERT(!entered);
#ifdef DEBUG
    allocCount = 0;
#endif
    freeChunkChain();
    m.init(*this);
}

template <class T, size_t N, class AP>
template <class Range>
inline void
Pool<T,N,AP>::clear(Range r)
{
    typedef typename Range::ElementType Elem;
    for (; !r.empty(); r.popFront())
        r.front().~Elem();
    freeRawMemory();
}

template <class T, size_t N, class AP>
template <class Range>
inline T *
Pool<T,N,AP>::consolidate(Range r, size_t count)
{
    
    size_t size = (count + 1) * sizeof(T);
    char *bytes = (char *)this->malloc(size);
    if (!bytes)
        return NULL;

    
    *reinterpret_cast<void **>(bytes) = NULL;
    T *arrayBegin = reinterpret_cast<T *>(bytes);
    ++arrayBegin;  
    for (T *dst = arrayBegin; !r.empty(); r.popFront(), ++dst) {
        JS_ASSERT(count-- > 0);
        new(dst) T(r.front());
        r.front().~T();
    }
    JS_ASSERT(count == 0);

    freeChunkChain();

    
    m.init(*this);
    m.chunkHead = bytes;
    m.lastAlloc = size;
    return arrayBegin;
}

}

#endif 
