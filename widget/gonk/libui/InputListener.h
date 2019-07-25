















#ifndef _UI_INPUT_LISTENER_H
#define _UI_INPUT_LISTENER_H

#include "Input.h"
#include <utils/RefBase.h>
#include <utils/Vector.h>

namespace android {

class InputListenerInterface;



struct NotifyArgs {
    virtual ~NotifyArgs() { }

    virtual void notify(const sp<InputListenerInterface>& listener) const = 0;
};



struct NotifyConfigurationChangedArgs : public NotifyArgs {
    nsecs_t eventTime;

    inline NotifyConfigurationChangedArgs() { }

    NotifyConfigurationChangedArgs(nsecs_t eventTime);

    NotifyConfigurationChangedArgs(const NotifyConfigurationChangedArgs& other);

    virtual ~NotifyConfigurationChangedArgs() { }

    virtual void notify(const sp<InputListenerInterface>& listener) const;
};



struct NotifyKeyArgs : public NotifyArgs {
    nsecs_t eventTime;
    int32_t deviceId;
    uint32_t source;
    uint32_t policyFlags;
    int32_t action;
    int32_t flags;
    int32_t keyCode;
    int32_t scanCode;
    int32_t metaState;
    nsecs_t downTime;

    inline NotifyKeyArgs() { }

    NotifyKeyArgs(nsecs_t eventTime, int32_t deviceId, uint32_t source, uint32_t policyFlags,
            int32_t action, int32_t flags, int32_t keyCode, int32_t scanCode,
            int32_t metaState, nsecs_t downTime);

    NotifyKeyArgs(const NotifyKeyArgs& other);

    virtual ~NotifyKeyArgs() { }

    virtual void notify(const sp<InputListenerInterface>& listener) const;
};



struct NotifyMotionArgs : public NotifyArgs {
    nsecs_t eventTime;
    int32_t deviceId;
    uint32_t source;
    uint32_t policyFlags;
    int32_t action;
    int32_t flags;
    int32_t metaState;
    int32_t buttonState;
    int32_t edgeFlags;
    uint32_t pointerCount;
    PointerProperties pointerProperties[MAX_POINTERS];
    PointerCoords pointerCoords[MAX_POINTERS];
    float xPrecision;
    float yPrecision;
    nsecs_t downTime;

    inline NotifyMotionArgs() { }

    NotifyMotionArgs(nsecs_t eventTime, int32_t deviceId, uint32_t source, uint32_t policyFlags,
            int32_t action, int32_t flags, int32_t metaState, int32_t buttonState,
            int32_t edgeFlags, uint32_t pointerCount,
            const PointerProperties* pointerProperties, const PointerCoords* pointerCoords,
            float xPrecision, float yPrecision, nsecs_t downTime);

    NotifyMotionArgs(const NotifyMotionArgs& other);

    virtual ~NotifyMotionArgs() { }

    virtual void notify(const sp<InputListenerInterface>& listener) const;
};



struct NotifySwitchArgs : public NotifyArgs {
    nsecs_t eventTime;
    uint32_t policyFlags;
    int32_t switchCode;
    int32_t switchValue;

    inline NotifySwitchArgs() { }

    NotifySwitchArgs(nsecs_t eventTime, uint32_t policyFlags,
            int32_t switchCode, int32_t switchValue);

    NotifySwitchArgs(const NotifySwitchArgs& other);

    virtual ~NotifySwitchArgs() { }

    virtual void notify(const sp<InputListenerInterface>& listener) const;
};




struct NotifyDeviceResetArgs : public NotifyArgs {
    nsecs_t eventTime;
    int32_t deviceId;

    inline NotifyDeviceResetArgs() { }

    NotifyDeviceResetArgs(nsecs_t eventTime, int32_t deviceId);

    NotifyDeviceResetArgs(const NotifyDeviceResetArgs& other);

    virtual ~NotifyDeviceResetArgs() { }

    virtual void notify(const sp<InputListenerInterface>& listener) const;
};





class InputListenerInterface : public virtual RefBase {
protected:
    InputListenerInterface() { }
    virtual ~InputListenerInterface() { }

public:
    virtual void notifyConfigurationChanged(const NotifyConfigurationChangedArgs* args) = 0;
    virtual void notifyKey(const NotifyKeyArgs* args) = 0;
    virtual void notifyMotion(const NotifyMotionArgs* args) = 0;
    virtual void notifySwitch(const NotifySwitchArgs* args) = 0;
    virtual void notifyDeviceReset(const NotifyDeviceResetArgs* args) = 0;
};






class QueuedInputListener : public InputListenerInterface {
protected:
    virtual ~QueuedInputListener();

public:
    QueuedInputListener(const sp<InputListenerInterface>& innerListener);

    virtual void notifyConfigurationChanged(const NotifyConfigurationChangedArgs* args);
    virtual void notifyKey(const NotifyKeyArgs* args);
    virtual void notifyMotion(const NotifyMotionArgs* args);
    virtual void notifySwitch(const NotifySwitchArgs* args);
    virtual void notifyDeviceReset(const NotifyDeviceResetArgs* args);

    void flush();

private:
    sp<InputListenerInterface> mInnerListener;
    Vector<NotifyArgs*> mArgsQueue;
};

} 

#endif 
