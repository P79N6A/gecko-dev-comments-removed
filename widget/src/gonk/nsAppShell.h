




































#ifndef nsAppShell_h
#define nsAppShell_h

#include "nsBaseAppShell.h"
#include "nsRect.h"
#include "nsTArray.h"

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
        : mtState(MT_START)
        , keyCode(0)
        , mtDown(false)
        , calibrated(false)
    {
        memset(name, 0, sizeof(name));
    }

    int fd;
    char name[64];
    FdHandlerCallback func;
    enum mtStates {
        MT_START,
        MT_COLLECT,
        MT_IGNORE
    } mtState;
    int mtX, mtY;
    int mtMajor;
    int keyCode;
    bool mtDown;
    
    
    
    bool calibrated;
    
    
    
    
    
    
    
    
    
    
    
    
    int inputMinX, inputMinY;
    float inputToScreenScaleX, inputToScreenScaleY;
    
    
    
    
    
    static const size_t kMaxVButtons = 4;
    struct VButton {
        nsIntRect buttonRect;   
        int keyCode;
    } vbuttons[kMaxVButtons];

    void run()
    {
        func(fd, this);
    }

    int inputXToScreenX(int inputX) {
        return inputToScreenScaleX * (inputX - inputMinX);
    }
    int inputYToScreenY(int inputY) {
        return inputToScreenScaleY * (inputY - inputMinY);
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

private:
    nsresult AddFdHandler(int fd, FdHandlerCallback handlerFunc,
                          const char* deviceName);

    
    bool mNativeCallbackRequest;
    nsTArray<FdHandler> mHandlers;
};

#endif 

