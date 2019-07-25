




































#ifndef nsAppShell_h
#define nsAppShell_h

#include "nsBaseAppShell.h"

namespace mozilla {
bool ProcessNextEvent();
void NotifyEvent();
}

extern bool gDrawRequest;

class FdHandler;
typedef void(*FdHandlerCallback)(int, FdHandler *);

class FdHandler {
public:
    FdHandler() : mtState(MT_START), mtDown(false) { }

    int fd;
    FdHandlerCallback func;
    enum mtStates {
        MT_START,
        MT_COLLECT,
        MT_IGNORE
    } mtState;
    int mtX, mtY;
    int mtMajor;
    bool mtDown;

    void run()
    {
        func(fd, this);
    }
};

class nsAppShell : public nsBaseAppShell {
public:
    nsAppShell();

    nsresult Init();
    virtual bool ProcessNextNativeEvent(bool maywait);

    void NotifyNativeEvent();

protected:
    virtual ~nsAppShell();

    virtual void ScheduleNativeEventCallback();

    
    bool mNativeCallbackRequest;
    nsTArray<FdHandler> mHandlers;
};

#endif 

