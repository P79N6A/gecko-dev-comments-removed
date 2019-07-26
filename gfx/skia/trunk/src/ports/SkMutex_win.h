






#ifndef SkMutex_win_DEFINED
#define SkMutex_win_DEFINED



#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#  define WIN32_IS_MEAN_WAS_LOCALLY_DEFINED
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#  define NOMINMAX_WAS_LOCALLY_DEFINED
#endif
#
#include <windows.h>
#
#ifdef WIN32_IS_MEAN_WAS_LOCALLY_DEFINED
#  undef WIN32_IS_MEAN_WAS_LOCALLY_DEFINED
#  undef WIN32_LEAN_AND_MEAN
#endif
#ifdef NOMINMAX_WAS_LOCALLY_DEFINED
#  undef NOMINMAX_WAS_LOCALLY_DEFINED
#  undef NOMINMAX
#endif



class SkMutex {
public:
    SkMutex() {
        InitializeCriticalSection(&fStorage);
    }

    ~SkMutex() {
        DeleteCriticalSection(&fStorage);
    }

    void acquire() {
        EnterCriticalSection(&fStorage);
    }

    void release() {
        LeaveCriticalSection(&fStorage);
    }

private:
    SkMutex(const SkMutex&);
    SkMutex& operator=(const SkMutex&);

    CRITICAL_SECTION fStorage;
};

typedef SkMutex SkBaseMutex;


#define SK_DECLARE_STATIC_MUTEX(name) static SkBaseMutex name
#define SK_DECLARE_GLOBAL_MUTEX(name) SkBaseMutex name

#endif
