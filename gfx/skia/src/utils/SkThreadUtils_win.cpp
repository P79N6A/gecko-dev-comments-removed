






#include "SkTypes.h"

#include "SkThreadUtils.h"
#include "SkThreadUtils_win.h"

SkThread_WinData::SkThread_WinData(SkThread::entryPointProc entryPoint, void* data)
    : fHandle(NULL)
    , fParam(data)
    , fThreadId(0)
    , fEntryPoint(entryPoint)
    , fStarted(false)
{
    fCancelEvent = CreateEvent(
        NULL,  
        false, 
        false, 
        NULL); 
}

SkThread_WinData::~SkThread_WinData() {
    CloseHandle(fCancelEvent);
}

static DWORD WINAPI thread_start(LPVOID data) {
    SkThread_WinData* winData = static_cast<SkThread_WinData*>(data);

    
    if (WaitForSingleObject(winData->fCancelEvent, 0) == WAIT_OBJECT_0) {
        return 0;
    }

    winData->fEntryPoint(winData->fParam);
    return 0;
}

SkThread::SkThread(entryPointProc entryPoint, void* data) {
    SkThread_WinData* winData = new SkThread_WinData(entryPoint, data);
    fData = winData;

    if (NULL == winData->fCancelEvent) {
        return;
    }

    winData->fHandle = CreateThread(
        NULL,                   
        0,                      
        thread_start,           
        winData,                
        CREATE_SUSPENDED,       
        &winData->fThreadId);   
}

SkThread::~SkThread() {
    if (fData != NULL) {
        SkThread_WinData* winData = static_cast<SkThread_WinData*>(fData);
        
        if (winData->fHandle != NULL && !winData->fStarted) {
            if (SetEvent(winData->fCancelEvent) != 0) {
                if (this->start()) {
                    this->join();
                }
            } else {
                
                TerminateThread(winData->fHandle, -1);
            }
        }
        delete winData;
    }
}

bool SkThread::start() {
    SkThread_WinData* winData = static_cast<SkThread_WinData*>(fData);
    if (NULL == winData->fHandle) {
        return false;
    }

    if (winData->fStarted) {
        return false;
    }
    winData->fStarted = -1 != ResumeThread(winData->fHandle);
    return winData->fStarted;
}

void SkThread::join() {
    SkThread_WinData* winData = static_cast<SkThread_WinData*>(fData);
    if (NULL == winData->fHandle || !winData->fStarted) {
        return;
    }

    WaitForSingleObject(winData->fHandle, INFINITE);
}

static unsigned int num_bits_set(DWORD_PTR mask) {
    unsigned int count;
    for (count = 0; mask; ++count) {
        mask &= mask - 1;
    }
    return count;
}

static unsigned int nth_set_bit(unsigned int n, DWORD_PTR mask) {
    n %= num_bits_set(mask);
    for (unsigned int setBitsSeen = 0, currentBit = 0; true; ++currentBit) {
        if (mask & (1 << currentBit)) {
            ++setBitsSeen;
            if (setBitsSeen > n) {
                return currentBit;
            }
        }
    }
}

bool SkThread::setProcessorAffinity(unsigned int processor) {
    SkThread_WinData* winData = static_cast<SkThread_WinData*>(fData);
    if (NULL == winData->fHandle) {
        return false;
    }

    DWORD_PTR processAffinityMask;
    DWORD_PTR systemAffinityMask;
    if (0 == GetProcessAffinityMask(GetCurrentProcess(),
                                    &processAffinityMask,
                                    &systemAffinityMask)) {
        return false;
    }

    DWORD_PTR threadAffinityMask = 1 << nth_set_bit(processor, processAffinityMask);
    return 0 != SetThreadAffinityMask(winData->fHandle, threadAffinityMask);
}
