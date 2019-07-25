




































#ifndef nsAppShell_h
#define nsAppShell_h

#include <queue>

#include "mozilla/Mutex.h"
#include "nsBaseAppShell.h"
#include "nsRect.h"
#include "nsTArray.h"

#include "utils/RefBase.h"

namespace mozilla {
bool ProcessNextEvent();
void NotifyEvent();
}

extern bool gDrawRequest;

class FdHandler;
typedef void(*FdHandlerCallback)(int, FdHandler *);

class FdHandler {
public:
    FdHandler()
    {
        memset(name, 0, sizeof(name));
    }

    int fd;
    char name[64];
    FdHandlerCallback func;
    void run()
    {
        func(fd, this);
    }
};

namespace android {
class EventHub;
class InputReader;
class InputReaderThread;
}

class GeckoInputReaderPolicy;
class GeckoInputDispatcher;

class nsAppShell : public nsBaseAppShell {
public:
    nsAppShell();

    nsresult Init();
    virtual bool ProcessNextNativeEvent(bool maywait);

    void NotifyNativeEvent();

protected:
    virtual ~nsAppShell();

    virtual void ScheduleNativeEventCallback();

private:
    nsresult AddFdHandler(int fd, FdHandlerCallback handlerFunc,
                          const char* deviceName);

    
    bool mNativeCallbackRequest;
    nsTArray<FdHandler> mHandlers;

    android::sp<android::EventHub>               mEventHub;
    android::sp<GeckoInputReaderPolicy> mReaderPolicy;
    android::sp<GeckoInputDispatcher>   mDispatcher;
    android::sp<android::InputReader>            mReader;
    android::sp<android::InputReaderThread>      mReaderThread;
};

#endif 

