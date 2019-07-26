






#ifndef ForkJoin_inl_h__
#define ForkJoin_inl_h__

namespace js {

inline ForkJoinSlice *
ForkJoinSlice::current()
{
#ifdef JS_THREADSAFE_ION
    return (ForkJoinSlice*) PR_GetThreadPrivate(ThreadPrivateIndex);
#else
    return NULL;
#endif
}



static inline bool
InParallelSection()
{
#ifdef JS_THREADSAFE
    return ForkJoinSlice::current() != NULL;
#else
    return false;
#endif
}

} 

#endif 
