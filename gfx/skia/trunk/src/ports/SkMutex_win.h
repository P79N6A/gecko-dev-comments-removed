






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





struct SkBaseMutex {
public:
    SkBaseMutex() {
        InitializeCriticalSection(&fStorage);
        SkDEBUGCODE(fOwner = 0;)
    }

    ~SkBaseMutex() {
        SkASSERT(0 == fOwner);
        DeleteCriticalSection(&fStorage);
    }

    void acquire() {
        EnterCriticalSection(&fStorage);
        SkDEBUGCODE(fOwner = GetCurrentThreadId();)
    }

    void release() {
        this->assertHeld();
        SkDEBUGCODE(fOwner = 0;)
        LeaveCriticalSection(&fStorage);
    }

    void assertHeld() {
        SkASSERT(GetCurrentThreadId() == fOwner);
    }

protected:
    CRITICAL_SECTION fStorage;
    SkDEBUGCODE(DWORD fOwner;)

private:
    SkBaseMutex(const SkBaseMutex&);
    SkBaseMutex& operator=(const SkBaseMutex&);
};

class SkMutex : public SkBaseMutex { };



#define SK_DECLARE_STATIC_MUTEX(name) namespace{} static SkBaseMutex name

#endif
