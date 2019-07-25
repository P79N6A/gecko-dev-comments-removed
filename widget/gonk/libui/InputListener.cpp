















#define LOG_TAG "InputListener"



#include "InputListener.h"

#include "cutils_log.h"

namespace android {



NotifyConfigurationChangedArgs::NotifyConfigurationChangedArgs(nsecs_t eventTime) :
        eventTime(eventTime) {
}

NotifyConfigurationChangedArgs::NotifyConfigurationChangedArgs(
        const NotifyConfigurationChangedArgs& other) :
        eventTime(other.eventTime) {
}

void NotifyConfigurationChangedArgs::notify(const sp<InputListenerInterface>& listener) const {
    listener->notifyConfigurationChanged(this);
}




NotifyKeyArgs::NotifyKeyArgs(nsecs_t eventTime, int32_t deviceId, uint32_t source,
        uint32_t policyFlags,
        int32_t action, int32_t flags, int32_t keyCode, int32_t scanCode,
        int32_t metaState, nsecs_t downTime) :
        eventTime(eventTime), deviceId(deviceId), source(source), policyFlags(policyFlags),
        action(action), flags(flags), keyCode(keyCode), scanCode(scanCode),
        metaState(metaState), downTime(downTime) {
}

NotifyKeyArgs::NotifyKeyArgs(const NotifyKeyArgs& other) :
        eventTime(other.eventTime), deviceId(other.deviceId), source(other.source),
        policyFlags(other.policyFlags),
        action(other.action), flags(other.flags),
        keyCode(other.keyCode), scanCode(other.scanCode),
        metaState(other.metaState), downTime(other.downTime) {
}

void NotifyKeyArgs::notify(const sp<InputListenerInterface>& listener) const {
    listener->notifyKey(this);
}




NotifyMotionArgs::NotifyMotionArgs(nsecs_t eventTime, int32_t deviceId, uint32_t source,
        uint32_t policyFlags,
        int32_t action, int32_t flags, int32_t metaState, int32_t buttonState,
        int32_t edgeFlags, uint32_t pointerCount,
        const PointerProperties* pointerProperties, const PointerCoords* pointerCoords,
        float xPrecision, float yPrecision, nsecs_t downTime) :
        eventTime(eventTime), deviceId(deviceId), source(source), policyFlags(policyFlags),
        action(action), flags(flags), metaState(metaState), buttonState(buttonState),
        edgeFlags(edgeFlags), pointerCount(pointerCount),
        xPrecision(xPrecision), yPrecision(yPrecision), downTime(downTime) {
    for (uint32_t i = 0; i < pointerCount; i++) {
        this->pointerProperties[i].copyFrom(pointerProperties[i]);
        this->pointerCoords[i].copyFrom(pointerCoords[i]);
    }
}

NotifyMotionArgs::NotifyMotionArgs(const NotifyMotionArgs& other) :
        eventTime(other.eventTime), deviceId(other.deviceId), source(other.source),
        policyFlags(other.policyFlags),
        action(other.action), flags(other.flags),
        metaState(other.metaState), buttonState(other.buttonState),
        edgeFlags(other.edgeFlags), pointerCount(other.pointerCount),
        xPrecision(other.xPrecision), yPrecision(other.yPrecision), downTime(other.downTime) {
    for (uint32_t i = 0; i < pointerCount; i++) {
        pointerProperties[i].copyFrom(other.pointerProperties[i]);
        pointerCoords[i].copyFrom(other.pointerCoords[i]);
    }
}

void NotifyMotionArgs::notify(const sp<InputListenerInterface>& listener) const {
    listener->notifyMotion(this);
}




NotifySwitchArgs::NotifySwitchArgs(nsecs_t eventTime, uint32_t policyFlags,
        int32_t switchCode, int32_t switchValue) :
        eventTime(eventTime), policyFlags(policyFlags),
        switchCode(switchCode), switchValue(switchValue) {
}

NotifySwitchArgs::NotifySwitchArgs(const NotifySwitchArgs& other) :
        eventTime(other.eventTime), policyFlags(other.policyFlags),
        switchCode(other.switchCode), switchValue(other.switchValue) {
}

void NotifySwitchArgs::notify(const sp<InputListenerInterface>& listener) const {
    listener->notifySwitch(this);
}




NotifyDeviceResetArgs::NotifyDeviceResetArgs(nsecs_t eventTime, int32_t deviceId) :
        eventTime(eventTime), deviceId(deviceId) {
}

NotifyDeviceResetArgs::NotifyDeviceResetArgs(const NotifyDeviceResetArgs& other) :
        eventTime(other.eventTime), deviceId(other.deviceId) {
}

void NotifyDeviceResetArgs::notify(const sp<InputListenerInterface>& listener) const {
    listener->notifyDeviceReset(this);
}




QueuedInputListener::QueuedInputListener(const sp<InputListenerInterface>& innerListener) :
        mInnerListener(innerListener) {
}

QueuedInputListener::~QueuedInputListener() {
    size_t count = mArgsQueue.size();
    for (size_t i = 0; i < count; i++) {
        delete mArgsQueue[i];
    }
}

void QueuedInputListener::notifyConfigurationChanged(
        const NotifyConfigurationChangedArgs* args) {
    mArgsQueue.push(new NotifyConfigurationChangedArgs(*args));
}

void QueuedInputListener::notifyKey(const NotifyKeyArgs* args) {
    mArgsQueue.push(new NotifyKeyArgs(*args));
}

void QueuedInputListener::notifyMotion(const NotifyMotionArgs* args) {
    mArgsQueue.push(new NotifyMotionArgs(*args));
}

void QueuedInputListener::notifySwitch(const NotifySwitchArgs* args) {
    mArgsQueue.push(new NotifySwitchArgs(*args));
}

void QueuedInputListener::notifyDeviceReset(const NotifyDeviceResetArgs* args) {
    mArgsQueue.push(new NotifyDeviceResetArgs(*args));
}

void QueuedInputListener::flush() {
    size_t count = mArgsQueue.size();
    for (size_t i = 0; i < count; i++) {
        NotifyArgs* args = mArgsQueue[i];
        args->notify(mInnerListener);
        delete args;
    }
    mArgsQueue.clear();
}


} 
