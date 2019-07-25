






#ifndef SkThreadUtils_WinData_DEFINED
#define SkThreadUtils_WinData_DEFINED

#include "SkTypes.h"

#include "SkThreadUtils.h"

class SkThread_WinData : SkNoncopyable {
public:
    SkThread_WinData(SkThread::entryPointProc entryPoint, void* data);
    ~SkThread_WinData();
    HANDLE fHandle;
    HANDLE fCancelEvent;

    LPVOID fParam;
    DWORD fThreadId;
    SkThread::entryPointProc fEntryPoint;
    bool fStarted;
};

#endif
