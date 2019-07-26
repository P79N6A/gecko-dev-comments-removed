















#ifndef _UI_INPUT_MANAGER_H
#define _UI_INPUT_MANAGER_H





#include "EventHub.h"
#include "InputReader.h"
#include "InputDispatcher.h"

#include "Input.h"
#include "InputTransport.h"
#include <utils/Errors.h>
#include <utils/Vector.h>
#include <utils/Timers.h>
#include <utils/RefBase.h>
#include <utils/String8.h>

namespace android {




















class InputManagerInterface : public virtual RefBase {
protected:
    InputManagerInterface() { }
    virtual ~InputManagerInterface() { }

public:
    
    virtual status_t start() = 0;

    
    virtual status_t stop() = 0;

    
    virtual sp<InputReaderInterface> getReader() = 0;

    
    virtual sp<InputDispatcherInterface> getDispatcher() = 0;
};

class InputManager : public InputManagerInterface {
protected:
    virtual ~InputManager();

public:
    InputManager(
            const sp<EventHubInterface>& eventHub,
            const sp<InputReaderPolicyInterface>& readerPolicy,
            const sp<InputDispatcherPolicyInterface>& dispatcherPolicy);

    
    InputManager(
            const sp<InputReaderInterface>& reader,
            const sp<InputDispatcherInterface>& dispatcher);

    virtual status_t start();
    virtual status_t stop();

    virtual sp<InputReaderInterface> getReader();
    virtual sp<InputDispatcherInterface> getDispatcher();

private:
    sp<InputReaderInterface> mReader;
    sp<InputReaderThread> mReaderThread;

    sp<InputDispatcherInterface> mDispatcher;
    sp<InputDispatcherThread> mDispatcherThread;

    void initialize();
};

} 

#endif 
