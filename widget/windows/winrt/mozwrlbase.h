




#pragma once









#if _WIN32_WINNT < 0x600

#include <windows.h>

VOID
WINAPI
ReleaseSRWLockExclusive(
    _Inout_ PSRWLOCK SRWLock
    );

VOID
WINAPI
ReleaseSRWLockShared(
    _Inout_ PSRWLOCK SRWLock
    );

BOOL
WINAPI
InitializeCriticalSectionEx(
    _Out_ LPCRITICAL_SECTION lpCriticalSection,
    _In_ DWORD dwSpinCount,
    _In_ DWORD Flags
    );

VOID
WINAPI
InitializeSRWLock(
    _Out_ PSRWLOCK SRWLock
    );

VOID
WINAPI
AcquireSRWLockExclusive(
    _Inout_ PSRWLOCK SRWLock
    );

BOOLEAN
WINAPI
TryAcquireSRWLockExclusive(
    _Inout_ PSRWLOCK SRWLock
    );

BOOLEAN
WINAPI
TryAcquireSRWLockShared(
    _Inout_ PSRWLOCK SRWLock
    );

VOID
WINAPI
AcquireSRWLockShared(
    _Inout_ PSRWLOCK SRWLock
    );

#undef WINVER
#undef _WIN32_WINNT
#define WINVER 0x600
#define _WIN32_WINNT 0x600

#endif 

#include <wrl.h>
