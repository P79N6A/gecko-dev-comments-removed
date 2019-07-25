








#include <windows.h>
#include "SkThread.h"

int32_t sk_atomic_inc(int32_t* addr)
{
    
    return InterlockedIncrement(reinterpret_cast<LONG*>(addr)) - 1;
}

int32_t sk_atomic_dec(int32_t* addr)
{
    return InterlockedDecrement(reinterpret_cast<LONG*>(addr)) + 1;
}

SkMutex::SkMutex(bool )
{
    SK_COMPILE_ASSERT(sizeof(fStorage) > sizeof(CRITICAL_SECTION),
                      NotEnoughSizeForCriticalSection);
    InitializeCriticalSection(reinterpret_cast<CRITICAL_SECTION*>(&fStorage));
}

SkMutex::~SkMutex()
{
    DeleteCriticalSection(reinterpret_cast<CRITICAL_SECTION*>(&fStorage));
}

void SkMutex::acquire()
{
    EnterCriticalSection(reinterpret_cast<CRITICAL_SECTION*>(&fStorage));
}

void SkMutex::release()
{
    LeaveCriticalSection(reinterpret_cast<CRITICAL_SECTION*>(&fStorage));
}

