















#define LOG_TAG "InputDispatcher"




#define DEBUG_INBOUND_EVENT_DETAILS 0


#define DEBUG_OUTBOUND_EVENT_DETAILS 0


#define DEBUG_BATCHING 0


#define DEBUG_DISPATCH_CYCLE 0


#define DEBUG_REGISTRATION 0


#define DEBUG_PERFORMANCE_STATISTICS 0


#define DEBUG_INJECTION 0


#define DEBUG_THROTTLING 0


#define DEBUG_FOCUS 0


#define DEBUG_APP_SWITCH 0


#define DEBUG_HOVER 0

#include "InputDispatcher.h"

#include <ui/PowerManager.h>

#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#define INDENT "  "
#define INDENT2 "    "

namespace android {



const nsecs_t DEFAULT_INPUT_DISPATCHING_TIMEOUT = 5000 * 1000000LL; 




const nsecs_t APP_SWITCH_TIMEOUT = 500 * 1000000LL; 



const nsecs_t STALE_EVENT_TIMEOUT = 10000 * 1000000LL; 















const nsecs_t MOTION_SAMPLE_COALESCE_INTERVAL = 3 * 1000000LL; 


static inline nsecs_t now() {
    return systemTime(SYSTEM_TIME_MONOTONIC);
}

static inline const char* toString(bool value) {
    return value ? "true" : "false";
}

static inline int32_t getMotionEventActionPointerIndex(int32_t action) {
    return (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
            >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
}

static bool isValidKeyAction(int32_t action) {
    switch (action) {
    case AKEY_EVENT_ACTION_DOWN:
    case AKEY_EVENT_ACTION_UP:
        return true;
    default:
        return false;
    }
}

static bool validateKeyEvent(int32_t action) {
    if (! isValidKeyAction(action)) {
        ALOGE("Key event has invalid action code 0x%x", action);
        return false;
    }
    return true;
}

static bool isValidMotionAction(int32_t action, size_t pointerCount) {
    switch (action & AMOTION_EVENT_ACTION_MASK) {
    case AMOTION_EVENT_ACTION_DOWN:
    case AMOTION_EVENT_ACTION_UP:
    case AMOTION_EVENT_ACTION_CANCEL:
    case AMOTION_EVENT_ACTION_MOVE:
    case AMOTION_EVENT_ACTION_OUTSIDE:
    case AMOTION_EVENT_ACTION_HOVER_ENTER:
    case AMOTION_EVENT_ACTION_HOVER_MOVE:
    case AMOTION_EVENT_ACTION_HOVER_EXIT:
    case AMOTION_EVENT_ACTION_SCROLL:
        return true;
    case AMOTION_EVENT_ACTION_POINTER_DOWN:
    case AMOTION_EVENT_ACTION_POINTER_UP: {
        int32_t index = getMotionEventActionPointerIndex(action);
        return index >= 0 && size_t(index) < pointerCount;
    }
    default:
        return false;
    }
}

static bool validateMotionEvent(int32_t action, size_t pointerCount,
        const PointerProperties* pointerProperties) {
    if (! isValidMotionAction(action, pointerCount)) {
        ALOGE("Motion event has invalid action code 0x%x", action);
        return false;
    }
    if (pointerCount < 1 || pointerCount > MAX_POINTERS) {
        ALOGE("Motion event has invalid pointer count %d; value must be between 1 and %d.",
                pointerCount, MAX_POINTERS);
        return false;
    }
    BitSet32 pointerIdBits;
    for (size_t i = 0; i < pointerCount; i++) {
        int32_t id = pointerProperties[i].id;
        if (id < 0 || id > MAX_POINTER_ID) {
            ALOGE("Motion event has invalid pointer id %d; value must be between 0 and %d",
                    id, MAX_POINTER_ID);
            return false;
        }
        if (pointerIdBits.hasBit(id)) {
            ALOGE("Motion event has duplicate pointer id %d", id);
            return false;
        }
        pointerIdBits.markBit(id);
    }
    return true;
}

static void scalePointerCoords(const PointerCoords* inCoords, size_t count, float scaleFactor,
        PointerCoords* outCoords) {
   for (size_t i = 0; i < count; i++) {
       outCoords[i] = inCoords[i];
       outCoords[i].scale(scaleFactor);
   }
}

static void dumpRegion(String8& dump, const SkRegion& region) {
    if (region.isEmpty()) {
        dump.append("<empty>");
        return;
    }

#ifdef HAVE_ANDROID_OS
    bool first = true;
    for (SkRegion::Iterator it(region); !it.done(); it.next()) {
        if (first) {
            first = false;
        } else {
            dump.append("|");
        }
        const SkIRect& rect = it.rect();
        dump.appendFormat("[%d,%d][%d,%d]", rect.fLeft, rect.fTop, rect.fRight, rect.fBottom);
    }
#endif
}




InputDispatcher::InputDispatcher(const sp<InputDispatcherPolicyInterface>& policy) :
    mPolicy(policy),
    mPendingEvent(NULL), mAppSwitchSawKeyDown(false), mAppSwitchDueTime(LONG_LONG_MAX),
    mNextUnblockedEvent(NULL),
    mDispatchEnabled(true), mDispatchFrozen(false), mInputFilterEnabled(false),
    mCurrentInputTargetsValid(false),
    mInputTargetWaitCause(INPUT_TARGET_WAIT_CAUSE_NONE) {
    mLooper = new Looper(false);

    mKeyRepeatState.lastKeyEntry = NULL;

    policy->getDispatcherConfiguration(&mConfig);

    mThrottleState.minTimeBetweenEvents = 1000000000LL / mConfig.maxEventsPerSecond;
    mThrottleState.lastDeviceId = -1;

#if DEBUG_THROTTLING
    mThrottleState.originalSampleCount = 0;
    ALOGD("Throttling - Max events per second = %d", mConfig.maxEventsPerSecond);
#endif
}

InputDispatcher::~InputDispatcher() {
    { 
        AutoMutex _l(mLock);

        resetKeyRepeatLocked();
        releasePendingEventLocked();
        drainInboundQueueLocked();
    }

    while (mConnectionsByReceiveFd.size() != 0) {
        unregisterInputChannel(mConnectionsByReceiveFd.valueAt(0)->inputChannel);
    }
}

void InputDispatcher::dispatchOnce() {
    nsecs_t nextWakeupTime = LONG_LONG_MAX;
    { 
        AutoMutex _l(mLock);
        dispatchOnceInnerLocked(&nextWakeupTime);

        if (runCommandsLockedInterruptible()) {
            nextWakeupTime = LONG_LONG_MIN;  
        }
    } 

    
    nsecs_t currentTime = now();
    int timeoutMillis = toMillisecondTimeoutDelay(currentTime, nextWakeupTime);
    mLooper->pollOnce(timeoutMillis);
}

void InputDispatcher::dispatchOnceInnerLocked(nsecs_t* nextWakeupTime) {
    nsecs_t currentTime = now();

    
    
    
    if (!mPolicy->isKeyRepeatEnabled()) {
        resetKeyRepeatLocked();
    }

    
    if (mDispatchFrozen) {
#if DEBUG_FOCUS
        ALOGD("Dispatch frozen.  Waiting some more.");
#endif
        return;
    }

    
    
    
    bool isAppSwitchDue = mAppSwitchDueTime <= currentTime;
    if (mAppSwitchDueTime < *nextWakeupTime) {
        *nextWakeupTime = mAppSwitchDueTime;
    }

    
    
    if (! mPendingEvent) {
        if (mInboundQueue.isEmpty()) {
            if (isAppSwitchDue) {
                
                
                resetPendingAppSwitchLocked(false);
                isAppSwitchDue = false;
            }

            
            if (mKeyRepeatState.lastKeyEntry) {
                if (currentTime >= mKeyRepeatState.nextRepeatTime) {
                    mPendingEvent = synthesizeKeyRepeatLocked(currentTime);
                } else {
                    if (mKeyRepeatState.nextRepeatTime < *nextWakeupTime) {
                        *nextWakeupTime = mKeyRepeatState.nextRepeatTime;
                    }
                }
            }

            
            if (! mPendingEvent) {
                if (mActiveConnections.isEmpty()) {
                    dispatchIdleLocked();
                }
                return;
            }
        } else {
            
            EventEntry* entry = mInboundQueue.head;

            
            
            
            
            
            if (entry->type == EventEntry::TYPE_MOTION
                    && !isAppSwitchDue
                    && mDispatchEnabled
                    && (entry->policyFlags & POLICY_FLAG_PASS_TO_USER)
                    && !entry->isInjected()) {
                MotionEntry* motionEntry = static_cast<MotionEntry*>(entry);
                int32_t deviceId = motionEntry->deviceId;
                uint32_t source = motionEntry->source;
                if (! isAppSwitchDue
                        && !motionEntry->next 
                        && (motionEntry->action == AMOTION_EVENT_ACTION_MOVE
                                || motionEntry->action == AMOTION_EVENT_ACTION_HOVER_MOVE)
                        && deviceId == mThrottleState.lastDeviceId
                        && source == mThrottleState.lastSource) {
                    nsecs_t nextTime = mThrottleState.lastEventTime
                            + mThrottleState.minTimeBetweenEvents;
                    if (currentTime < nextTime) {
                        
#if DEBUG_THROTTLING
                        ALOGD("Throttling - Delaying motion event for "
                                "device %d, source 0x%08x by up to %0.3fms.",
                                deviceId, source, (nextTime - currentTime) * 0.000001);
#endif
                        if (nextTime < *nextWakeupTime) {
                            *nextWakeupTime = nextTime;
                        }
                        if (mThrottleState.originalSampleCount == 0) {
                            mThrottleState.originalSampleCount =
                                    motionEntry->countSamples();
                        }
                        return;
                    }
                }

#if DEBUG_THROTTLING
                if (mThrottleState.originalSampleCount != 0) {
                    uint32_t count = motionEntry->countSamples();
                    ALOGD("Throttling - Motion event sample count grew by %d from %d to %d.",
                            count - mThrottleState.originalSampleCount,
                            mThrottleState.originalSampleCount, count);
                    mThrottleState.originalSampleCount = 0;
                }
#endif

                mThrottleState.lastEventTime = currentTime;
                mThrottleState.lastDeviceId = deviceId;
                mThrottleState.lastSource = source;
            }

            mInboundQueue.dequeue(entry);
            mPendingEvent = entry;
        }

        
        if (mPendingEvent->policyFlags & POLICY_FLAG_PASS_TO_USER) {
            pokeUserActivityLocked(mPendingEvent);
        }
    }

    
    
    ALOG_ASSERT(mPendingEvent != NULL);
    bool done = false;
    DropReason dropReason = DROP_REASON_NOT_DROPPED;
    if (!(mPendingEvent->policyFlags & POLICY_FLAG_PASS_TO_USER)) {
        dropReason = DROP_REASON_POLICY;
    } else if (!mDispatchEnabled) {
        dropReason = DROP_REASON_DISABLED;
    }

    if (mNextUnblockedEvent == mPendingEvent) {
        mNextUnblockedEvent = NULL;
    }

    switch (mPendingEvent->type) {
    case EventEntry::TYPE_CONFIGURATION_CHANGED: {
        ConfigurationChangedEntry* typedEntry =
                static_cast<ConfigurationChangedEntry*>(mPendingEvent);
        done = dispatchConfigurationChangedLocked(currentTime, typedEntry);
        dropReason = DROP_REASON_NOT_DROPPED; 
        break;
    }

    case EventEntry::TYPE_DEVICE_RESET: {
        DeviceResetEntry* typedEntry =
                static_cast<DeviceResetEntry*>(mPendingEvent);
        done = dispatchDeviceResetLocked(currentTime, typedEntry);
        dropReason = DROP_REASON_NOT_DROPPED; 
        break;
    }

    case EventEntry::TYPE_KEY: {
        KeyEntry* typedEntry = static_cast<KeyEntry*>(mPendingEvent);
        if (isAppSwitchDue) {
            if (isAppSwitchKeyEventLocked(typedEntry)) {
                resetPendingAppSwitchLocked(true);
                isAppSwitchDue = false;
            } else if (dropReason == DROP_REASON_NOT_DROPPED) {
                dropReason = DROP_REASON_APP_SWITCH;
            }
        }
        if (dropReason == DROP_REASON_NOT_DROPPED
                && isStaleEventLocked(currentTime, typedEntry)) {
            dropReason = DROP_REASON_STALE;
        }
        if (dropReason == DROP_REASON_NOT_DROPPED && mNextUnblockedEvent) {
            dropReason = DROP_REASON_BLOCKED;
        }
        done = dispatchKeyLocked(currentTime, typedEntry, &dropReason, nextWakeupTime);
        break;
    }

    case EventEntry::TYPE_MOTION: {
        MotionEntry* typedEntry = static_cast<MotionEntry*>(mPendingEvent);
        if (dropReason == DROP_REASON_NOT_DROPPED && isAppSwitchDue) {
            dropReason = DROP_REASON_APP_SWITCH;
        }
        if (dropReason == DROP_REASON_NOT_DROPPED
                && isStaleEventLocked(currentTime, typedEntry)) {
            dropReason = DROP_REASON_STALE;
        }
        if (dropReason == DROP_REASON_NOT_DROPPED && mNextUnblockedEvent) {
            dropReason = DROP_REASON_BLOCKED;
        }
        done = dispatchMotionLocked(currentTime, typedEntry,
                &dropReason, nextWakeupTime);
        break;
    }

    default:
        ALOG_ASSERT(false);
        break;
    }

    if (done) {
        if (dropReason != DROP_REASON_NOT_DROPPED) {
            dropInboundEventLocked(mPendingEvent, dropReason);
        }

        releasePendingEventLocked();
        *nextWakeupTime = LONG_LONG_MIN;  
    }
}

void InputDispatcher::dispatchIdleLocked() {
#if DEBUG_FOCUS
    ALOGD("Dispatcher idle.  There are no pending events or active connections.");
#endif

    
    
    resetTargetsLocked();
}

bool InputDispatcher::enqueueInboundEventLocked(EventEntry* entry) {
    bool needWake = mInboundQueue.isEmpty();
    mInboundQueue.enqueueAtTail(entry);

    switch (entry->type) {
    case EventEntry::TYPE_KEY: {
        
        
        
        KeyEntry* keyEntry = static_cast<KeyEntry*>(entry);
        if (isAppSwitchKeyEventLocked(keyEntry)) {
            if (keyEntry->action == AKEY_EVENT_ACTION_DOWN) {
                mAppSwitchSawKeyDown = true;
            } else if (keyEntry->action == AKEY_EVENT_ACTION_UP) {
                if (mAppSwitchSawKeyDown) {
#if DEBUG_APP_SWITCH
                    ALOGD("App switch is pending!");
#endif
                    mAppSwitchDueTime = keyEntry->eventTime + APP_SWITCH_TIMEOUT;
                    mAppSwitchSawKeyDown = false;
                    needWake = true;
                }
            }
        }
        break;
    }

    case EventEntry::TYPE_MOTION: {
        
        
        
        
        MotionEntry* motionEntry = static_cast<MotionEntry*>(entry);
        if (motionEntry->action == AMOTION_EVENT_ACTION_DOWN
                && (motionEntry->source & AINPUT_SOURCE_CLASS_POINTER)
                && mInputTargetWaitCause == INPUT_TARGET_WAIT_CAUSE_APPLICATION_NOT_READY
                && mInputTargetWaitApplicationHandle != NULL) {
            int32_t x = int32_t(motionEntry->firstSample.pointerCoords[0].
                    getAxisValue(AMOTION_EVENT_AXIS_X));
            int32_t y = int32_t(motionEntry->firstSample.pointerCoords[0].
                    getAxisValue(AMOTION_EVENT_AXIS_Y));
            sp<InputWindowHandle> touchedWindowHandle = findTouchedWindowAtLocked(x, y);
            if (touchedWindowHandle != NULL
                    && touchedWindowHandle->inputApplicationHandle
                            != mInputTargetWaitApplicationHandle) {
                
                
                mNextUnblockedEvent = motionEntry;
                needWake = true;
            }
        }
        break;
    }
    }

    return needWake;
}

sp<InputWindowHandle> InputDispatcher::findTouchedWindowAtLocked(int32_t x, int32_t y) {
    
    size_t numWindows = mWindowHandles.size();
    for (size_t i = 0; i < numWindows; i++) {
        sp<InputWindowHandle> windowHandle = mWindowHandles.itemAt(i);
        const InputWindowInfo* windowInfo = windowHandle->getInfo();
        int32_t flags = windowInfo->layoutParamsFlags;

        if (windowInfo->visible) {
            if (!(flags & InputWindowInfo::FLAG_NOT_TOUCHABLE)) {
                bool isTouchModal = (flags & (InputWindowInfo::FLAG_NOT_FOCUSABLE
                        | InputWindowInfo::FLAG_NOT_TOUCH_MODAL)) == 0;
                if (isTouchModal || windowInfo->touchableRegionContainsPoint(x, y)) {
                    
                    return windowHandle;
                }
            }
        }

        if (flags & InputWindowInfo::FLAG_SYSTEM_ERROR) {
            
            return NULL;
        }
    }
    return NULL;
}

void InputDispatcher::dropInboundEventLocked(EventEntry* entry, DropReason dropReason) {
    const char* reason;
    switch (dropReason) {
    case DROP_REASON_POLICY:
#if DEBUG_INBOUND_EVENT_DETAILS
        ALOGD("Dropped event because policy consumed it.");
#endif
        reason = "inbound event was dropped because the policy consumed it";
        break;
    case DROP_REASON_DISABLED:
        ALOGI("Dropped event because input dispatch is disabled.");
        reason = "inbound event was dropped because input dispatch is disabled";
        break;
    case DROP_REASON_APP_SWITCH:
        ALOGI("Dropped event because of pending overdue app switch.");
        reason = "inbound event was dropped because of pending overdue app switch";
        break;
    case DROP_REASON_BLOCKED:
        ALOGI("Dropped event because the current application is not responding and the user "
                "has started interacting with a different application.");
        reason = "inbound event was dropped because the current application is not responding "
                "and the user has started interacting with a different application";
        break;
    case DROP_REASON_STALE:
        ALOGI("Dropped event because it is stale.");
        reason = "inbound event was dropped because it is stale";
        break;
    default:
        ALOG_ASSERT(false);
        return;
    }

    switch (entry->type) {
    case EventEntry::TYPE_KEY: {
        CancelationOptions options(CancelationOptions::CANCEL_NON_POINTER_EVENTS, reason);
        synthesizeCancelationEventsForAllConnectionsLocked(options);
        break;
    }
    case EventEntry::TYPE_MOTION: {
        MotionEntry* motionEntry = static_cast<MotionEntry*>(entry);
        if (motionEntry->source & AINPUT_SOURCE_CLASS_POINTER) {
            CancelationOptions options(CancelationOptions::CANCEL_POINTER_EVENTS, reason);
            synthesizeCancelationEventsForAllConnectionsLocked(options);
        } else {
            CancelationOptions options(CancelationOptions::CANCEL_NON_POINTER_EVENTS, reason);
            synthesizeCancelationEventsForAllConnectionsLocked(options);
        }
        break;
    }
    }
}

bool InputDispatcher::isAppSwitchKeyCode(int32_t keyCode) {
    return keyCode == AKEYCODE_HOME || keyCode == AKEYCODE_ENDCALL;
}

bool InputDispatcher::isAppSwitchKeyEventLocked(KeyEntry* keyEntry) {
    return ! (keyEntry->flags & AKEY_EVENT_FLAG_CANCELED)
            && isAppSwitchKeyCode(keyEntry->keyCode)
            && (keyEntry->policyFlags & POLICY_FLAG_TRUSTED)
            && (keyEntry->policyFlags & POLICY_FLAG_PASS_TO_USER);
}

bool InputDispatcher::isAppSwitchPendingLocked() {
    return mAppSwitchDueTime != LONG_LONG_MAX;
}

void InputDispatcher::resetPendingAppSwitchLocked(bool handled) {
    mAppSwitchDueTime = LONG_LONG_MAX;

#if DEBUG_APP_SWITCH
    if (handled) {
        ALOGD("App switch has arrived.");
    } else {
        ALOGD("App switch was abandoned.");
    }
#endif
}

bool InputDispatcher::isStaleEventLocked(nsecs_t currentTime, EventEntry* entry) {
    return currentTime - entry->eventTime >= STALE_EVENT_TIMEOUT;
}

bool InputDispatcher::runCommandsLockedInterruptible() {
    if (mCommandQueue.isEmpty()) {
        return false;
    }

    do {
        CommandEntry* commandEntry = mCommandQueue.dequeueAtHead();

        Command command = commandEntry->command;
        (this->*command)(commandEntry); 

        commandEntry->connection.clear();
        delete commandEntry;
    } while (! mCommandQueue.isEmpty());
    return true;
}

InputDispatcher::CommandEntry* InputDispatcher::postCommandLocked(Command command) {
    CommandEntry* commandEntry = new CommandEntry(command);
    mCommandQueue.enqueueAtTail(commandEntry);
    return commandEntry;
}

void InputDispatcher::drainInboundQueueLocked() {
    while (! mInboundQueue.isEmpty()) {
        EventEntry* entry = mInboundQueue.dequeueAtHead();
        releaseInboundEventLocked(entry);
    }
}

void InputDispatcher::releasePendingEventLocked() {
    if (mPendingEvent) {
        releaseInboundEventLocked(mPendingEvent);
        mPendingEvent = NULL;
    }
}

void InputDispatcher::releaseInboundEventLocked(EventEntry* entry) {
    InjectionState* injectionState = entry->injectionState;
    if (injectionState && injectionState->injectionResult == INPUT_EVENT_INJECTION_PENDING) {
#if DEBUG_DISPATCH_CYCLE
        ALOGD("Injected inbound event was dropped.");
#endif
        setInjectionResultLocked(entry, INPUT_EVENT_INJECTION_FAILED);
    }
    if (entry == mNextUnblockedEvent) {
        mNextUnblockedEvent = NULL;
    }
    entry->release();
}

void InputDispatcher::resetKeyRepeatLocked() {
    if (mKeyRepeatState.lastKeyEntry) {
        mKeyRepeatState.lastKeyEntry->release();
        mKeyRepeatState.lastKeyEntry = NULL;
    }
}

InputDispatcher::KeyEntry* InputDispatcher::synthesizeKeyRepeatLocked(nsecs_t currentTime) {
    KeyEntry* entry = mKeyRepeatState.lastKeyEntry;

    
    uint32_t policyFlags = (entry->policyFlags & POLICY_FLAG_RAW_MASK)
            | POLICY_FLAG_PASS_TO_USER | POLICY_FLAG_TRUSTED;
    if (entry->refCount == 1) {
        entry->recycle();
        entry->eventTime = currentTime;
        entry->policyFlags = policyFlags;
        entry->repeatCount += 1;
    } else {
        KeyEntry* newEntry = new KeyEntry(currentTime,
                entry->deviceId, entry->source, policyFlags,
                entry->action, entry->flags, entry->keyCode, entry->scanCode,
                entry->metaState, entry->repeatCount + 1, entry->downTime);

        mKeyRepeatState.lastKeyEntry = newEntry;
        entry->release();

        entry = newEntry;
    }
    entry->syntheticRepeat = true;

    
    
    entry->refCount += 1;

    mKeyRepeatState.nextRepeatTime = currentTime + mConfig.keyRepeatDelay;
    return entry;
}

bool InputDispatcher::dispatchConfigurationChangedLocked(
        nsecs_t currentTime, ConfigurationChangedEntry* entry) {
#if DEBUG_OUTBOUND_EVENT_DETAILS
    ALOGD("dispatchConfigurationChanged - eventTime=%lld", entry->eventTime);
#endif

    
    resetKeyRepeatLocked();

    
    CommandEntry* commandEntry = postCommandLocked(
            & InputDispatcher::doNotifyConfigurationChangedInterruptible);
    commandEntry->eventTime = entry->eventTime;
    return true;
}

bool InputDispatcher::dispatchDeviceResetLocked(
        nsecs_t currentTime, DeviceResetEntry* entry) {
#if DEBUG_OUTBOUND_EVENT_DETAILS
    ALOGD("dispatchDeviceReset - eventTime=%lld, deviceId=%d", entry->eventTime, entry->deviceId);
#endif

    CancelationOptions options(CancelationOptions::CANCEL_ALL_EVENTS,
            "device was reset");
    options.deviceId = entry->deviceId;
    synthesizeCancelationEventsForAllConnectionsLocked(options);
    return true;
}

bool InputDispatcher::dispatchKeyLocked(nsecs_t currentTime, KeyEntry* entry,
        DropReason* dropReason, nsecs_t* nextWakeupTime) {
    
    if (! entry->dispatchInProgress) {
        if (entry->repeatCount == 0
                && entry->action == AKEY_EVENT_ACTION_DOWN
                && (entry->policyFlags & POLICY_FLAG_TRUSTED)
                && (!(entry->policyFlags & POLICY_FLAG_DISABLE_KEY_REPEAT))) {
            if (mKeyRepeatState.lastKeyEntry
                    && mKeyRepeatState.lastKeyEntry->keyCode == entry->keyCode) {
                
                
                
                
                entry->repeatCount = mKeyRepeatState.lastKeyEntry->repeatCount + 1;
                resetKeyRepeatLocked();
                mKeyRepeatState.nextRepeatTime = LONG_LONG_MAX; 
            } else {
                
                resetKeyRepeatLocked();
                mKeyRepeatState.nextRepeatTime = entry->eventTime + mConfig.keyRepeatTimeout;
            }
            mKeyRepeatState.lastKeyEntry = entry;
            entry->refCount += 1;
        } else if (! entry->syntheticRepeat) {
            resetKeyRepeatLocked();
        }

        if (entry->repeatCount == 1) {
            entry->flags |= AKEY_EVENT_FLAG_LONG_PRESS;
        } else {
            entry->flags &= ~AKEY_EVENT_FLAG_LONG_PRESS;
        }

        entry->dispatchInProgress = true;
        resetTargetsLocked();

        logOutboundKeyDetailsLocked("dispatchKey - ", entry);
    }

    
    if (entry->interceptKeyResult == KeyEntry::INTERCEPT_KEY_RESULT_TRY_AGAIN_LATER) {
        if (currentTime < entry->interceptKeyWakeupTime) {
            if (entry->interceptKeyWakeupTime < *nextWakeupTime) {
                *nextWakeupTime = entry->interceptKeyWakeupTime;
            }
            return false; 
        }
        entry->interceptKeyResult = KeyEntry::INTERCEPT_KEY_RESULT_UNKNOWN;
        entry->interceptKeyWakeupTime = 0;
    }

    
    if (entry->interceptKeyResult == KeyEntry::INTERCEPT_KEY_RESULT_UNKNOWN) {
        if (entry->policyFlags & POLICY_FLAG_PASS_TO_USER) {
            CommandEntry* commandEntry = postCommandLocked(
                    & InputDispatcher::doInterceptKeyBeforeDispatchingLockedInterruptible);
            if (mFocusedWindowHandle != NULL) {
                commandEntry->inputWindowHandle = mFocusedWindowHandle;
            }
            commandEntry->keyEntry = entry;
            entry->refCount += 1;
            return false; 
        } else {
            entry->interceptKeyResult = KeyEntry::INTERCEPT_KEY_RESULT_CONTINUE;
        }
    } else if (entry->interceptKeyResult == KeyEntry::INTERCEPT_KEY_RESULT_SKIP) {
        if (*dropReason == DROP_REASON_NOT_DROPPED) {
            *dropReason = DROP_REASON_POLICY;
        }
    }

    
    if (*dropReason != DROP_REASON_NOT_DROPPED) {
        resetTargetsLocked();
        setInjectionResultLocked(entry, *dropReason == DROP_REASON_POLICY
                ? INPUT_EVENT_INJECTION_SUCCEEDED : INPUT_EVENT_INJECTION_FAILED);
        return true;
    }

    
    if (! mCurrentInputTargetsValid) {
        int32_t injectionResult = findFocusedWindowTargetsLocked(currentTime,
                entry, nextWakeupTime);
        if (injectionResult == INPUT_EVENT_INJECTION_PENDING) {
            return false;
        }

        setInjectionResultLocked(entry, injectionResult);
        if (injectionResult != INPUT_EVENT_INJECTION_SUCCEEDED) {
            return true;
        }

        addMonitoringTargetsLocked();
        commitTargetsLocked();
    }

    
    dispatchEventToCurrentInputTargetsLocked(currentTime, entry, false);
    return true;
}

void InputDispatcher::logOutboundKeyDetailsLocked(const char* prefix, const KeyEntry* entry) {
#if DEBUG_OUTBOUND_EVENT_DETAILS
    ALOGD("%seventTime=%lld, deviceId=%d, source=0x%x, policyFlags=0x%x, "
            "action=0x%x, flags=0x%x, keyCode=0x%x, scanCode=0x%x, metaState=0x%x, "
            "repeatCount=%d, downTime=%lld",
            prefix,
            entry->eventTime, entry->deviceId, entry->source, entry->policyFlags,
            entry->action, entry->flags, entry->keyCode, entry->scanCode, entry->metaState,
            entry->repeatCount, entry->downTime);
#endif
}

bool InputDispatcher::dispatchMotionLocked(
        nsecs_t currentTime, MotionEntry* entry, DropReason* dropReason, nsecs_t* nextWakeupTime) {
    
    if (! entry->dispatchInProgress) {
        entry->dispatchInProgress = true;
        resetTargetsLocked();

        logOutboundMotionDetailsLocked("dispatchMotion - ", entry);
    }

    
    if (*dropReason != DROP_REASON_NOT_DROPPED) {
        resetTargetsLocked();
        setInjectionResultLocked(entry, *dropReason == DROP_REASON_POLICY
                ? INPUT_EVENT_INJECTION_SUCCEEDED : INPUT_EVENT_INJECTION_FAILED);
        return true;
    }

    bool isPointerEvent = entry->source & AINPUT_SOURCE_CLASS_POINTER;

    
    bool conflictingPointerActions = false;
    if (! mCurrentInputTargetsValid) {
        int32_t injectionResult;
        const MotionSample* splitBatchAfterSample = NULL;
        if (isPointerEvent) {
            
            injectionResult = findTouchedWindowTargetsLocked(currentTime,
                    entry, nextWakeupTime, &conflictingPointerActions, &splitBatchAfterSample);
        } else {
            
            injectionResult = findFocusedWindowTargetsLocked(currentTime,
                    entry, nextWakeupTime);
        }
        if (injectionResult == INPUT_EVENT_INJECTION_PENDING) {
            return false;
        }

        setInjectionResultLocked(entry, injectionResult);
        if (injectionResult != INPUT_EVENT_INJECTION_SUCCEEDED) {
            return true;
        }

        addMonitoringTargetsLocked();
        commitTargetsLocked();

        
        
        if (splitBatchAfterSample && splitBatchAfterSample->next) {
#if DEBUG_BATCHING
            uint32_t originalSampleCount = entry->countSamples();
#endif
            MotionSample* nextSample = splitBatchAfterSample->next;
            MotionEntry* nextEntry = new MotionEntry(nextSample->eventTime,
                    entry->deviceId, entry->source, entry->policyFlags,
                    entry->action, entry->flags,
                    entry->metaState, entry->buttonState, entry->edgeFlags,
                    entry->xPrecision, entry->yPrecision, entry->downTime,
                    entry->pointerCount, entry->pointerProperties, nextSample->pointerCoords);
            if (nextSample != entry->lastSample) {
                nextEntry->firstSample.next = nextSample->next;
                nextEntry->lastSample = entry->lastSample;
            }
            delete nextSample;

            entry->lastSample = const_cast<MotionSample*>(splitBatchAfterSample);
            entry->lastSample->next = NULL;

            if (entry->injectionState) {
                nextEntry->injectionState = entry->injectionState;
                entry->injectionState->refCount += 1;
            }

#if DEBUG_BATCHING
            ALOGD("Split batch of %d samples into two parts, first part has %d samples, "
                    "second part has %d samples.", originalSampleCount,
                    entry->countSamples(), nextEntry->countSamples());
#endif

            mInboundQueue.enqueueAtHead(nextEntry);
        }
    }

    
    if (conflictingPointerActions) {
        CancelationOptions options(CancelationOptions::CANCEL_POINTER_EVENTS,
                "conflicting pointer actions");
        synthesizeCancelationEventsForAllConnectionsLocked(options);
    }
    dispatchEventToCurrentInputTargetsLocked(currentTime, entry, false);
    return true;
}


void InputDispatcher::logOutboundMotionDetailsLocked(const char* prefix, const MotionEntry* entry) {
#if DEBUG_OUTBOUND_EVENT_DETAILS
    ALOGD("%seventTime=%lld, deviceId=%d, source=0x%x, policyFlags=0x%x, "
            "action=0x%x, flags=0x%x, "
            "metaState=0x%x, buttonState=0x%x, "
            "edgeFlags=0x%x, xPrecision=%f, yPrecision=%f, downTime=%lld",
            prefix,
            entry->eventTime, entry->deviceId, entry->source, entry->policyFlags,
            entry->action, entry->flags,
            entry->metaState, entry->buttonState,
            entry->edgeFlags, entry->xPrecision, entry->yPrecision,
            entry->downTime);

    
    size_t sampleCount = 1;
    const MotionSample* sample = & entry->firstSample;
    for (; sample->next != NULL; sample = sample->next) {
        sampleCount += 1;
    }
    for (uint32_t i = 0; i < entry->pointerCount; i++) {
        ALOGD("  Pointer %d: id=%d, toolType=%d, "
                "x=%f, y=%f, pressure=%f, size=%f, "
                "touchMajor=%f, touchMinor=%f, toolMajor=%f, toolMinor=%f, "
                "orientation=%f",
                i, entry->pointerProperties[i].id,
                entry->pointerProperties[i].toolType,
                sample->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_X),
                sample->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_Y),
                sample->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_PRESSURE),
                sample->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_SIZE),
                sample->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_TOUCH_MAJOR),
                sample->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_TOUCH_MINOR),
                sample->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_TOOL_MAJOR),
                sample->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_TOOL_MINOR),
                sample->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_ORIENTATION));
    }

    
    
    if (entry->action == AMOTION_EVENT_ACTION_MOVE) {
        ALOGD("  ... Total movement samples currently batched %d ...", sampleCount);
    }
#endif
}

void InputDispatcher::dispatchEventToCurrentInputTargetsLocked(nsecs_t currentTime,
        EventEntry* eventEntry, bool resumeWithAppendedMotionSample) {
#if DEBUG_DISPATCH_CYCLE
    ALOGD("dispatchEventToCurrentInputTargets - "
            "resumeWithAppendedMotionSample=%s",
            toString(resumeWithAppendedMotionSample));
#endif

    ALOG_ASSERT(eventEntry->dispatchInProgress); 

    pokeUserActivityLocked(eventEntry);

    for (size_t i = 0; i < mCurrentInputTargets.size(); i++) {
        const InputTarget& inputTarget = mCurrentInputTargets.itemAt(i);

        ssize_t connectionIndex = getConnectionIndexLocked(inputTarget.inputChannel);
        if (connectionIndex >= 0) {
            sp<Connection> connection = mConnectionsByReceiveFd.valueAt(connectionIndex);
            prepareDispatchCycleLocked(currentTime, connection, eventEntry, & inputTarget,
                    resumeWithAppendedMotionSample);
        } else {
#if DEBUG_FOCUS
            ALOGD("Dropping event delivery to target with channel '%s' because it "
                    "is no longer registered with the input dispatcher.",
                    inputTarget.inputChannel->getName().string());
#endif
        }
    }
}

void InputDispatcher::resetTargetsLocked() {
    mCurrentInputTargetsValid = false;
    mCurrentInputTargets.clear();
    resetANRTimeoutsLocked();
}

void InputDispatcher::commitTargetsLocked() {
    mCurrentInputTargetsValid = true;
}

int32_t InputDispatcher::handleTargetsNotReadyLocked(nsecs_t currentTime,
        const EventEntry* entry,
        const sp<InputApplicationHandle>& applicationHandle,
        const sp<InputWindowHandle>& windowHandle,
        nsecs_t* nextWakeupTime) {
    if (applicationHandle == NULL && windowHandle == NULL) {
        if (mInputTargetWaitCause != INPUT_TARGET_WAIT_CAUSE_SYSTEM_NOT_READY) {
#if DEBUG_FOCUS
            ALOGD("Waiting for system to become ready for input.");
#endif
            mInputTargetWaitCause = INPUT_TARGET_WAIT_CAUSE_SYSTEM_NOT_READY;
            mInputTargetWaitStartTime = currentTime;
            mInputTargetWaitTimeoutTime = LONG_LONG_MAX;
            mInputTargetWaitTimeoutExpired = false;
            mInputTargetWaitApplicationHandle.clear();
        }
    } else {
        if (mInputTargetWaitCause != INPUT_TARGET_WAIT_CAUSE_APPLICATION_NOT_READY) {
#if DEBUG_FOCUS
            ALOGD("Waiting for application to become ready for input: %s",
                    getApplicationWindowLabelLocked(applicationHandle, windowHandle).string());
#endif
            nsecs_t timeout;
            if (windowHandle != NULL) {
                timeout = windowHandle->getDispatchingTimeout(DEFAULT_INPUT_DISPATCHING_TIMEOUT);
            } else if (applicationHandle != NULL) {
                timeout = applicationHandle->getDispatchingTimeout(
                        DEFAULT_INPUT_DISPATCHING_TIMEOUT);
            } else {
                timeout = DEFAULT_INPUT_DISPATCHING_TIMEOUT;
            }

            mInputTargetWaitCause = INPUT_TARGET_WAIT_CAUSE_APPLICATION_NOT_READY;
            mInputTargetWaitStartTime = currentTime;
            mInputTargetWaitTimeoutTime = currentTime + timeout;
            mInputTargetWaitTimeoutExpired = false;
            mInputTargetWaitApplicationHandle.clear();

            if (windowHandle != NULL) {
                mInputTargetWaitApplicationHandle = windowHandle->inputApplicationHandle;
            }
            if (mInputTargetWaitApplicationHandle == NULL && applicationHandle != NULL) {
                mInputTargetWaitApplicationHandle = applicationHandle;
            }
        }
    }

    if (mInputTargetWaitTimeoutExpired) {
        return INPUT_EVENT_INJECTION_TIMED_OUT;
    }

    if (currentTime >= mInputTargetWaitTimeoutTime) {
        onANRLocked(currentTime, applicationHandle, windowHandle,
                entry->eventTime, mInputTargetWaitStartTime);

        
        
        *nextWakeupTime = LONG_LONG_MIN;
        return INPUT_EVENT_INJECTION_PENDING;
    } else {
        
        if (mInputTargetWaitTimeoutTime < *nextWakeupTime) {
            *nextWakeupTime = mInputTargetWaitTimeoutTime;
        }
        return INPUT_EVENT_INJECTION_PENDING;
    }
}

void InputDispatcher::resumeAfterTargetsNotReadyTimeoutLocked(nsecs_t newTimeout,
        const sp<InputChannel>& inputChannel) {
    if (newTimeout > 0) {
        
        mInputTargetWaitTimeoutTime = now() + newTimeout;
    } else {
        
        mInputTargetWaitTimeoutExpired = true;

        
        mTouchState.reset();

        
        if (inputChannel.get()) {
            ssize_t connectionIndex = getConnectionIndexLocked(inputChannel);
            if (connectionIndex >= 0) {
                sp<Connection> connection = mConnectionsByReceiveFd.valueAt(connectionIndex);
                if (connection->status == Connection::STATUS_NORMAL) {
                    CancelationOptions options(CancelationOptions::CANCEL_ALL_EVENTS,
                            "application not responding");
                    synthesizeCancelationEventsForConnectionLocked(connection, options);
                }
            }
        }
    }
}

nsecs_t InputDispatcher::getTimeSpentWaitingForApplicationLocked(
        nsecs_t currentTime) {
    if (mInputTargetWaitCause == INPUT_TARGET_WAIT_CAUSE_APPLICATION_NOT_READY) {
        return currentTime - mInputTargetWaitStartTime;
    }
    return 0;
}

void InputDispatcher::resetANRTimeoutsLocked() {
#if DEBUG_FOCUS
        ALOGD("Resetting ANR timeouts.");
#endif

    
    mInputTargetWaitCause = INPUT_TARGET_WAIT_CAUSE_NONE;
    mInputTargetWaitApplicationHandle.clear();
}

int32_t InputDispatcher::findFocusedWindowTargetsLocked(nsecs_t currentTime,
        const EventEntry* entry, nsecs_t* nextWakeupTime) {
    mCurrentInputTargets.clear();

    int32_t injectionResult;

    
    
    if (mFocusedWindowHandle == NULL) {
        if (mFocusedApplicationHandle != NULL) {
#if DEBUG_FOCUS
            ALOGD("Waiting because there is no focused window but there is a "
                    "focused application that may eventually add a window: %s.",
                    getApplicationWindowLabelLocked(mFocusedApplicationHandle, NULL).string());
#endif
            injectionResult = handleTargetsNotReadyLocked(currentTime, entry,
                    mFocusedApplicationHandle, NULL, nextWakeupTime);
            goto Unresponsive;
        }

        ALOGI("Dropping event because there is no focused window or focused application.");
        injectionResult = INPUT_EVENT_INJECTION_FAILED;
        goto Failed;
    }

    
    if (! checkInjectionPermission(mFocusedWindowHandle, entry->injectionState)) {
        injectionResult = INPUT_EVENT_INJECTION_PERMISSION_DENIED;
        goto Failed;
    }

    
    if (mFocusedWindowHandle->getInfo()->paused) {
#if DEBUG_FOCUS
        ALOGD("Waiting because focused window is paused.");
#endif
        injectionResult = handleTargetsNotReadyLocked(currentTime, entry,
                mFocusedApplicationHandle, mFocusedWindowHandle, nextWakeupTime);
        goto Unresponsive;
    }

    
    if (! isWindowFinishedWithPreviousInputLocked(mFocusedWindowHandle)) {
#if DEBUG_FOCUS
        ALOGD("Waiting because focused window still processing previous input.");
#endif
        injectionResult = handleTargetsNotReadyLocked(currentTime, entry,
                mFocusedApplicationHandle, mFocusedWindowHandle, nextWakeupTime);
        goto Unresponsive;
    }

    
    injectionResult = INPUT_EVENT_INJECTION_SUCCEEDED;
    addWindowTargetLocked(mFocusedWindowHandle,
            InputTarget::FLAG_FOREGROUND | InputTarget::FLAG_DISPATCH_AS_IS, BitSet32(0));

    
Failed:
Unresponsive:
    nsecs_t timeSpentWaitingForApplication = getTimeSpentWaitingForApplicationLocked(currentTime);
    updateDispatchStatisticsLocked(currentTime, entry,
            injectionResult, timeSpentWaitingForApplication);
#if DEBUG_FOCUS
    ALOGD("findFocusedWindow finished: injectionResult=%d, "
            "timeSpendWaitingForApplication=%0.1fms",
            injectionResult, timeSpentWaitingForApplication / 1000000.0);
#endif
    return injectionResult;
}

int32_t InputDispatcher::findTouchedWindowTargetsLocked(nsecs_t currentTime,
        const MotionEntry* entry, nsecs_t* nextWakeupTime, bool* outConflictingPointerActions,
        const MotionSample** outSplitBatchAfterSample) {
    enum InjectionPermission {
        INJECTION_PERMISSION_UNKNOWN,
        INJECTION_PERMISSION_GRANTED,
        INJECTION_PERMISSION_DENIED
    };

    mCurrentInputTargets.clear();

    nsecs_t startTime = now();

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    bool screenWasOff = false; 

    int32_t action = entry->action;
    int32_t maskedAction = action & AMOTION_EVENT_ACTION_MASK;

    
    int32_t injectionResult = INPUT_EVENT_INJECTION_PENDING;
    InjectionPermission injectionPermission = INJECTION_PERMISSION_UNKNOWN;
    sp<InputWindowHandle> newHoverWindowHandle;

    bool isSplit = mTouchState.split;
    bool switchedDevice = mTouchState.deviceId >= 0
            && (mTouchState.deviceId != entry->deviceId
                    || mTouchState.source != entry->source);
    bool isHoverAction = (maskedAction == AMOTION_EVENT_ACTION_HOVER_MOVE
            || maskedAction == AMOTION_EVENT_ACTION_HOVER_ENTER
            || maskedAction == AMOTION_EVENT_ACTION_HOVER_EXIT);
    bool newGesture = (maskedAction == AMOTION_EVENT_ACTION_DOWN
            || maskedAction == AMOTION_EVENT_ACTION_SCROLL
            || isHoverAction);
    bool wrongDevice = false;
    if (newGesture) {
        bool down = maskedAction == AMOTION_EVENT_ACTION_DOWN;
        if (switchedDevice && mTouchState.down && !down) {
#if DEBUG_FOCUS
            ALOGD("Dropping event because a pointer for a different device is already down.");
#endif
            mTempTouchState.copyFrom(mTouchState);
            injectionResult = INPUT_EVENT_INJECTION_FAILED;
            switchedDevice = false;
            wrongDevice = true;
            goto Failed;
        }
        mTempTouchState.reset();
        mTempTouchState.down = down;
        mTempTouchState.deviceId = entry->deviceId;
        mTempTouchState.source = entry->source;
        isSplit = false;
    } else {
        mTempTouchState.copyFrom(mTouchState);
    }

    if (newGesture || (isSplit && maskedAction == AMOTION_EVENT_ACTION_POINTER_DOWN)) {
        

        const MotionSample* sample = &entry->firstSample;
        int32_t pointerIndex = getMotionEventActionPointerIndex(action);
        int32_t x = int32_t(sample->pointerCoords[pointerIndex].
                getAxisValue(AMOTION_EVENT_AXIS_X));
        int32_t y = int32_t(sample->pointerCoords[pointerIndex].
                getAxisValue(AMOTION_EVENT_AXIS_Y));
        sp<InputWindowHandle> newTouchedWindowHandle;
        sp<InputWindowHandle> topErrorWindowHandle;
        bool isTouchModal = false;

        
        size_t numWindows = mWindowHandles.size();
        for (size_t i = 0; i < numWindows; i++) {
            sp<InputWindowHandle> windowHandle = mWindowHandles.itemAt(i);
            const InputWindowInfo* windowInfo = windowHandle->getInfo();
            int32_t flags = windowInfo->layoutParamsFlags;

            if (flags & InputWindowInfo::FLAG_SYSTEM_ERROR) {
                if (topErrorWindowHandle == NULL) {
                    topErrorWindowHandle = windowHandle;
                }
            }

            if (windowInfo->visible) {
                if (! (flags & InputWindowInfo::FLAG_NOT_TOUCHABLE)) {
                    isTouchModal = (flags & (InputWindowInfo::FLAG_NOT_FOCUSABLE
                            | InputWindowInfo::FLAG_NOT_TOUCH_MODAL)) == 0;
                    if (isTouchModal || windowInfo->touchableRegionContainsPoint(x, y)) {
                        if (! screenWasOff
                                || (flags & InputWindowInfo::FLAG_TOUCHABLE_WHEN_WAKING)) {
                            newTouchedWindowHandle = windowHandle;
                        }
                        break; 
                    }
                }

                if (maskedAction == AMOTION_EVENT_ACTION_DOWN
                        && (flags & InputWindowInfo::FLAG_WATCH_OUTSIDE_TOUCH)) {
                    int32_t outsideTargetFlags = InputTarget::FLAG_DISPATCH_AS_OUTSIDE;
                    if (isWindowObscuredAtPointLocked(windowHandle, x, y)) {
                        outsideTargetFlags |= InputTarget::FLAG_WINDOW_IS_OBSCURED;
                    }

                    mTempTouchState.addOrUpdateWindow(
                            windowHandle, outsideTargetFlags, BitSet32(0));
                }
            }
        }

        
        
        
        if (topErrorWindowHandle != NULL && newTouchedWindowHandle != topErrorWindowHandle) {
#if DEBUG_FOCUS
            ALOGD("Waiting because system error window is pending.");
#endif
            injectionResult = handleTargetsNotReadyLocked(currentTime, entry,
                    NULL, NULL, nextWakeupTime);
            injectionPermission = INJECTION_PERMISSION_UNKNOWN;
            goto Unresponsive;
        }

        
        if (newTouchedWindowHandle != NULL
                && newTouchedWindowHandle->getInfo()->supportsSplitTouch()) {
            
            isSplit = true;
        } else if (isSplit) {
            
            
            
            newTouchedWindowHandle = mTempTouchState.getFirstForegroundWindowHandle();
        }

        
        if (newTouchedWindowHandle == NULL) {
            if (mFocusedApplicationHandle != NULL) {
#if DEBUG_FOCUS
                ALOGD("Waiting because there is no touched window but there is a "
                        "focused application that may eventually add a new window: %s.",
                        getApplicationWindowLabelLocked(mFocusedApplicationHandle, NULL).string());
#endif
                injectionResult = handleTargetsNotReadyLocked(currentTime, entry,
                        mFocusedApplicationHandle, NULL, nextWakeupTime);
                goto Unresponsive;
            }

            ALOGI("Dropping event because there is no touched window or focused application.");
            injectionResult = INPUT_EVENT_INJECTION_FAILED;
            goto Failed;
        }

        
        int32_t targetFlags = InputTarget::FLAG_FOREGROUND | InputTarget::FLAG_DISPATCH_AS_IS;
        if (isSplit) {
            targetFlags |= InputTarget::FLAG_SPLIT;
        }
        if (isWindowObscuredAtPointLocked(newTouchedWindowHandle, x, y)) {
            targetFlags |= InputTarget::FLAG_WINDOW_IS_OBSCURED;
        }

        
        if (isHoverAction) {
            newHoverWindowHandle = newTouchedWindowHandle;

            
            
            
            if (!isTouchModal) {
                while (sample->next) {
                    if (!newHoverWindowHandle->getInfo()->touchableRegionContainsPoint(
                            sample->next->pointerCoords[0].getAxisValue(AMOTION_EVENT_AXIS_X),
                            sample->next->pointerCoords[0].getAxisValue(AMOTION_EVENT_AXIS_Y))) {
                        *outSplitBatchAfterSample = sample;
                        break;
                    }
                    sample = sample->next;
                }
            }
        } else if (maskedAction == AMOTION_EVENT_ACTION_SCROLL) {
            newHoverWindowHandle = mLastHoverWindowHandle;
        }

        
        BitSet32 pointerIds;
        if (isSplit) {
            uint32_t pointerId = entry->pointerProperties[pointerIndex].id;
            pointerIds.markBit(pointerId);
        }
        mTempTouchState.addOrUpdateWindow(newTouchedWindowHandle, targetFlags, pointerIds);
    } else {
        

        
        if (! mTempTouchState.down) {
#if DEBUG_FOCUS
            ALOGD("Dropping event because the pointer is not down or we previously "
                    "dropped the pointer down event.");
#endif
            injectionResult = INPUT_EVENT_INJECTION_FAILED;
            goto Failed;
        }

        
        if (maskedAction == AMOTION_EVENT_ACTION_MOVE
                && entry->pointerCount == 1
                && mTempTouchState.isSlippery()) {
            const MotionSample* sample = &entry->firstSample;
            int32_t x = int32_t(sample->pointerCoords[0].getAxisValue(AMOTION_EVENT_AXIS_X));
            int32_t y = int32_t(sample->pointerCoords[0].getAxisValue(AMOTION_EVENT_AXIS_Y));

            sp<InputWindowHandle> oldTouchedWindowHandle =
                    mTempTouchState.getFirstForegroundWindowHandle();
            sp<InputWindowHandle> newTouchedWindowHandle = findTouchedWindowAtLocked(x, y);
            if (oldTouchedWindowHandle != newTouchedWindowHandle
                    && newTouchedWindowHandle != NULL) {
#if DEBUG_FOCUS
                ALOGD("Touch is slipping out of window %s into window %s.",
                        oldTouchedWindowHandle->getName().string(),
                        newTouchedWindowHandle->getName().string());
#endif
                
                mTempTouchState.addOrUpdateWindow(oldTouchedWindowHandle,
                        InputTarget::FLAG_DISPATCH_AS_SLIPPERY_EXIT, BitSet32(0));

                
                if (newTouchedWindowHandle->getInfo()->supportsSplitTouch()) {
                    isSplit = true;
                }

                int32_t targetFlags = InputTarget::FLAG_FOREGROUND
                        | InputTarget::FLAG_DISPATCH_AS_SLIPPERY_ENTER;
                if (isSplit) {
                    targetFlags |= InputTarget::FLAG_SPLIT;
                }
                if (isWindowObscuredAtPointLocked(newTouchedWindowHandle, x, y)) {
                    targetFlags |= InputTarget::FLAG_WINDOW_IS_OBSCURED;
                }

                BitSet32 pointerIds;
                if (isSplit) {
                    pointerIds.markBit(entry->pointerProperties[0].id);
                }
                mTempTouchState.addOrUpdateWindow(newTouchedWindowHandle, targetFlags, pointerIds);

                
                *outSplitBatchAfterSample = &entry->firstSample;
            }
        }
    }

    if (newHoverWindowHandle != mLastHoverWindowHandle) {
        
        *outSplitBatchAfterSample = &entry->firstSample;

        
        if (mLastHoverWindowHandle != NULL) {
#if DEBUG_HOVER
            ALOGD("Sending hover exit event to window %s.",
                    mLastHoverWindowHandle->getName().string());
#endif
            mTempTouchState.addOrUpdateWindow(mLastHoverWindowHandle,
                    InputTarget::FLAG_DISPATCH_AS_HOVER_EXIT, BitSet32(0));
        }

        
        if (newHoverWindowHandle != NULL) {
#if DEBUG_HOVER
            ALOGD("Sending hover enter event to window %s.",
                    newHoverWindowHandle->getName().string());
#endif
            mTempTouchState.addOrUpdateWindow(newHoverWindowHandle,
                    InputTarget::FLAG_DISPATCH_AS_HOVER_ENTER, BitSet32(0));
        }
    }

    
    
    {
        bool haveForegroundWindow = false;
        for (size_t i = 0; i < mTempTouchState.windows.size(); i++) {
            const TouchedWindow& touchedWindow = mTempTouchState.windows[i];
            if (touchedWindow.targetFlags & InputTarget::FLAG_FOREGROUND) {
                haveForegroundWindow = true;
                if (! checkInjectionPermission(touchedWindow.windowHandle,
                        entry->injectionState)) {
                    injectionResult = INPUT_EVENT_INJECTION_PERMISSION_DENIED;
                    injectionPermission = INJECTION_PERMISSION_DENIED;
                    goto Failed;
                }
            }
        }
        if (! haveForegroundWindow) {
#if DEBUG_FOCUS
            ALOGD("Dropping event because there is no touched foreground window to receive it.");
#endif
            injectionResult = INPUT_EVENT_INJECTION_FAILED;
            goto Failed;
        }

        
        injectionPermission = INJECTION_PERMISSION_GRANTED;
    }

    
    
    if (maskedAction == AMOTION_EVENT_ACTION_DOWN) {
        sp<InputWindowHandle> foregroundWindowHandle =
                mTempTouchState.getFirstForegroundWindowHandle();
        const int32_t foregroundWindowUid = foregroundWindowHandle->getInfo()->ownerUid;
        for (size_t i = 0; i < mTempTouchState.windows.size(); i++) {
            const TouchedWindow& touchedWindow = mTempTouchState.windows[i];
            if (touchedWindow.targetFlags & InputTarget::FLAG_DISPATCH_AS_OUTSIDE) {
                sp<InputWindowHandle> inputWindowHandle = touchedWindow.windowHandle;
                if (inputWindowHandle->getInfo()->ownerUid != foregroundWindowUid) {
                    mTempTouchState.addOrUpdateWindow(inputWindowHandle,
                            InputTarget::FLAG_ZERO_COORDS, BitSet32(0));
                }
            }
        }
    }

    
    for (size_t i = 0; i < mTempTouchState.windows.size(); i++) {
        const TouchedWindow& touchedWindow = mTempTouchState.windows[i];
        if (touchedWindow.targetFlags & InputTarget::FLAG_FOREGROUND) {
            
            if (touchedWindow.windowHandle->getInfo()->paused) {
#if DEBUG_FOCUS
                ALOGD("Waiting because touched window is paused.");
#endif
                injectionResult = handleTargetsNotReadyLocked(currentTime, entry,
                        NULL, touchedWindow.windowHandle, nextWakeupTime);
                goto Unresponsive;
            }

            
            if (! isWindowFinishedWithPreviousInputLocked(touchedWindow.windowHandle)) {
#if DEBUG_FOCUS
                ALOGD("Waiting because touched window still processing previous input.");
#endif
                injectionResult = handleTargetsNotReadyLocked(currentTime, entry,
                        NULL, touchedWindow.windowHandle, nextWakeupTime);
                goto Unresponsive;
            }
        }
    }

    
    
    
    
    
    
    if (maskedAction == AMOTION_EVENT_ACTION_DOWN) {
        sp<InputWindowHandle> foregroundWindowHandle =
                mTempTouchState.getFirstForegroundWindowHandle();
        if (foregroundWindowHandle->getInfo()->hasWallpaper) {
            for (size_t i = 0; i < mWindowHandles.size(); i++) {
                sp<InputWindowHandle> windowHandle = mWindowHandles.itemAt(i);
                if (windowHandle->getInfo()->layoutParamsType
                        == InputWindowInfo::TYPE_WALLPAPER) {
                    mTempTouchState.addOrUpdateWindow(windowHandle,
                            InputTarget::FLAG_WINDOW_IS_OBSCURED
                                    | InputTarget::FLAG_DISPATCH_AS_IS,
                            BitSet32(0));
                }
            }
        }
    }

    
    injectionResult = INPUT_EVENT_INJECTION_SUCCEEDED;

    for (size_t i = 0; i < mTempTouchState.windows.size(); i++) {
        const TouchedWindow& touchedWindow = mTempTouchState.windows.itemAt(i);
        addWindowTargetLocked(touchedWindow.windowHandle, touchedWindow.targetFlags,
                touchedWindow.pointerIds);
    }

    
    
    mTempTouchState.filterNonAsIsTouchWindows();

Failed:
    
    if (injectionPermission == INJECTION_PERMISSION_UNKNOWN) {
        if (checkInjectionPermission(NULL, entry->injectionState)) {
            injectionPermission = INJECTION_PERMISSION_GRANTED;
        } else {
            injectionPermission = INJECTION_PERMISSION_DENIED;
        }
    }

    
    if (injectionPermission == INJECTION_PERMISSION_GRANTED) {
        if (!wrongDevice) {
            if (switchedDevice) {
#if DEBUG_FOCUS
                ALOGD("Conflicting pointer actions: Switched to a different device.");
#endif
                *outConflictingPointerActions = true;
            }

            if (isHoverAction) {
                
                if (mTouchState.down) {
#if DEBUG_FOCUS
                    ALOGD("Conflicting pointer actions: Hover received while pointer was down.");
#endif
                    *outConflictingPointerActions = true;
                }
                mTouchState.reset();
                if (maskedAction == AMOTION_EVENT_ACTION_HOVER_ENTER
                        || maskedAction == AMOTION_EVENT_ACTION_HOVER_MOVE) {
                    mTouchState.deviceId = entry->deviceId;
                    mTouchState.source = entry->source;
                }
            } else if (maskedAction == AMOTION_EVENT_ACTION_UP
                    || maskedAction == AMOTION_EVENT_ACTION_CANCEL) {
                
                mTouchState.reset();
            } else if (maskedAction == AMOTION_EVENT_ACTION_DOWN) {
                
                if (mTouchState.down) {
#if DEBUG_FOCUS
                    ALOGD("Conflicting pointer actions: Down received while already down.");
#endif
                    *outConflictingPointerActions = true;
                }
                mTouchState.copyFrom(mTempTouchState);
            } else if (maskedAction == AMOTION_EVENT_ACTION_POINTER_UP) {
                
                if (isSplit) {
                    int32_t pointerIndex = getMotionEventActionPointerIndex(action);
                    uint32_t pointerId = entry->pointerProperties[pointerIndex].id;

                    for (size_t i = 0; i < mTempTouchState.windows.size(); ) {
                        TouchedWindow& touchedWindow = mTempTouchState.windows.editItemAt(i);
                        if (touchedWindow.targetFlags & InputTarget::FLAG_SPLIT) {
                            touchedWindow.pointerIds.clearBit(pointerId);
                            if (touchedWindow.pointerIds.isEmpty()) {
                                mTempTouchState.windows.removeAt(i);
                                continue;
                            }
                        }
                        i += 1;
                    }
                }
                mTouchState.copyFrom(mTempTouchState);
            } else if (maskedAction == AMOTION_EVENT_ACTION_SCROLL) {
                
            } else {
                
                mTouchState.copyFrom(mTempTouchState);
            }

            
            mLastHoverWindowHandle = newHoverWindowHandle;
        }
    } else {
#if DEBUG_FOCUS
        ALOGD("Not updating touch focus because injection was denied.");
#endif
    }

Unresponsive:
    
    mTempTouchState.reset();

    nsecs_t timeSpentWaitingForApplication = getTimeSpentWaitingForApplicationLocked(currentTime);
    updateDispatchStatisticsLocked(currentTime, entry,
            injectionResult, timeSpentWaitingForApplication);
#if DEBUG_FOCUS
    ALOGD("findTouchedWindow finished: injectionResult=%d, injectionPermission=%d, "
            "timeSpentWaitingForApplication=%0.1fms",
            injectionResult, injectionPermission, timeSpentWaitingForApplication / 1000000.0);
#endif
    return injectionResult;
}

void InputDispatcher::addWindowTargetLocked(const sp<InputWindowHandle>& windowHandle,
        int32_t targetFlags, BitSet32 pointerIds) {
    mCurrentInputTargets.push();

    const InputWindowInfo* windowInfo = windowHandle->getInfo();
    InputTarget& target = mCurrentInputTargets.editTop();
    target.inputChannel = windowInfo->inputChannel;
    target.flags = targetFlags;
    target.xOffset = - windowInfo->frameLeft;
    target.yOffset = - windowInfo->frameTop;
    target.scaleFactor = windowInfo->scaleFactor;
    target.pointerIds = pointerIds;
}

void InputDispatcher::addMonitoringTargetsLocked() {
    for (size_t i = 0; i < mMonitoringChannels.size(); i++) {
        mCurrentInputTargets.push();

        InputTarget& target = mCurrentInputTargets.editTop();
        target.inputChannel = mMonitoringChannels[i];
        target.flags = InputTarget::FLAG_DISPATCH_AS_IS;
        target.xOffset = 0;
        target.yOffset = 0;
        target.pointerIds.clear();
        target.scaleFactor = 1.0f;
    }
}

bool InputDispatcher::checkInjectionPermission(const sp<InputWindowHandle>& windowHandle,
        const InjectionState* injectionState) {
    if (injectionState
            && (windowHandle == NULL
                    || windowHandle->getInfo()->ownerUid != injectionState->injectorUid)
            && !hasInjectionPermission(injectionState->injectorPid, injectionState->injectorUid)) {
        if (windowHandle != NULL) {
            ALOGW("Permission denied: injecting event from pid %d uid %d to window %s "
                    "owned by uid %d",
                    injectionState->injectorPid, injectionState->injectorUid,
                    windowHandle->getName().string(),
                    windowHandle->getInfo()->ownerUid);
        } else {
            ALOGW("Permission denied: injecting event from pid %d uid %d",
                    injectionState->injectorPid, injectionState->injectorUid);
        }
        return false;
    }
    return true;
}

bool InputDispatcher::isWindowObscuredAtPointLocked(
        const sp<InputWindowHandle>& windowHandle, int32_t x, int32_t y) const {
    size_t numWindows = mWindowHandles.size();
    for (size_t i = 0; i < numWindows; i++) {
        sp<InputWindowHandle> otherHandle = mWindowHandles.itemAt(i);
        if (otherHandle == windowHandle) {
            break;
        }

        const InputWindowInfo* otherInfo = otherHandle->getInfo();
        if (otherInfo->visible && ! otherInfo->isTrustedOverlay()
                && otherInfo->frameContainsPoint(x, y)) {
            return true;
        }
    }
    return false;
}

bool InputDispatcher::isWindowFinishedWithPreviousInputLocked(
        const sp<InputWindowHandle>& windowHandle) {
    ssize_t connectionIndex = getConnectionIndexLocked(windowHandle->getInputChannel());
    if (connectionIndex >= 0) {
        sp<Connection> connection = mConnectionsByReceiveFd.valueAt(connectionIndex);
        return connection->outboundQueue.isEmpty();
    } else {
        return true;
    }
}

String8 InputDispatcher::getApplicationWindowLabelLocked(
        const sp<InputApplicationHandle>& applicationHandle,
        const sp<InputWindowHandle>& windowHandle) {
    if (applicationHandle != NULL) {
        if (windowHandle != NULL) {
            String8 label(applicationHandle->getName());
            label.append(" - ");
            label.append(windowHandle->getName());
            return label;
        } else {
            return applicationHandle->getName();
        }
    } else if (windowHandle != NULL) {
        return windowHandle->getName();
    } else {
        return String8("<unknown application or window>");
    }
}

void InputDispatcher::pokeUserActivityLocked(const EventEntry* eventEntry) {
    int32_t eventType = POWER_MANAGER_OTHER_EVENT;
    switch (eventEntry->type) {
    case EventEntry::TYPE_MOTION: {
        const MotionEntry* motionEntry = static_cast<const MotionEntry*>(eventEntry);
        if (motionEntry->action == AMOTION_EVENT_ACTION_CANCEL) {
            return;
        }

        if (MotionEvent::isTouchEvent(motionEntry->source, motionEntry->action)) {
            eventType = POWER_MANAGER_TOUCH_EVENT;
        }
        break;
    }
    case EventEntry::TYPE_KEY: {
        const KeyEntry* keyEntry = static_cast<const KeyEntry*>(eventEntry);
        if (keyEntry->flags & AKEY_EVENT_FLAG_CANCELED) {
            return;
        }
        eventType = POWER_MANAGER_BUTTON_EVENT;
        break;
    }
    }

    CommandEntry* commandEntry = postCommandLocked(
            & InputDispatcher::doPokeUserActivityLockedInterruptible);
    commandEntry->eventTime = eventEntry->eventTime;
    commandEntry->userActivityEventType = eventType;
}

void InputDispatcher::prepareDispatchCycleLocked(nsecs_t currentTime,
        const sp<Connection>& connection, EventEntry* eventEntry, const InputTarget* inputTarget,
        bool resumeWithAppendedMotionSample) {
#if DEBUG_DISPATCH_CYCLE
    ALOGD("channel '%s' ~ prepareDispatchCycle - flags=0x%08x, "
            "xOffset=%f, yOffset=%f, scaleFactor=%f, "
            "pointerIds=0x%x, "
            "resumeWithAppendedMotionSample=%s",
            connection->getInputChannelName(), inputTarget->flags,
            inputTarget->xOffset, inputTarget->yOffset,
            inputTarget->scaleFactor, inputTarget->pointerIds.value,
            toString(resumeWithAppendedMotionSample));
#endif

    
    bool isSplit = inputTarget->flags & InputTarget::FLAG_SPLIT;
    ALOG_ASSERT(! (resumeWithAppendedMotionSample && isSplit));

    
    
    if (connection->status != Connection::STATUS_NORMAL) {
#if DEBUG_DISPATCH_CYCLE
        ALOGD("channel '%s' ~ Dropping event because the channel status is %s",
                connection->getInputChannelName(), connection->getStatusLabel());
#endif
        return;
    }

    
    if (isSplit) {
        ALOG_ASSERT(eventEntry->type == EventEntry::TYPE_MOTION);

        MotionEntry* originalMotionEntry = static_cast<MotionEntry*>(eventEntry);
        if (inputTarget->pointerIds.count() != originalMotionEntry->pointerCount) {
            MotionEntry* splitMotionEntry = splitMotionEvent(
                    originalMotionEntry, inputTarget->pointerIds);
            if (!splitMotionEntry) {
                return; 
            }
#if DEBUG_FOCUS
            ALOGD("channel '%s' ~ Split motion event.",
                    connection->getInputChannelName());
            logOutboundMotionDetailsLocked("  ", splitMotionEntry);
#endif
            eventEntry = splitMotionEntry;
        }
    }

    
    
    
    
    bool wasEmpty = connection->outboundQueue.isEmpty();

    if (! wasEmpty && resumeWithAppendedMotionSample) {
        DispatchEntry* motionEventDispatchEntry =
                connection->findQueuedDispatchEntryForEvent(eventEntry);
        if (motionEventDispatchEntry) {
            
            
            
            if (! motionEventDispatchEntry->inProgress) {
#if DEBUG_BATCHING
                ALOGD("channel '%s' ~ Not streaming because the motion event has "
                        "not yet been dispatched.  "
                        "(Waiting for earlier events to be consumed.)",
                        connection->getInputChannelName());
#endif
                return;
            }

            
            
            
            
            
            if (motionEventDispatchEntry->tailMotionSample) {
#if DEBUG_BATCHING
                ALOGD("channel '%s' ~ Not streaming because no new samples can "
                        "be appended to the motion event in this dispatch cycle.  "
                        "(Waiting for next dispatch cycle to start.)",
                        connection->getInputChannelName());
#endif
                return;
            }

            
            if ((motionEventDispatchEntry->targetFlags & InputTarget::FLAG_DISPATCH_MASK)
                    != InputTarget::FLAG_DISPATCH_AS_IS) {
#if DEBUG_BATCHING
                ALOGD("channel '%s' ~ Not streaming because the motion event was not "
                        "being dispatched as-is.  "
                        "(Waiting for next dispatch cycle to start.)",
                        connection->getInputChannelName());
#endif
                return;
            }

            
            
            
            MotionEntry* motionEntry = static_cast<MotionEntry*>(eventEntry);
            MotionSample* appendedMotionSample = motionEntry->lastSample;
            status_t status;
            if (motionEventDispatchEntry->scaleFactor == 1.0f) {
                status = connection->inputPublisher.appendMotionSample(
                        appendedMotionSample->eventTime, appendedMotionSample->pointerCoords);
            } else {
                PointerCoords scaledCoords[MAX_POINTERS];
                for (size_t i = 0; i < motionEntry->pointerCount; i++) {
                    scaledCoords[i] = appendedMotionSample->pointerCoords[i];
                    scaledCoords[i].scale(motionEventDispatchEntry->scaleFactor);
                }
                status = connection->inputPublisher.appendMotionSample(
                        appendedMotionSample->eventTime, scaledCoords);
            }
            if (status == OK) {
#if DEBUG_BATCHING
                ALOGD("channel '%s' ~ Successfully streamed new motion sample.",
                        connection->getInputChannelName());
#endif
                return;
            }

#if DEBUG_BATCHING
            if (status == NO_MEMORY) {
                ALOGD("channel '%s' ~ Could not append motion sample to currently "
                        "dispatched move event because the shared memory buffer is full.  "
                        "(Waiting for next dispatch cycle to start.)",
                        connection->getInputChannelName());
            } else if (status == status_t(FAILED_TRANSACTION)) {
                ALOGD("channel '%s' ~ Could not append motion sample to currently "
                        "dispatched move event because the event has already been consumed.  "
                        "(Waiting for next dispatch cycle to start.)",
                        connection->getInputChannelName());
            } else {
                ALOGD("channel '%s' ~ Could not append motion sample to currently "
                        "dispatched move event due to an error, status=%d.  "
                        "(Waiting for next dispatch cycle to start.)",
                        connection->getInputChannelName(), status);
            }
#endif
            
            
            motionEventDispatchEntry->tailMotionSample = appendedMotionSample;
            return;
        }
    }

    
    enqueueDispatchEntryLocked(connection, eventEntry, inputTarget,
            resumeWithAppendedMotionSample, InputTarget::FLAG_DISPATCH_AS_HOVER_EXIT);
    enqueueDispatchEntryLocked(connection, eventEntry, inputTarget,
            resumeWithAppendedMotionSample, InputTarget::FLAG_DISPATCH_AS_OUTSIDE);
    enqueueDispatchEntryLocked(connection, eventEntry, inputTarget,
            resumeWithAppendedMotionSample, InputTarget::FLAG_DISPATCH_AS_HOVER_ENTER);
    enqueueDispatchEntryLocked(connection, eventEntry, inputTarget,
            resumeWithAppendedMotionSample, InputTarget::FLAG_DISPATCH_AS_IS);
    enqueueDispatchEntryLocked(connection, eventEntry, inputTarget,
            resumeWithAppendedMotionSample, InputTarget::FLAG_DISPATCH_AS_SLIPPERY_EXIT);
    enqueueDispatchEntryLocked(connection, eventEntry, inputTarget,
            resumeWithAppendedMotionSample, InputTarget::FLAG_DISPATCH_AS_SLIPPERY_ENTER);

    
    if (wasEmpty && !connection->outboundQueue.isEmpty()) {
        activateConnectionLocked(connection.get());
        startDispatchCycleLocked(currentTime, connection);
    }
}

void InputDispatcher::enqueueDispatchEntryLocked(
        const sp<Connection>& connection, EventEntry* eventEntry, const InputTarget* inputTarget,
        bool resumeWithAppendedMotionSample, int32_t dispatchMode) {
    int32_t inputTargetFlags = inputTarget->flags;
    if (!(inputTargetFlags & dispatchMode)) {
        return;
    }
    inputTargetFlags = (inputTargetFlags & ~InputTarget::FLAG_DISPATCH_MASK) | dispatchMode;

    
    
    DispatchEntry* dispatchEntry = new DispatchEntry(eventEntry, 
            inputTargetFlags, inputTarget->xOffset, inputTarget->yOffset,
            inputTarget->scaleFactor);
    if (dispatchEntry->hasForegroundTarget()) {
        incrementPendingForegroundDispatchesLocked(eventEntry);
    }

    
    
    
    
    if (resumeWithAppendedMotionSample) {
#if DEBUG_BATCHING
        ALOGD("channel '%s' ~ Preparing a new dispatch cycle for additional motion samples "
                "that cannot be streamed because the motion event has already been consumed.",
                connection->getInputChannelName());
#endif
        MotionSample* appendedMotionSample = static_cast<MotionEntry*>(eventEntry)->lastSample;
        dispatchEntry->headMotionSample = appendedMotionSample;
    }

    
    switch (eventEntry->type) {
    case EventEntry::TYPE_KEY: {
        KeyEntry* keyEntry = static_cast<KeyEntry*>(eventEntry);
        dispatchEntry->resolvedAction = keyEntry->action;
        dispatchEntry->resolvedFlags = keyEntry->flags;

        if (!connection->inputState.trackKey(keyEntry,
                dispatchEntry->resolvedAction, dispatchEntry->resolvedFlags)) {
#if DEBUG_DISPATCH_CYCLE
            ALOGD("channel '%s' ~ enqueueDispatchEntryLocked: skipping inconsistent key event",
                    connection->getInputChannelName());
#endif
            return; 
        }
        break;
    }

    case EventEntry::TYPE_MOTION: {
        MotionEntry* motionEntry = static_cast<MotionEntry*>(eventEntry);
        if (dispatchMode & InputTarget::FLAG_DISPATCH_AS_OUTSIDE) {
            dispatchEntry->resolvedAction = AMOTION_EVENT_ACTION_OUTSIDE;
        } else if (dispatchMode & InputTarget::FLAG_DISPATCH_AS_HOVER_EXIT) {
            dispatchEntry->resolvedAction = AMOTION_EVENT_ACTION_HOVER_EXIT;
        } else if (dispatchMode & InputTarget::FLAG_DISPATCH_AS_HOVER_ENTER) {
            dispatchEntry->resolvedAction = AMOTION_EVENT_ACTION_HOVER_ENTER;
        } else if (dispatchMode & InputTarget::FLAG_DISPATCH_AS_SLIPPERY_EXIT) {
            dispatchEntry->resolvedAction = AMOTION_EVENT_ACTION_CANCEL;
        } else if (dispatchMode & InputTarget::FLAG_DISPATCH_AS_SLIPPERY_ENTER) {
            dispatchEntry->resolvedAction = AMOTION_EVENT_ACTION_DOWN;
        } else {
            dispatchEntry->resolvedAction = motionEntry->action;
        }
        if (dispatchEntry->resolvedAction == AMOTION_EVENT_ACTION_HOVER_MOVE
                && !connection->inputState.isHovering(
                        motionEntry->deviceId, motionEntry->source)) {
#if DEBUG_DISPATCH_CYCLE
        ALOGD("channel '%s' ~ enqueueDispatchEntryLocked: filling in missing hover enter event",
                connection->getInputChannelName());
#endif
            dispatchEntry->resolvedAction = AMOTION_EVENT_ACTION_HOVER_ENTER;
        }

        dispatchEntry->resolvedFlags = motionEntry->flags;
        if (dispatchEntry->targetFlags & InputTarget::FLAG_WINDOW_IS_OBSCURED) {
            dispatchEntry->resolvedFlags |= AMOTION_EVENT_FLAG_WINDOW_IS_OBSCURED;
        }

        if (!connection->inputState.trackMotion(motionEntry,
                dispatchEntry->resolvedAction, dispatchEntry->resolvedFlags)) {
#if DEBUG_DISPATCH_CYCLE
            ALOGD("channel '%s' ~ enqueueDispatchEntryLocked: skipping inconsistent motion event",
                    connection->getInputChannelName());
#endif
            return; 
        }
        break;
    }
    }

    
    connection->outboundQueue.enqueueAtTail(dispatchEntry);
}

void InputDispatcher::startDispatchCycleLocked(nsecs_t currentTime,
        const sp<Connection>& connection) {
#if DEBUG_DISPATCH_CYCLE
    ALOGD("channel '%s' ~ startDispatchCycle",
            connection->getInputChannelName());
#endif

    ALOG_ASSERT(connection->status == Connection::STATUS_NORMAL);
    ALOG_ASSERT(! connection->outboundQueue.isEmpty());

    DispatchEntry* dispatchEntry = connection->outboundQueue.head;
    ALOG_ASSERT(! dispatchEntry->inProgress);

    
    dispatchEntry->inProgress = true;

    
    status_t status;
    EventEntry* eventEntry = dispatchEntry->eventEntry;
    switch (eventEntry->type) {
    case EventEntry::TYPE_KEY: {
        KeyEntry* keyEntry = static_cast<KeyEntry*>(eventEntry);

        
        status = connection->inputPublisher.publishKeyEvent(
                keyEntry->deviceId, keyEntry->source,
                dispatchEntry->resolvedAction, dispatchEntry->resolvedFlags,
                keyEntry->keyCode, keyEntry->scanCode,
                keyEntry->metaState, keyEntry->repeatCount, keyEntry->downTime,
                keyEntry->eventTime);

        if (status) {
            ALOGE("channel '%s' ~ Could not publish key event, "
                    "status=%d", connection->getInputChannelName(), status);
            abortBrokenDispatchCycleLocked(currentTime, connection, true );
            return;
        }
        break;
    }

    case EventEntry::TYPE_MOTION: {
        MotionEntry* motionEntry = static_cast<MotionEntry*>(eventEntry);

        
        
        
        
        MotionSample* firstMotionSample = dispatchEntry->headMotionSample;
        if (! firstMotionSample) {
            firstMotionSample = & motionEntry->firstSample;
        }

        PointerCoords scaledCoords[MAX_POINTERS];
        const PointerCoords* usingCoords = firstMotionSample->pointerCoords;

        
        float xOffset, yOffset, scaleFactor;
        if (motionEntry->source & AINPUT_SOURCE_CLASS_POINTER
                && !(dispatchEntry->targetFlags & InputTarget::FLAG_ZERO_COORDS)) {
            scaleFactor = dispatchEntry->scaleFactor;
            xOffset = dispatchEntry->xOffset * scaleFactor;
            yOffset = dispatchEntry->yOffset * scaleFactor;
            if (scaleFactor != 1.0f) {
                for (size_t i = 0; i < motionEntry->pointerCount; i++) {
                    scaledCoords[i] = firstMotionSample->pointerCoords[i];
                    scaledCoords[i].scale(scaleFactor);
                }
                usingCoords = scaledCoords;
            }
        } else {
            xOffset = 0.0f;
            yOffset = 0.0f;
            scaleFactor = 1.0f;

            
            if (dispatchEntry->targetFlags & InputTarget::FLAG_ZERO_COORDS) {
                for (size_t i = 0; i < motionEntry->pointerCount; i++) {
                    scaledCoords[i].clear();
                }
                usingCoords = scaledCoords;
            }
        }

        
        status = connection->inputPublisher.publishMotionEvent(
                motionEntry->deviceId, motionEntry->source,
                dispatchEntry->resolvedAction, dispatchEntry->resolvedFlags,
                motionEntry->edgeFlags, motionEntry->metaState, motionEntry->buttonState,
                xOffset, yOffset,
                motionEntry->xPrecision, motionEntry->yPrecision,
                motionEntry->downTime, firstMotionSample->eventTime,
                motionEntry->pointerCount, motionEntry->pointerProperties,
                usingCoords);

        if (status) {
            ALOGE("channel '%s' ~ Could not publish motion event, "
                    "status=%d", connection->getInputChannelName(), status);
            abortBrokenDispatchCycleLocked(currentTime, connection, true );
            return;
        }

        if (dispatchEntry->resolvedAction == AMOTION_EVENT_ACTION_MOVE
                || dispatchEntry->resolvedAction == AMOTION_EVENT_ACTION_HOVER_MOVE) {
            
            MotionSample* nextMotionSample = firstMotionSample->next;
            for (; nextMotionSample != NULL; nextMotionSample = nextMotionSample->next) {
                if (usingCoords == scaledCoords) {
                    if (!(dispatchEntry->targetFlags & InputTarget::FLAG_ZERO_COORDS)) {
                        for (size_t i = 0; i < motionEntry->pointerCount; i++) {
                            scaledCoords[i] = nextMotionSample->pointerCoords[i];
                            scaledCoords[i].scale(scaleFactor);
                        }
                    }
                } else {
                    usingCoords = nextMotionSample->pointerCoords;
                }
                status = connection->inputPublisher.appendMotionSample(
                        nextMotionSample->eventTime, usingCoords);
                if (status == NO_MEMORY) {
#if DEBUG_DISPATCH_CYCLE
                    ALOGD("channel '%s' ~ Shared memory buffer full.  Some motion samples will "
                            "be sent in the next dispatch cycle.",
                            connection->getInputChannelName());
#endif
                    break;
                }
                if (status != OK) {
                    ALOGE("channel '%s' ~ Could not append motion sample "
                            "for a reason other than out of memory, status=%d",
                            connection->getInputChannelName(), status);
                    abortBrokenDispatchCycleLocked(currentTime, connection, true );
                    return;
                }
            }

            
            
            dispatchEntry->tailMotionSample = nextMotionSample;
        }
        break;
    }

    default: {
        ALOG_ASSERT(false);
    }
    }

    
    status = connection->inputPublisher.sendDispatchSignal();
    if (status) {
        ALOGE("channel '%s' ~ Could not send dispatch signal, status=%d",
                connection->getInputChannelName(), status);
        abortBrokenDispatchCycleLocked(currentTime, connection, true );
        return;
    }

    
    connection->lastEventTime = eventEntry->eventTime;
    connection->lastDispatchTime = currentTime;

    
    onDispatchCycleStartedLocked(currentTime, connection);
}

void InputDispatcher::finishDispatchCycleLocked(nsecs_t currentTime,
        const sp<Connection>& connection, bool handled) {
#if DEBUG_DISPATCH_CYCLE
    ALOGD("channel '%s' ~ finishDispatchCycle - %01.1fms since event, "
            "%01.1fms since dispatch, handled=%s",
            connection->getInputChannelName(),
            connection->getEventLatencyMillis(currentTime),
            connection->getDispatchLatencyMillis(currentTime),
            toString(handled));
#endif

    if (connection->status == Connection::STATUS_BROKEN
            || connection->status == Connection::STATUS_ZOMBIE) {
        return;
    }

    
    
    
    status_t status = connection->inputPublisher.reset();
    if (status) {
        ALOGE("channel '%s' ~ Could not reset publisher, status=%d",
                connection->getInputChannelName(), status);
        abortBrokenDispatchCycleLocked(currentTime, connection, true );
        return;
    }

    
    onDispatchCycleFinishedLocked(currentTime, connection, handled);
}

void InputDispatcher::startNextDispatchCycleLocked(nsecs_t currentTime,
        const sp<Connection>& connection) {
    
    while (! connection->outboundQueue.isEmpty()) {
        DispatchEntry* dispatchEntry = connection->outboundQueue.head;
        if (dispatchEntry->inProgress) {
             
            if (dispatchEntry->tailMotionSample) {
                
                
                dispatchEntry->inProgress = false;
                dispatchEntry->headMotionSample = dispatchEntry->tailMotionSample;
                dispatchEntry->tailMotionSample = NULL;
                startDispatchCycleLocked(currentTime, connection);
                return;
            }
            
            connection->outboundQueue.dequeueAtHead();
            if (dispatchEntry->hasForegroundTarget()) {
                decrementPendingForegroundDispatchesLocked(dispatchEntry->eventEntry);
            }
            delete dispatchEntry;
        } else {
            
            
            
            startDispatchCycleLocked(currentTime, connection);
            return;
        }
    }

    
    deactivateConnectionLocked(connection.get());
}

void InputDispatcher::abortBrokenDispatchCycleLocked(nsecs_t currentTime,
        const sp<Connection>& connection, bool notify) {
#if DEBUG_DISPATCH_CYCLE
    ALOGD("channel '%s' ~ abortBrokenDispatchCycle - notify=%s",
            connection->getInputChannelName(), toString(notify));
#endif

    
    drainOutboundQueueLocked(connection.get());

    
    
    if (connection->status == Connection::STATUS_NORMAL) {
        connection->status = Connection::STATUS_BROKEN;

        if (notify) {
            
            onDispatchCycleBrokenLocked(currentTime, connection);
        }
    }
}

void InputDispatcher::drainOutboundQueueLocked(Connection* connection) {
    while (! connection->outboundQueue.isEmpty()) {
        DispatchEntry* dispatchEntry = connection->outboundQueue.dequeueAtHead();
        if (dispatchEntry->hasForegroundTarget()) {
            decrementPendingForegroundDispatchesLocked(dispatchEntry->eventEntry);
        }
        delete dispatchEntry;
    }

    deactivateConnectionLocked(connection);
}

int InputDispatcher::handleReceiveCallback(int receiveFd, int events, void* data) {
    InputDispatcher* d = static_cast<InputDispatcher*>(data);

    { 
        AutoMutex _l(d->mLock);

        ssize_t connectionIndex = d->mConnectionsByReceiveFd.indexOfKey(receiveFd);
        if (connectionIndex < 0) {
            ALOGE("Received spurious receive callback for unknown input channel.  "
                    "fd=%d, events=0x%x", receiveFd, events);
            return 0; 
        }

        bool notify;
        sp<Connection> connection = d->mConnectionsByReceiveFd.valueAt(connectionIndex);
        if (!(events & (ALOOPER_EVENT_ERROR | ALOOPER_EVENT_HANGUP))) {
            if (!(events & ALOOPER_EVENT_INPUT)) {
                ALOGW("channel '%s' ~ Received spurious callback for unhandled poll event.  "
                        "events=0x%x", connection->getInputChannelName(), events);
                return 1;
            }

            bool handled = false;
            status_t status = connection->inputPublisher.receiveFinishedSignal(&handled);
            if (!status) {
                nsecs_t currentTime = now();
                d->finishDispatchCycleLocked(currentTime, connection, handled);
                d->runCommandsLockedInterruptible();
                return 1;
            }

            ALOGE("channel '%s' ~ Failed to receive finished signal.  status=%d",
                    connection->getInputChannelName(), status);
            notify = true;
        } else {
            
            
            
            notify = !connection->monitor;
            if (notify) {
                ALOGW("channel '%s' ~ Consumer closed input channel or an error occurred.  "
                        "events=0x%x", connection->getInputChannelName(), events);
            }
        }

        
        d->unregisterInputChannelLocked(connection->inputChannel, notify);
        return 0; 
    } 
}

void InputDispatcher::synthesizeCancelationEventsForAllConnectionsLocked(
        const CancelationOptions& options) {
    for (size_t i = 0; i < mConnectionsByReceiveFd.size(); i++) {
        synthesizeCancelationEventsForConnectionLocked(
                mConnectionsByReceiveFd.valueAt(i), options);
    }
}

void InputDispatcher::synthesizeCancelationEventsForInputChannelLocked(
        const sp<InputChannel>& channel, const CancelationOptions& options) {
    ssize_t index = getConnectionIndexLocked(channel);
    if (index >= 0) {
        synthesizeCancelationEventsForConnectionLocked(
                mConnectionsByReceiveFd.valueAt(index), options);
    }
}

void InputDispatcher::synthesizeCancelationEventsForConnectionLocked(
        const sp<Connection>& connection, const CancelationOptions& options) {
    nsecs_t currentTime = now();

    mTempCancelationEvents.clear();
    connection->inputState.synthesizeCancelationEvents(currentTime,
            mTempCancelationEvents, options);

    if (! mTempCancelationEvents.isEmpty()
            && connection->status != Connection::STATUS_BROKEN) {
#if DEBUG_OUTBOUND_EVENT_DETAILS
        ALOGD("channel '%s' ~ Synthesized %d cancelation events to bring channel back in sync "
                "with reality: %s, mode=%d.",
                connection->getInputChannelName(), mTempCancelationEvents.size(),
                options.reason, options.mode);
#endif
        for (size_t i = 0; i < mTempCancelationEvents.size(); i++) {
            EventEntry* cancelationEventEntry = mTempCancelationEvents.itemAt(i);
            switch (cancelationEventEntry->type) {
            case EventEntry::TYPE_KEY:
                logOutboundKeyDetailsLocked("cancel - ",
                        static_cast<KeyEntry*>(cancelationEventEntry));
                break;
            case EventEntry::TYPE_MOTION:
                logOutboundMotionDetailsLocked("cancel - ",
                        static_cast<MotionEntry*>(cancelationEventEntry));
                break;
            }

            InputTarget target;
            sp<InputWindowHandle> windowHandle = getWindowHandleLocked(connection->inputChannel);
            if (windowHandle != NULL) {
                const InputWindowInfo* windowInfo = windowHandle->getInfo();
                target.xOffset = -windowInfo->frameLeft;
                target.yOffset = -windowInfo->frameTop;
                target.scaleFactor = windowInfo->scaleFactor;
            } else {
                target.xOffset = 0;
                target.yOffset = 0;
                target.scaleFactor = 1.0f;
            }
            target.inputChannel = connection->inputChannel;
            target.flags = InputTarget::FLAG_DISPATCH_AS_IS;

            enqueueDispatchEntryLocked(connection, cancelationEventEntry, 
                    &target, false, InputTarget::FLAG_DISPATCH_AS_IS);

            cancelationEventEntry->release();
        }

        if (!connection->outboundQueue.head->inProgress) {
            startDispatchCycleLocked(currentTime, connection);
        }
    }
}

InputDispatcher::MotionEntry*
InputDispatcher::splitMotionEvent(const MotionEntry* originalMotionEntry, BitSet32 pointerIds) {
    ALOG_ASSERT(pointerIds.value != 0);

    uint32_t splitPointerIndexMap[MAX_POINTERS];
    PointerProperties splitPointerProperties[MAX_POINTERS];
    PointerCoords splitPointerCoords[MAX_POINTERS];

    uint32_t originalPointerCount = originalMotionEntry->pointerCount;
    uint32_t splitPointerCount = 0;

    for (uint32_t originalPointerIndex = 0; originalPointerIndex < originalPointerCount;
            originalPointerIndex++) {
        const PointerProperties& pointerProperties =
                originalMotionEntry->pointerProperties[originalPointerIndex];
        uint32_t pointerId = uint32_t(pointerProperties.id);
        if (pointerIds.hasBit(pointerId)) {
            splitPointerIndexMap[splitPointerCount] = originalPointerIndex;
            splitPointerProperties[splitPointerCount].copyFrom(pointerProperties);
            splitPointerCoords[splitPointerCount].copyFrom(
                    originalMotionEntry->firstSample.pointerCoords[originalPointerIndex]);
            splitPointerCount += 1;
        }
    }

    if (splitPointerCount != pointerIds.count()) {
        
        
        
        
        
        ALOGW("Dropping split motion event because the pointer count is %d but "
                "we expected there to be %d pointers.  This probably means we received "
                "a broken sequence of pointer ids from the input device.",
                splitPointerCount, pointerIds.count());
        return NULL;
    }

    int32_t action = originalMotionEntry->action;
    int32_t maskedAction = action & AMOTION_EVENT_ACTION_MASK;
    if (maskedAction == AMOTION_EVENT_ACTION_POINTER_DOWN
            || maskedAction == AMOTION_EVENT_ACTION_POINTER_UP) {
        int32_t originalPointerIndex = getMotionEventActionPointerIndex(action);
        const PointerProperties& pointerProperties =
                originalMotionEntry->pointerProperties[originalPointerIndex];
        uint32_t pointerId = uint32_t(pointerProperties.id);
        if (pointerIds.hasBit(pointerId)) {
            if (pointerIds.count() == 1) {
                
                action = maskedAction == AMOTION_EVENT_ACTION_POINTER_DOWN
                        ? AMOTION_EVENT_ACTION_DOWN : AMOTION_EVENT_ACTION_UP;
            } else {
                
                uint32_t splitPointerIndex = 0;
                while (pointerId != uint32_t(splitPointerProperties[splitPointerIndex].id)) {
                    splitPointerIndex += 1;
                }
                action = maskedAction | (splitPointerIndex
                        << AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
            }
        } else {
            
            action = AMOTION_EVENT_ACTION_MOVE;
        }
    }

    MotionEntry* splitMotionEntry = new MotionEntry(
            originalMotionEntry->eventTime,
            originalMotionEntry->deviceId,
            originalMotionEntry->source,
            originalMotionEntry->policyFlags,
            action,
            originalMotionEntry->flags,
            originalMotionEntry->metaState,
            originalMotionEntry->buttonState,
            originalMotionEntry->edgeFlags,
            originalMotionEntry->xPrecision,
            originalMotionEntry->yPrecision,
            originalMotionEntry->downTime,
            splitPointerCount, splitPointerProperties, splitPointerCoords);

    for (MotionSample* originalMotionSample = originalMotionEntry->firstSample.next;
            originalMotionSample != NULL; originalMotionSample = originalMotionSample->next) {
        for (uint32_t splitPointerIndex = 0; splitPointerIndex < splitPointerCount;
                splitPointerIndex++) {
            uint32_t originalPointerIndex = splitPointerIndexMap[splitPointerIndex];
            splitPointerCoords[splitPointerIndex].copyFrom(
                    originalMotionSample->pointerCoords[originalPointerIndex]);
        }

        splitMotionEntry->appendSample(originalMotionSample->eventTime, splitPointerCoords);
    }

    if (originalMotionEntry->injectionState) {
        splitMotionEntry->injectionState = originalMotionEntry->injectionState;
        splitMotionEntry->injectionState->refCount += 1;
    }

    return splitMotionEntry;
}

void InputDispatcher::notifyConfigurationChanged(const NotifyConfigurationChangedArgs* args) {
#if DEBUG_INBOUND_EVENT_DETAILS
    ALOGD("notifyConfigurationChanged - eventTime=%lld", args->eventTime);
#endif

    bool needWake;
    { 
        AutoMutex _l(mLock);

        ConfigurationChangedEntry* newEntry = new ConfigurationChangedEntry(args->eventTime);
        needWake = enqueueInboundEventLocked(newEntry);
    } 

    if (needWake) {
        mLooper->wake();
    }
}

void InputDispatcher::notifyKey(const NotifyKeyArgs* args) {
#if DEBUG_INBOUND_EVENT_DETAILS
    ALOGD("notifyKey - eventTime=%lld, deviceId=%d, source=0x%x, policyFlags=0x%x, action=0x%x, "
            "flags=0x%x, keyCode=0x%x, scanCode=0x%x, metaState=0x%x, downTime=%lld",
            args->eventTime, args->deviceId, args->source, args->policyFlags,
            args->action, args->flags, args->keyCode, args->scanCode,
            args->metaState, args->downTime);
#endif
    if (!validateKeyEvent(args->action)) {
        return;
    }

    uint32_t policyFlags = args->policyFlags;
    int32_t flags = args->flags;
    int32_t metaState = args->metaState;
    if ((policyFlags & POLICY_FLAG_VIRTUAL) || (flags & AKEY_EVENT_FLAG_VIRTUAL_HARD_KEY)) {
        policyFlags |= POLICY_FLAG_VIRTUAL;
        flags |= AKEY_EVENT_FLAG_VIRTUAL_HARD_KEY;
    }
    if (policyFlags & POLICY_FLAG_ALT) {
        metaState |= AMETA_ALT_ON | AMETA_ALT_LEFT_ON;
    }
    if (policyFlags & POLICY_FLAG_ALT_GR) {
        metaState |= AMETA_ALT_ON | AMETA_ALT_RIGHT_ON;
    }
    if (policyFlags & POLICY_FLAG_SHIFT) {
        metaState |= AMETA_SHIFT_ON | AMETA_SHIFT_LEFT_ON;
    }
    if (policyFlags & POLICY_FLAG_CAPS_LOCK) {
        metaState |= AMETA_CAPS_LOCK_ON;
    }
    if (policyFlags & POLICY_FLAG_FUNCTION) {
        metaState |= AMETA_FUNCTION_ON;
    }

    policyFlags |= POLICY_FLAG_TRUSTED;

    KeyEvent event;
    event.initialize(args->deviceId, args->source, args->action,
            flags, args->keyCode, args->scanCode, metaState, 0,
            args->downTime, args->eventTime);

    mPolicy->interceptKeyBeforeQueueing(&event,  policyFlags);

    if (policyFlags & POLICY_FLAG_WOKE_HERE) {
        flags |= AKEY_EVENT_FLAG_WOKE_HERE;
    }

    bool needWake;
    { 
        mLock.lock();

        if (mInputFilterEnabled) {
            mLock.unlock();

            policyFlags |= POLICY_FLAG_FILTERED;
            if (!mPolicy->filterInputEvent(&event, policyFlags)) {
                return; 
            }

            mLock.lock();
        }

        int32_t repeatCount = 0;
        KeyEntry* newEntry = new KeyEntry(args->eventTime,
                args->deviceId, args->source, policyFlags,
                args->action, flags, args->keyCode, args->scanCode,
                metaState, repeatCount, args->downTime);

        needWake = enqueueInboundEventLocked(newEntry);
        mLock.unlock();
    } 

    if (needWake) {
        mLooper->wake();
    }
}

void InputDispatcher::notifyMotion(const NotifyMotionArgs* args) {
#if DEBUG_INBOUND_EVENT_DETAILS
    ALOGD("notifyMotion - eventTime=%lld, deviceId=%d, source=0x%x, policyFlags=0x%x, "
            "action=0x%x, flags=0x%x, metaState=0x%x, buttonState=0x%x, edgeFlags=0x%x, "
            "xPrecision=%f, yPrecision=%f, downTime=%lld",
            args->eventTime, args->deviceId, args->source, args->policyFlags,
            args->action, args->flags, args->metaState, args->buttonState,
            args->edgeFlags, args->xPrecision, args->yPrecision, args->downTime);
    for (uint32_t i = 0; i < args->pointerCount; i++) {
        ALOGD("  Pointer %d: id=%d, toolType=%d, "
                "x=%f, y=%f, pressure=%f, size=%f, "
                "touchMajor=%f, touchMinor=%f, toolMajor=%f, toolMinor=%f, "
                "orientation=%f",
                i, args->pointerProperties[i].id,
                args->pointerProperties[i].toolType,
                args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_X),
                args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_Y),
                args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_PRESSURE),
                args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_SIZE),
                args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_TOUCH_MAJOR),
                args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_TOUCH_MINOR),
                args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_TOOL_MAJOR),
                args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_TOOL_MINOR),
                args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_ORIENTATION));
    }
#endif
    if (!validateMotionEvent(args->action, args->pointerCount, args->pointerProperties)) {
        return;
    }

    uint32_t policyFlags = args->policyFlags;
    policyFlags |= POLICY_FLAG_TRUSTED;
    mPolicy->interceptMotionBeforeQueueing(args->eventTime,  policyFlags);

    bool needWake;
    { 
        mLock.lock();

        if (mInputFilterEnabled) {
            mLock.unlock();

            MotionEvent event;
            event.initialize(args->deviceId, args->source, args->action, args->flags,
                    args->edgeFlags, args->metaState, args->buttonState, 0, 0,
                    args->xPrecision, args->yPrecision,
                    args->downTime, args->eventTime,
                    args->pointerCount, args->pointerProperties, args->pointerCoords);

            policyFlags |= POLICY_FLAG_FILTERED;
            if (!mPolicy->filterInputEvent(&event, policyFlags)) {
                return; 
            }

            mLock.lock();
        }

        
        if (args->action == AMOTION_EVENT_ACTION_MOVE
                || args->action == AMOTION_EVENT_ACTION_HOVER_MOVE) {
            
            
            
            
            
            for (EventEntry* entry = mInboundQueue.tail; entry; entry = entry->prev) {
                if (entry->type != EventEntry::TYPE_MOTION) {
                    
                    continue;
                }

                MotionEntry* motionEntry = static_cast<MotionEntry*>(entry);
                if (motionEntry->deviceId != args->deviceId
                        || motionEntry->source != args->source) {
                    
                    continue;
                }

                if (!motionEntry->canAppendSamples(args->action,
                        args->pointerCount, args->pointerProperties)) {
                    
                    
                    goto NoBatchingOrStreaming;
                }

                
                batchMotionLocked(motionEntry, args->eventTime,
                        args->metaState, args->pointerCoords,
                        "most recent motion event for this device and source in the inbound queue");
                mLock.unlock();
                return; 
            }

            
            
            
            
            
            if (mPendingEvent
                    && (!mPendingEvent->dispatchInProgress || !mCurrentInputTargetsValid)
                    && mPendingEvent->type == EventEntry::TYPE_MOTION) {
                MotionEntry* motionEntry = static_cast<MotionEntry*>(mPendingEvent);
                if (motionEntry->deviceId == args->deviceId
                        && motionEntry->source == args->source) {
                    if (!motionEntry->canAppendSamples(args->action,
                            args->pointerCount, args->pointerProperties)) {
                        
                        
                        goto NoBatchingOrStreaming;
                    }

                    
                    batchMotionLocked(motionEntry, args->eventTime,
                            args->metaState, args->pointerCoords,
                            "pending motion event");
                    mLock.unlock();
                    return; 
                }
            }

            
            
            
            
            
            
            
            
            if (mCurrentInputTargetsValid) {
                for (size_t i = 0; i < mCurrentInputTargets.size(); i++) {
                    const InputTarget& inputTarget = mCurrentInputTargets[i];
                    if ((inputTarget.flags & InputTarget::FLAG_FOREGROUND) == 0) {
                        
                        
                        continue;
                    }

                    ssize_t connectionIndex = getConnectionIndexLocked(inputTarget.inputChannel);
                    if (connectionIndex < 0) {
                        
                        continue;
                    }

                    sp<Connection> connection = mConnectionsByReceiveFd.valueAt(connectionIndex);
                    if (connection->outboundQueue.isEmpty()) {
                        
                        continue;
                    }

                    DispatchEntry* dispatchEntry = connection->outboundQueue.head;
                    if (! dispatchEntry->inProgress
                            || dispatchEntry->eventEntry->type != EventEntry::TYPE_MOTION
                            || dispatchEntry->isSplit()) {
                        
                        
                        continue;
                    }

                    MotionEntry* motionEntry = static_cast<MotionEntry*>(
                            dispatchEntry->eventEntry);
                    if (motionEntry->action != args->action
                            || motionEntry->deviceId != args->deviceId
                            || motionEntry->source != args->source
                            || motionEntry->pointerCount != args->pointerCount
                            || motionEntry->isInjected()) {
                        
                        continue;
                    }

                    if (args->action == AMOTION_EVENT_ACTION_HOVER_MOVE) {
                        if (mLastHoverWindowHandle == NULL) {
#if DEBUG_BATCHING
                            ALOGD("Not streaming hover move because there is no "
                                    "last hovered window.");
#endif
                            goto NoBatchingOrStreaming;
                        }

                        sp<InputWindowHandle> hoverWindowHandle = findTouchedWindowAtLocked(
                                args->pointerCoords[0].getAxisValue(AMOTION_EVENT_AXIS_X),
                                args->pointerCoords[0].getAxisValue(AMOTION_EVENT_AXIS_Y));
                        if (mLastHoverWindowHandle != hoverWindowHandle) {
#if DEBUG_BATCHING
                            ALOGD("Not streaming hover move because the last hovered window "
                                    "is '%s' but the currently hovered window is '%s'.",
                                    mLastHoverWindowHandle->getName().string(),
                                    hoverWindowHandle != NULL
                                            ? hoverWindowHandle->getName().string() : "<null>");
#endif
                            goto NoBatchingOrStreaming;
                        }
                    }

                    
                    
                    motionEntry->appendSample(args->eventTime, args->pointerCoords);
#if DEBUG_BATCHING
                    ALOGD("Appended motion sample onto batch for most recently dispatched "
                            "motion event for this device and source in the outbound queues.  "
                            "Attempting to stream the motion sample.");
#endif
                    nsecs_t currentTime = now();
                    dispatchEventToCurrentInputTargetsLocked(currentTime, motionEntry,
                            true );

                    runCommandsLockedInterruptible();
                    mLock.unlock();
                    return; 
                }
            }

NoBatchingOrStreaming:;
        }

        
        MotionEntry* newEntry = new MotionEntry(args->eventTime,
                args->deviceId, args->source, policyFlags,
                args->action, args->flags, args->metaState, args->buttonState,
                args->edgeFlags, args->xPrecision, args->yPrecision, args->downTime,
                args->pointerCount, args->pointerProperties, args->pointerCoords);

        needWake = enqueueInboundEventLocked(newEntry);
        mLock.unlock();
    } 

    if (needWake) {
        mLooper->wake();
    }
}

void InputDispatcher::batchMotionLocked(MotionEntry* entry, nsecs_t eventTime,
        int32_t metaState, const PointerCoords* pointerCoords, const char* eventDescription) {
    
    entry->metaState |= metaState;

    
    
    MotionSample* lastSample = entry->lastSample;
    long interval = eventTime - lastSample->eventTimeBeforeCoalescing;
    if (interval <= MOTION_SAMPLE_COALESCE_INTERVAL) {
        uint32_t pointerCount = entry->pointerCount;
        for (uint32_t i = 0; i < pointerCount; i++) {
            lastSample->pointerCoords[i].copyFrom(pointerCoords[i]);
        }
        lastSample->eventTime = eventTime;
#if DEBUG_BATCHING
        ALOGD("Coalesced motion into last sample of batch for %s, events were %0.3f ms apart",
                eventDescription, interval * 0.000001f);
#endif
        return;
    }

    
    entry->appendSample(eventTime, pointerCoords);
#if DEBUG_BATCHING
    ALOGD("Appended motion sample onto batch for %s, events were %0.3f ms apart",
            eventDescription, interval * 0.000001f);
#endif
}

void InputDispatcher::notifySwitch(const NotifySwitchArgs* args) {
#if DEBUG_INBOUND_EVENT_DETAILS
    ALOGD("notifySwitch - eventTime=%lld, policyFlags=0x%x, switchCode=%d, switchValue=%d",
            args->eventTime, args->policyFlags,
            args->switchCode, args->switchValue);
#endif

    uint32_t policyFlags = args->policyFlags;
    policyFlags |= POLICY_FLAG_TRUSTED;
    mPolicy->notifySwitch(args->eventTime,
            args->switchCode, args->switchValue, policyFlags);
}

void InputDispatcher::notifyDeviceReset(const NotifyDeviceResetArgs* args) {
#if DEBUG_INBOUND_EVENT_DETAILS
    ALOGD("notifyDeviceReset - eventTime=%lld, deviceId=%d",
            args->eventTime, args->deviceId);
#endif

    bool needWake;
    { 
        AutoMutex _l(mLock);

        DeviceResetEntry* newEntry = new DeviceResetEntry(args->eventTime, args->deviceId);
        needWake = enqueueInboundEventLocked(newEntry);
    } 

    if (needWake) {
        mLooper->wake();
    }
}

int32_t InputDispatcher::injectInputEvent(const InputEvent* event,
        int32_t injectorPid, int32_t injectorUid, int32_t syncMode, int32_t timeoutMillis,
        uint32_t policyFlags) {
#if DEBUG_INBOUND_EVENT_DETAILS
    ALOGD("injectInputEvent - eventType=%d, injectorPid=%d, injectorUid=%d, "
            "syncMode=%d, timeoutMillis=%d, policyFlags=0x%08x",
            event->getType(), injectorPid, injectorUid, syncMode, timeoutMillis, policyFlags);
#endif

    nsecs_t endTime = now() + milliseconds_to_nanoseconds(timeoutMillis);

    policyFlags |= POLICY_FLAG_INJECTED;
    if (hasInjectionPermission(injectorPid, injectorUid)) {
        policyFlags |= POLICY_FLAG_TRUSTED;
    }

    EventEntry* injectedEntry;
    switch (event->getType()) {
    case AINPUT_EVENT_TYPE_KEY: {
        const KeyEvent* keyEvent = static_cast<const KeyEvent*>(event);
        int32_t action = keyEvent->getAction();
        if (! validateKeyEvent(action)) {
            return INPUT_EVENT_INJECTION_FAILED;
        }

        int32_t flags = keyEvent->getFlags();
        if (flags & AKEY_EVENT_FLAG_VIRTUAL_HARD_KEY) {
            policyFlags |= POLICY_FLAG_VIRTUAL;
        }

        if (!(policyFlags & POLICY_FLAG_FILTERED)) {
            mPolicy->interceptKeyBeforeQueueing(keyEvent,  policyFlags);
        }

        if (policyFlags & POLICY_FLAG_WOKE_HERE) {
            flags |= AKEY_EVENT_FLAG_WOKE_HERE;
        }

        mLock.lock();
        injectedEntry = new KeyEntry(keyEvent->getEventTime(),
                keyEvent->getDeviceId(), keyEvent->getSource(),
                policyFlags, action, flags,
                keyEvent->getKeyCode(), keyEvent->getScanCode(), keyEvent->getMetaState(),
                keyEvent->getRepeatCount(), keyEvent->getDownTime());
        break;
    }

    case AINPUT_EVENT_TYPE_MOTION: {
        const MotionEvent* motionEvent = static_cast<const MotionEvent*>(event);
        int32_t action = motionEvent->getAction();
        size_t pointerCount = motionEvent->getPointerCount();
        const PointerProperties* pointerProperties = motionEvent->getPointerProperties();
        if (! validateMotionEvent(action, pointerCount, pointerProperties)) {
            return INPUT_EVENT_INJECTION_FAILED;
        }

        if (!(policyFlags & POLICY_FLAG_FILTERED)) {
            nsecs_t eventTime = motionEvent->getEventTime();
            mPolicy->interceptMotionBeforeQueueing(eventTime,  policyFlags);
        }

        mLock.lock();
        const nsecs_t* sampleEventTimes = motionEvent->getSampleEventTimes();
        const PointerCoords* samplePointerCoords = motionEvent->getSamplePointerCoords();
        MotionEntry* motionEntry = new MotionEntry(*sampleEventTimes,
                motionEvent->getDeviceId(), motionEvent->getSource(), policyFlags,
                action, motionEvent->getFlags(),
                motionEvent->getMetaState(), motionEvent->getButtonState(),
                motionEvent->getEdgeFlags(),
                motionEvent->getXPrecision(), motionEvent->getYPrecision(),
                motionEvent->getDownTime(), uint32_t(pointerCount),
                pointerProperties, samplePointerCoords);
        for (size_t i = motionEvent->getHistorySize(); i > 0; i--) {
            sampleEventTimes += 1;
            samplePointerCoords += pointerCount;
            motionEntry->appendSample(*sampleEventTimes, samplePointerCoords);
        }
        injectedEntry = motionEntry;
        break;
    }

    default:
        ALOGW("Cannot inject event of type %d", event->getType());
        return INPUT_EVENT_INJECTION_FAILED;
    }

    InjectionState* injectionState = new InjectionState(injectorPid, injectorUid);
    if (syncMode == INPUT_EVENT_INJECTION_SYNC_NONE) {
        injectionState->injectionIsAsync = true;
    }

    injectionState->refCount += 1;
    injectedEntry->injectionState = injectionState;

    bool needWake = enqueueInboundEventLocked(injectedEntry);
    mLock.unlock();

    if (needWake) {
        mLooper->wake();
    }

    int32_t injectionResult;
    { 
        AutoMutex _l(mLock);

        if (syncMode == INPUT_EVENT_INJECTION_SYNC_NONE) {
            injectionResult = INPUT_EVENT_INJECTION_SUCCEEDED;
        } else {
            for (;;) {
                injectionResult = injectionState->injectionResult;
                if (injectionResult != INPUT_EVENT_INJECTION_PENDING) {
                    break;
                }

                nsecs_t remainingTimeout = endTime - now();
                if (remainingTimeout <= 0) {
#if DEBUG_INJECTION
                    ALOGD("injectInputEvent - Timed out waiting for injection result "
                            "to become available.");
#endif
                    injectionResult = INPUT_EVENT_INJECTION_TIMED_OUT;
                    break;
                }

                mInjectionResultAvailableCondition.waitRelative(mLock, remainingTimeout);
            }

            if (injectionResult == INPUT_EVENT_INJECTION_SUCCEEDED
                    && syncMode == INPUT_EVENT_INJECTION_SYNC_WAIT_FOR_FINISHED) {
                while (injectionState->pendingForegroundDispatches != 0) {
#if DEBUG_INJECTION
                    ALOGD("injectInputEvent - Waiting for %d pending foreground dispatches.",
                            injectionState->pendingForegroundDispatches);
#endif
                    nsecs_t remainingTimeout = endTime - now();
                    if (remainingTimeout <= 0) {
#if DEBUG_INJECTION
                    ALOGD("injectInputEvent - Timed out waiting for pending foreground "
                            "dispatches to finish.");
#endif
                        injectionResult = INPUT_EVENT_INJECTION_TIMED_OUT;
                        break;
                    }

                    mInjectionSyncFinishedCondition.waitRelative(mLock, remainingTimeout);
                }
            }
        }

        injectionState->release();
    } 

#if DEBUG_INJECTION
    ALOGD("injectInputEvent - Finished with result %d.  "
            "injectorPid=%d, injectorUid=%d",
            injectionResult, injectorPid, injectorUid);
#endif

    return injectionResult;
}

bool InputDispatcher::hasInjectionPermission(int32_t injectorPid, int32_t injectorUid) {
    return injectorUid == 0
            || mPolicy->checkInjectEventsPermissionNonReentrant(injectorPid, injectorUid);
}

void InputDispatcher::setInjectionResultLocked(EventEntry* entry, int32_t injectionResult) {
    InjectionState* injectionState = entry->injectionState;
    if (injectionState) {
#if DEBUG_INJECTION
        ALOGD("Setting input event injection result to %d.  "
                "injectorPid=%d, injectorUid=%d",
                 injectionResult, injectionState->injectorPid, injectionState->injectorUid);
#endif

        if (injectionState->injectionIsAsync
                && !(entry->policyFlags & POLICY_FLAG_FILTERED)) {
            
            switch (injectionResult) {
            case INPUT_EVENT_INJECTION_SUCCEEDED:
                ALOGV("Asynchronous input event injection succeeded.");
                break;
            case INPUT_EVENT_INJECTION_FAILED:
                ALOGW("Asynchronous input event injection failed.");
                break;
            case INPUT_EVENT_INJECTION_PERMISSION_DENIED:
                ALOGW("Asynchronous input event injection permission denied.");
                break;
            case INPUT_EVENT_INJECTION_TIMED_OUT:
                ALOGW("Asynchronous input event injection timed out.");
                break;
            }
        }

        injectionState->injectionResult = injectionResult;
        mInjectionResultAvailableCondition.broadcast();
    }
}

void InputDispatcher::incrementPendingForegroundDispatchesLocked(EventEntry* entry) {
    InjectionState* injectionState = entry->injectionState;
    if (injectionState) {
        injectionState->pendingForegroundDispatches += 1;
    }
}

void InputDispatcher::decrementPendingForegroundDispatchesLocked(EventEntry* entry) {
    InjectionState* injectionState = entry->injectionState;
    if (injectionState) {
        injectionState->pendingForegroundDispatches -= 1;

        if (injectionState->pendingForegroundDispatches == 0) {
            mInjectionSyncFinishedCondition.broadcast();
        }
    }
}

sp<InputWindowHandle> InputDispatcher::getWindowHandleLocked(
        const sp<InputChannel>& inputChannel) const {
    size_t numWindows = mWindowHandles.size();
    for (size_t i = 0; i < numWindows; i++) {
        const sp<InputWindowHandle>& windowHandle = mWindowHandles.itemAt(i);
        if (windowHandle->getInputChannel() == inputChannel) {
            return windowHandle;
        }
    }
    return NULL;
}

bool InputDispatcher::hasWindowHandleLocked(
        const sp<InputWindowHandle>& windowHandle) const {
    size_t numWindows = mWindowHandles.size();
    for (size_t i = 0; i < numWindows; i++) {
        if (mWindowHandles.itemAt(i) == windowHandle) {
            return true;
        }
    }
    return false;
}

void InputDispatcher::setInputWindows(const Vector<sp<InputWindowHandle> >& inputWindowHandles) {
#if DEBUG_FOCUS
    ALOGD("setInputWindows");
#endif
    { 
        AutoMutex _l(mLock);

        Vector<sp<InputWindowHandle> > oldWindowHandles = mWindowHandles;
        mWindowHandles = inputWindowHandles;

        sp<InputWindowHandle> newFocusedWindowHandle;
        bool foundHoveredWindow = false;
        for (size_t i = 0; i < mWindowHandles.size(); i++) {
            const sp<InputWindowHandle>& windowHandle = mWindowHandles.itemAt(i);
            if (!windowHandle->updateInfo() || windowHandle->getInputChannel() == NULL) {
                mWindowHandles.removeAt(i--);
                continue;
            }
            if (windowHandle->getInfo()->hasFocus) {
                newFocusedWindowHandle = windowHandle;
            }
            if (windowHandle == mLastHoverWindowHandle) {
                foundHoveredWindow = true;
            }
        }

        if (!foundHoveredWindow) {
            mLastHoverWindowHandle = NULL;
        }

        if (mFocusedWindowHandle != newFocusedWindowHandle) {
            if (mFocusedWindowHandle != NULL) {
#if DEBUG_FOCUS
                ALOGD("Focus left window: %s",
                        mFocusedWindowHandle->getName().string());
#endif
                sp<InputChannel> focusedInputChannel = mFocusedWindowHandle->getInputChannel();
                if (focusedInputChannel != NULL) {
                    CancelationOptions options(CancelationOptions::CANCEL_NON_POINTER_EVENTS,
                            "focus left window");
                    synthesizeCancelationEventsForInputChannelLocked(
                            focusedInputChannel, options);
                }
            }
            if (newFocusedWindowHandle != NULL) {
#if DEBUG_FOCUS
                ALOGD("Focus entered window: %s",
                        newFocusedWindowHandle->getName().string());
#endif
            }
            mFocusedWindowHandle = newFocusedWindowHandle;
        }

        for (size_t i = 0; i < mTouchState.windows.size(); i++) {
            TouchedWindow& touchedWindow = mTouchState.windows.editItemAt(i);
            if (!hasWindowHandleLocked(touchedWindow.windowHandle)) {
#if DEBUG_FOCUS
                ALOGD("Touched window was removed: %s",
                        touchedWindow.windowHandle->getName().string());
#endif
                sp<InputChannel> touchedInputChannel =
                        touchedWindow.windowHandle->getInputChannel();
                if (touchedInputChannel != NULL) {
                    CancelationOptions options(CancelationOptions::CANCEL_POINTER_EVENTS,
                            "touched window was removed");
                    synthesizeCancelationEventsForInputChannelLocked(
                            touchedInputChannel, options);
                }
                mTouchState.windows.removeAt(i--);
            }
        }

        
        
        
        
        for (size_t i = 0; i < oldWindowHandles.size(); i++) {
            const sp<InputWindowHandle>& oldWindowHandle = oldWindowHandles.itemAt(i);
            if (!hasWindowHandleLocked(oldWindowHandle)) {
#if DEBUG_FOCUS
                ALOGD("Window went away: %s", oldWindowHandle->getName().string());
#endif
                oldWindowHandle->releaseInfo();
            }
        }
    } 

    
    mLooper->wake();
}

void InputDispatcher::setFocusedApplication(
        const sp<InputApplicationHandle>& inputApplicationHandle) {
#if DEBUG_FOCUS
    ALOGD("setFocusedApplication");
#endif
    { 
        AutoMutex _l(mLock);

        if (inputApplicationHandle != NULL && inputApplicationHandle->updateInfo()) {
            if (mFocusedApplicationHandle != inputApplicationHandle) {
                if (mFocusedApplicationHandle != NULL) {
                    resetTargetsLocked();
                    mFocusedApplicationHandle->releaseInfo();
                }
                mFocusedApplicationHandle = inputApplicationHandle;
            }
        } else if (mFocusedApplicationHandle != NULL) {
            resetTargetsLocked();
            mFocusedApplicationHandle->releaseInfo();
            mFocusedApplicationHandle.clear();
        }

#if DEBUG_FOCUS
        
#endif
    } 

    
    mLooper->wake();
}

void InputDispatcher::setInputDispatchMode(bool enabled, bool frozen) {
#if DEBUG_FOCUS
    ALOGD("setInputDispatchMode: enabled=%d, frozen=%d", enabled, frozen);
#endif

    bool changed;
    { 
        AutoMutex _l(mLock);

        if (mDispatchEnabled != enabled || mDispatchFrozen != frozen) {
            if (mDispatchFrozen && !frozen) {
                resetANRTimeoutsLocked();
            }

            if (mDispatchEnabled && !enabled) {
                resetAndDropEverythingLocked("dispatcher is being disabled");
            }

            mDispatchEnabled = enabled;
            mDispatchFrozen = frozen;
            changed = true;
        } else {
            changed = false;
        }

#if DEBUG_FOCUS
        
#endif
    } 

    if (changed) {
        
        mLooper->wake();
    }
}

void InputDispatcher::setInputFilterEnabled(bool enabled) {
#if DEBUG_FOCUS
    ALOGD("setInputFilterEnabled: enabled=%d", enabled);
#endif

    { 
        AutoMutex _l(mLock);

        if (mInputFilterEnabled == enabled) {
            return;
        }

        mInputFilterEnabled = enabled;
        resetAndDropEverythingLocked("input filter is being enabled or disabled");
    } 

    
    mLooper->wake();
}

bool InputDispatcher::transferTouchFocus(const sp<InputChannel>& fromChannel,
        const sp<InputChannel>& toChannel) {
#if DEBUG_FOCUS
    ALOGD("transferTouchFocus: fromChannel=%s, toChannel=%s",
            fromChannel->getName().string(), toChannel->getName().string());
#endif
    { 
        AutoMutex _l(mLock);

        sp<InputWindowHandle> fromWindowHandle = getWindowHandleLocked(fromChannel);
        sp<InputWindowHandle> toWindowHandle = getWindowHandleLocked(toChannel);
        if (fromWindowHandle == NULL || toWindowHandle == NULL) {
#if DEBUG_FOCUS
            ALOGD("Cannot transfer focus because from or to window not found.");
#endif
            return false;
        }
        if (fromWindowHandle == toWindowHandle) {
#if DEBUG_FOCUS
            ALOGD("Trivial transfer to same window.");
#endif
            return true;
        }

        bool found = false;
        for (size_t i = 0; i < mTouchState.windows.size(); i++) {
            const TouchedWindow& touchedWindow = mTouchState.windows[i];
            if (touchedWindow.windowHandle == fromWindowHandle) {
                int32_t oldTargetFlags = touchedWindow.targetFlags;
                BitSet32 pointerIds = touchedWindow.pointerIds;

                mTouchState.windows.removeAt(i);

                int32_t newTargetFlags = oldTargetFlags
                        & (InputTarget::FLAG_FOREGROUND
                                | InputTarget::FLAG_SPLIT | InputTarget::FLAG_DISPATCH_AS_IS);
                mTouchState.addOrUpdateWindow(toWindowHandle, newTargetFlags, pointerIds);

                found = true;
                break;
            }
        }

        if (! found) {
#if DEBUG_FOCUS
            ALOGD("Focus transfer failed because from window did not have focus.");
#endif
            return false;
        }

        ssize_t fromConnectionIndex = getConnectionIndexLocked(fromChannel);
        ssize_t toConnectionIndex = getConnectionIndexLocked(toChannel);
        if (fromConnectionIndex >= 0 && toConnectionIndex >= 0) {
            sp<Connection> fromConnection = mConnectionsByReceiveFd.valueAt(fromConnectionIndex);
            sp<Connection> toConnection = mConnectionsByReceiveFd.valueAt(toConnectionIndex);

            fromConnection->inputState.copyPointerStateTo(toConnection->inputState);
            CancelationOptions options(CancelationOptions::CANCEL_POINTER_EVENTS,
                    "transferring touch focus from this window to another window");
            synthesizeCancelationEventsForConnectionLocked(fromConnection, options);
        }

#if DEBUG_FOCUS
        logDispatchStateLocked();
#endif
    } 

    
    mLooper->wake();
    return true;
}

void InputDispatcher::resetAndDropEverythingLocked(const char* reason) {
#if DEBUG_FOCUS
    ALOGD("Resetting and dropping all events (%s).", reason);
#endif

    CancelationOptions options(CancelationOptions::CANCEL_ALL_EVENTS, reason);
    synthesizeCancelationEventsForAllConnectionsLocked(options);

    resetKeyRepeatLocked();
    releasePendingEventLocked();
    drainInboundQueueLocked();
    resetTargetsLocked();

    mTouchState.reset();
    mLastHoverWindowHandle.clear();
}

void InputDispatcher::logDispatchStateLocked() {
    String8 dump;
    dumpDispatchStateLocked(dump);

    char* text = dump.lockBuffer(dump.size());
    char* start = text;
    while (*start != '\0') {
        char* end = strchr(start, '\n');
        if (*end == '\n') {
            *(end++) = '\0';
        }
        ALOGD("%s", start);
        start = end;
    }
}

void InputDispatcher::dumpDispatchStateLocked(String8& dump) {
    dump.appendFormat(INDENT "DispatchEnabled: %d\n", mDispatchEnabled);
    dump.appendFormat(INDENT "DispatchFrozen: %d\n", mDispatchFrozen);

    if (mFocusedApplicationHandle != NULL) {
        dump.appendFormat(INDENT "FocusedApplication: name='%s', dispatchingTimeout=%0.3fms\n",
                mFocusedApplicationHandle->getName().string(),
                mFocusedApplicationHandle->getDispatchingTimeout(
                        DEFAULT_INPUT_DISPATCHING_TIMEOUT) / 1000000.0);
    } else {
        dump.append(INDENT "FocusedApplication: <null>\n");
    }
    dump.appendFormat(INDENT "FocusedWindow: name='%s'\n",
            mFocusedWindowHandle != NULL ? mFocusedWindowHandle->getName().string() : "<null>");

    dump.appendFormat(INDENT "TouchDown: %s\n", toString(mTouchState.down));
    dump.appendFormat(INDENT "TouchSplit: %s\n", toString(mTouchState.split));
    dump.appendFormat(INDENT "TouchDeviceId: %d\n", mTouchState.deviceId);
    dump.appendFormat(INDENT "TouchSource: 0x%08x\n", mTouchState.source);
    if (!mTouchState.windows.isEmpty()) {
        dump.append(INDENT "TouchedWindows:\n");
        for (size_t i = 0; i < mTouchState.windows.size(); i++) {
            const TouchedWindow& touchedWindow = mTouchState.windows[i];
            dump.appendFormat(INDENT2 "%d: name='%s', pointerIds=0x%0x, targetFlags=0x%x\n",
                    i, touchedWindow.windowHandle->getName().string(),
                    touchedWindow.pointerIds.value,
                    touchedWindow.targetFlags);
        }
    } else {
        dump.append(INDENT "TouchedWindows: <none>\n");
    }

    if (!mWindowHandles.isEmpty()) {
        dump.append(INDENT "Windows:\n");
        for (size_t i = 0; i < mWindowHandles.size(); i++) {
            const sp<InputWindowHandle>& windowHandle = mWindowHandles.itemAt(i);
            const InputWindowInfo* windowInfo = windowHandle->getInfo();

            dump.appendFormat(INDENT2 "%d: name='%s', paused=%s, hasFocus=%s, hasWallpaper=%s, "
                    "visible=%s, canReceiveKeys=%s, flags=0x%08x, type=0x%08x, layer=%d, "
                    "frame=[%d,%d][%d,%d], scale=%f, "
                    "touchableRegion=",
                    i, windowInfo->name.string(),
                    toString(windowInfo->paused),
                    toString(windowInfo->hasFocus),
                    toString(windowInfo->hasWallpaper),
                    toString(windowInfo->visible),
                    toString(windowInfo->canReceiveKeys),
                    windowInfo->layoutParamsFlags, windowInfo->layoutParamsType,
                    windowInfo->layer,
                    windowInfo->frameLeft, windowInfo->frameTop,
                    windowInfo->frameRight, windowInfo->frameBottom,
                    windowInfo->scaleFactor);
#ifdef HAVE_ANDROID_OS
            dumpRegion(dump, windowInfo->touchableRegion);
#endif
            dump.appendFormat(", inputFeatures=0x%08x", windowInfo->inputFeatures);
            dump.appendFormat(", ownerPid=%d, ownerUid=%d, dispatchingTimeout=%0.3fms\n",
                    windowInfo->ownerPid, windowInfo->ownerUid,
                    windowInfo->dispatchingTimeout / 1000000.0);
        }
    } else {
        dump.append(INDENT "Windows: <none>\n");
    }

    if (!mMonitoringChannels.isEmpty()) {
        dump.append(INDENT "MonitoringChannels:\n");
        for (size_t i = 0; i < mMonitoringChannels.size(); i++) {
            const sp<InputChannel>& channel = mMonitoringChannels[i];
            dump.appendFormat(INDENT2 "%d: '%s'\n", i, channel->getName().string());
        }
    } else {
        dump.append(INDENT "MonitoringChannels: <none>\n");
    }

    dump.appendFormat(INDENT "InboundQueue: length=%u\n", mInboundQueue.count());

    if (!mActiveConnections.isEmpty()) {
        dump.append(INDENT "ActiveConnections:\n");
        for (size_t i = 0; i < mActiveConnections.size(); i++) {
            const Connection* connection = mActiveConnections[i];
            dump.appendFormat(INDENT2 "%d: '%s', status=%s, outboundQueueLength=%u, "
                    "inputState.isNeutral=%s\n",
                    i, connection->getInputChannelName(), connection->getStatusLabel(),
                    connection->outboundQueue.count(),
                    toString(connection->inputState.isNeutral()));
        }
    } else {
        dump.append(INDENT "ActiveConnections: <none>\n");
    }

    if (isAppSwitchPendingLocked()) {
        dump.appendFormat(INDENT "AppSwitch: pending, due in %01.1fms\n",
                (mAppSwitchDueTime - now()) / 1000000.0);
    } else {
        dump.append(INDENT "AppSwitch: not pending\n");
    }
}

status_t InputDispatcher::registerInputChannel(const sp<InputChannel>& inputChannel,
        const sp<InputWindowHandle>& inputWindowHandle, bool monitor) {
#if DEBUG_REGISTRATION
    ALOGD("channel '%s' ~ registerInputChannel - monitor=%s", inputChannel->getName().string(),
            toString(monitor));
#endif

    { 
        AutoMutex _l(mLock);

        if (getConnectionIndexLocked(inputChannel) >= 0) {
            ALOGW("Attempted to register already registered input channel '%s'",
                    inputChannel->getName().string());
            return BAD_VALUE;
        }

        sp<Connection> connection = new Connection(inputChannel, inputWindowHandle, monitor);
        status_t status = connection->initialize();
        if (status) {
            ALOGE("Failed to initialize input publisher for input channel '%s', status=%d",
                    inputChannel->getName().string(), status);
            return status;
        }

        int32_t receiveFd = inputChannel->getReceivePipeFd();
        mConnectionsByReceiveFd.add(receiveFd, connection);

        if (monitor) {
            mMonitoringChannels.push(inputChannel);
        }

        mLooper->addFd(receiveFd, 0, ALOOPER_EVENT_INPUT, handleReceiveCallback, this);

        runCommandsLockedInterruptible();
    } 
    return OK;
}

status_t InputDispatcher::unregisterInputChannel(const sp<InputChannel>& inputChannel) {
#if DEBUG_REGISTRATION
    ALOGD("channel '%s' ~ unregisterInputChannel", inputChannel->getName().string());
#endif

    { 
        AutoMutex _l(mLock);

        status_t status = unregisterInputChannelLocked(inputChannel, false );
        if (status) {
            return status;
        }
    } 

    
    
    mLooper->wake();
    return OK;
}

status_t InputDispatcher::unregisterInputChannelLocked(const sp<InputChannel>& inputChannel,
        bool notify) {
    ssize_t connectionIndex = getConnectionIndexLocked(inputChannel);
    if (connectionIndex < 0) {
        ALOGW("Attempted to unregister already unregistered input channel '%s'",
                inputChannel->getName().string());
        return BAD_VALUE;
    }

    sp<Connection> connection = mConnectionsByReceiveFd.valueAt(connectionIndex);
    mConnectionsByReceiveFd.removeItemsAt(connectionIndex);

    if (connection->monitor) {
        removeMonitorChannelLocked(inputChannel);
    }

    mLooper->removeFd(inputChannel->getReceivePipeFd());

    nsecs_t currentTime = now();
    abortBrokenDispatchCycleLocked(currentTime, connection, notify);

    runCommandsLockedInterruptible();

    connection->status = Connection::STATUS_ZOMBIE;
    return OK;
}

void InputDispatcher::removeMonitorChannelLocked(const sp<InputChannel>& inputChannel) {
    for (size_t i = 0; i < mMonitoringChannels.size(); i++) {
         if (mMonitoringChannels[i] == inputChannel) {
             mMonitoringChannels.removeAt(i);
             break;
         }
    }
}

ssize_t InputDispatcher::getConnectionIndexLocked(const sp<InputChannel>& inputChannel) {
    ssize_t connectionIndex = mConnectionsByReceiveFd.indexOfKey(inputChannel->getReceivePipeFd());
    if (connectionIndex >= 0) {
        sp<Connection> connection = mConnectionsByReceiveFd.valueAt(connectionIndex);
        if (connection->inputChannel.get() == inputChannel.get()) {
            return connectionIndex;
        }
    }

    return -1;
}

void InputDispatcher::activateConnectionLocked(Connection* connection) {
    for (size_t i = 0; i < mActiveConnections.size(); i++) {
        if (mActiveConnections.itemAt(i) == connection) {
            return;
        }
    }
    mActiveConnections.add(connection);
}

void InputDispatcher::deactivateConnectionLocked(Connection* connection) {
    for (size_t i = 0; i < mActiveConnections.size(); i++) {
        if (mActiveConnections.itemAt(i) == connection) {
            mActiveConnections.removeAt(i);
            return;
        }
    }
}

void InputDispatcher::onDispatchCycleStartedLocked(
        nsecs_t currentTime, const sp<Connection>& connection) {
}

void InputDispatcher::onDispatchCycleFinishedLocked(
        nsecs_t currentTime, const sp<Connection>& connection, bool handled) {
    CommandEntry* commandEntry = postCommandLocked(
            & InputDispatcher::doDispatchCycleFinishedLockedInterruptible);
    commandEntry->connection = connection;
    commandEntry->handled = handled;
}

void InputDispatcher::onDispatchCycleBrokenLocked(
        nsecs_t currentTime, const sp<Connection>& connection) {
    ALOGE("channel '%s' ~ Channel is unrecoverably broken and will be disposed!",
            connection->getInputChannelName());

    CommandEntry* commandEntry = postCommandLocked(
            & InputDispatcher::doNotifyInputChannelBrokenLockedInterruptible);
    commandEntry->connection = connection;
}

void InputDispatcher::onANRLocked(
        nsecs_t currentTime, const sp<InputApplicationHandle>& applicationHandle,
        const sp<InputWindowHandle>& windowHandle,
        nsecs_t eventTime, nsecs_t waitStartTime) {
    ALOGI("Application is not responding: %s.  "
            "%01.1fms since event, %01.1fms since wait started",
            getApplicationWindowLabelLocked(applicationHandle, windowHandle).string(),
            (currentTime - eventTime) / 1000000.0,
            (currentTime - waitStartTime) / 1000000.0);

    CommandEntry* commandEntry = postCommandLocked(
            & InputDispatcher::doNotifyANRLockedInterruptible);
    commandEntry->inputApplicationHandle = applicationHandle;
    commandEntry->inputWindowHandle = windowHandle;
}

void InputDispatcher::doNotifyConfigurationChangedInterruptible(
        CommandEntry* commandEntry) {
    mLock.unlock();

    mPolicy->notifyConfigurationChanged(commandEntry->eventTime);

    mLock.lock();
}

void InputDispatcher::doNotifyInputChannelBrokenLockedInterruptible(
        CommandEntry* commandEntry) {
    sp<Connection> connection = commandEntry->connection;

    if (connection->status != Connection::STATUS_ZOMBIE) {
        mLock.unlock();

        mPolicy->notifyInputChannelBroken(connection->inputWindowHandle);

        mLock.lock();
    }
}

void InputDispatcher::doNotifyANRLockedInterruptible(
        CommandEntry* commandEntry) {
    mLock.unlock();

    nsecs_t newTimeout = mPolicy->notifyANR(
            commandEntry->inputApplicationHandle, commandEntry->inputWindowHandle);

    mLock.lock();

    resumeAfterTargetsNotReadyTimeoutLocked(newTimeout,
            commandEntry->inputWindowHandle != NULL
                    ? commandEntry->inputWindowHandle->getInputChannel() : NULL);
}

void InputDispatcher::doInterceptKeyBeforeDispatchingLockedInterruptible(
        CommandEntry* commandEntry) {
    KeyEntry* entry = commandEntry->keyEntry;

    KeyEvent event;
    initializeKeyEvent(&event, entry);

    mLock.unlock();

    nsecs_t delay = mPolicy->interceptKeyBeforeDispatching(commandEntry->inputWindowHandle,
            &event, entry->policyFlags);

    mLock.lock();

    if (delay < 0) {
        entry->interceptKeyResult = KeyEntry::INTERCEPT_KEY_RESULT_SKIP;
    } else if (!delay) {
        entry->interceptKeyResult = KeyEntry::INTERCEPT_KEY_RESULT_CONTINUE;
    } else {
        entry->interceptKeyResult = KeyEntry::INTERCEPT_KEY_RESULT_TRY_AGAIN_LATER;
        entry->interceptKeyWakeupTime = now() + delay;
    }
    entry->release();
}

void InputDispatcher::doDispatchCycleFinishedLockedInterruptible(
        CommandEntry* commandEntry) {
    sp<Connection> connection = commandEntry->connection;
    bool handled = commandEntry->handled;

    bool skipNext = false;
    if (!connection->outboundQueue.isEmpty()) {
        DispatchEntry* dispatchEntry = connection->outboundQueue.head;
        if (dispatchEntry->inProgress) {
            if (dispatchEntry->eventEntry->type == EventEntry::TYPE_KEY) {
                KeyEntry* keyEntry = static_cast<KeyEntry*>(dispatchEntry->eventEntry);
                skipNext = afterKeyEventLockedInterruptible(connection,
                        dispatchEntry, keyEntry, handled);
            } else if (dispatchEntry->eventEntry->type == EventEntry::TYPE_MOTION) {
                MotionEntry* motionEntry = static_cast<MotionEntry*>(dispatchEntry->eventEntry);
                skipNext = afterMotionEventLockedInterruptible(connection,
                        dispatchEntry, motionEntry, handled);
            }
        }
    }

    if (!skipNext) {
        startNextDispatchCycleLocked(now(), connection);
    }
}

bool InputDispatcher::afterKeyEventLockedInterruptible(const sp<Connection>& connection,
        DispatchEntry* dispatchEntry, KeyEntry* keyEntry, bool handled) {
    if (!(keyEntry->flags & AKEY_EVENT_FLAG_FALLBACK)) {
        
        
        int32_t originalKeyCode = keyEntry->keyCode;
        int32_t fallbackKeyCode = connection->inputState.getFallbackKey(originalKeyCode);
        if (keyEntry->action == AKEY_EVENT_ACTION_UP) {
            connection->inputState.removeFallbackKey(originalKeyCode);
        }

        if (handled || !dispatchEntry->hasForegroundTarget()) {
            
            
            
            if (fallbackKeyCode != -1) {
                if (fallbackKeyCode != AKEYCODE_UNKNOWN) {
                    CancelationOptions options(CancelationOptions::CANCEL_FALLBACK_EVENTS,
                            "application handled the original non-fallback key "
                            "or is no longer a foreground target, "
                            "canceling previously dispatched fallback key");
                    options.keyCode = fallbackKeyCode;
                    synthesizeCancelationEventsForConnectionLocked(connection, options);
                }
                connection->inputState.removeFallbackKey(originalKeyCode);
            }
        } else {
            
            
            
            bool initialDown = keyEntry->action == AKEY_EVENT_ACTION_DOWN
                    && keyEntry->repeatCount == 0;
            if (fallbackKeyCode == -1 && !initialDown) {
#if DEBUG_OUTBOUND_EVENT_DETAILS
                ALOGD("Unhandled key event: Skipping unhandled key event processing "
                        "since this is not an initial down.  "
                        "keyCode=%d, action=%d, repeatCount=%d",
                        originalKeyCode, keyEntry->action, keyEntry->repeatCount);
#endif
                return false;
            }

            
#if DEBUG_OUTBOUND_EVENT_DETAILS
            ALOGD("Unhandled key event: Asking policy to perform fallback action.  "
                    "keyCode=%d, action=%d, repeatCount=%d",
                    keyEntry->keyCode, keyEntry->action, keyEntry->repeatCount);
#endif
            KeyEvent event;
            initializeKeyEvent(&event, keyEntry);

            mLock.unlock();

            bool fallback = mPolicy->dispatchUnhandledKey(connection->inputWindowHandle,
                    &event, keyEntry->policyFlags, &event);

            mLock.lock();

            if (connection->status != Connection::STATUS_NORMAL) {
                connection->inputState.removeFallbackKey(originalKeyCode);
                return true; 
            }

            ALOG_ASSERT(connection->outboundQueue.head == dispatchEntry);

            
            
            if (initialDown) {
                if (fallback) {
                    fallbackKeyCode = event.getKeyCode();
                } else {
                    fallbackKeyCode = AKEYCODE_UNKNOWN;
                }
                connection->inputState.setFallbackKey(originalKeyCode, fallbackKeyCode);
            }

            ALOG_ASSERT(fallbackKeyCode != -1);

            
            
            
            if (fallbackKeyCode != AKEYCODE_UNKNOWN
                    && (!fallback || fallbackKeyCode != event.getKeyCode())) {
#if DEBUG_OUTBOUND_EVENT_DETAILS
                if (fallback) {
                    ALOGD("Unhandled key event: Policy requested to send key %d"
                            "as a fallback for %d, but on the DOWN it had requested "
                            "to send %d instead.  Fallback canceled.",
                            event.getKeyCode(), originalKeyCode, fallbackKeyCode);
                } else {
                    ALOGD("Unhandled key event: Policy did not request fallback for %d,"
                            "but on the DOWN it had requested to send %d.  "
                            "Fallback canceled.",
                            originalKeyCode, fallbackKeyCode);
                }
#endif

                CancelationOptions options(CancelationOptions::CANCEL_FALLBACK_EVENTS,
                        "canceling fallback, policy no longer desires it");
                options.keyCode = fallbackKeyCode;
                synthesizeCancelationEventsForConnectionLocked(connection, options);

                fallback = false;
                fallbackKeyCode = AKEYCODE_UNKNOWN;
                if (keyEntry->action != AKEY_EVENT_ACTION_UP) {
                    connection->inputState.setFallbackKey(originalKeyCode,
                            fallbackKeyCode);
                }
            }

#if DEBUG_OUTBOUND_EVENT_DETAILS
            {
                String8 msg;
                const KeyedVector<int32_t, int32_t>& fallbackKeys =
                        connection->inputState.getFallbackKeys();
                for (size_t i = 0; i < fallbackKeys.size(); i++) {
                    msg.appendFormat(", %d->%d", fallbackKeys.keyAt(i),
                            fallbackKeys.valueAt(i));
                }
                ALOGD("Unhandled key event: %d currently tracked fallback keys%s.",
                        fallbackKeys.size(), msg.string());
            }
#endif

            if (fallback) {
                
                keyEntry->eventTime = event.getEventTime();
                keyEntry->deviceId = event.getDeviceId();
                keyEntry->source = event.getSource();
                keyEntry->flags = event.getFlags() | AKEY_EVENT_FLAG_FALLBACK;
                keyEntry->keyCode = fallbackKeyCode;
                keyEntry->scanCode = event.getScanCode();
                keyEntry->metaState = event.getMetaState();
                keyEntry->repeatCount = event.getRepeatCount();
                keyEntry->downTime = event.getDownTime();
                keyEntry->syntheticRepeat = false;

#if DEBUG_OUTBOUND_EVENT_DETAILS
                ALOGD("Unhandled key event: Dispatching fallback key.  "
                        "originalKeyCode=%d, fallbackKeyCode=%d, fallbackMetaState=%08x",
                        originalKeyCode, fallbackKeyCode, keyEntry->metaState);
#endif

                dispatchEntry->inProgress = false;
                startDispatchCycleLocked(now(), connection);
                return true; 
            } else {
#if DEBUG_OUTBOUND_EVENT_DETAILS
                ALOGD("Unhandled key event: No fallback key.");
#endif
            }
        }
    }
    return false;
}

bool InputDispatcher::afterMotionEventLockedInterruptible(const sp<Connection>& connection,
        DispatchEntry* dispatchEntry, MotionEntry* motionEntry, bool handled) {
    return false;
}

void InputDispatcher::doPokeUserActivityLockedInterruptible(CommandEntry* commandEntry) {
    mLock.unlock();

    mPolicy->pokeUserActivity(commandEntry->eventTime, commandEntry->userActivityEventType);

    mLock.lock();
}

void InputDispatcher::initializeKeyEvent(KeyEvent* event, const KeyEntry* entry) {
    event->initialize(entry->deviceId, entry->source, entry->action, entry->flags,
            entry->keyCode, entry->scanCode, entry->metaState, entry->repeatCount,
            entry->downTime, entry->eventTime);
}

void InputDispatcher::updateDispatchStatisticsLocked(nsecs_t currentTime, const EventEntry* entry,
        int32_t injectionResult, nsecs_t timeSpentWaitingForApplication) {
    
}

void InputDispatcher::dump(String8& dump) {
    AutoMutex _l(mLock);

    dump.append("Input Dispatcher State:\n");
    dumpDispatchStateLocked(dump);

    dump.append(INDENT "Configuration:\n");
    dump.appendFormat(INDENT2 "MaxEventsPerSecond: %d\n", mConfig.maxEventsPerSecond);
    dump.appendFormat(INDENT2 "KeyRepeatDelay: %0.1fms\n", mConfig.keyRepeatDelay * 0.000001f);
    dump.appendFormat(INDENT2 "KeyRepeatTimeout: %0.1fms\n", mConfig.keyRepeatTimeout * 0.000001f);
}

void InputDispatcher::monitor() {
    
    mLock.lock();
    mLock.unlock();
}




template <typename T>
uint32_t InputDispatcher::Queue<T>::count() const {
    uint32_t result = 0;
    for (const T* entry = head; entry; entry = entry->next) {
        result += 1;
    }
    return result;
}




InputDispatcher::InjectionState::InjectionState(int32_t injectorPid, int32_t injectorUid) :
        refCount(1),
        injectorPid(injectorPid), injectorUid(injectorUid),
        injectionResult(INPUT_EVENT_INJECTION_PENDING), injectionIsAsync(false),
        pendingForegroundDispatches(0) {
}

InputDispatcher::InjectionState::~InjectionState() {
}

void InputDispatcher::InjectionState::release() {
    refCount -= 1;
    if (refCount == 0) {
        delete this;
    } else {
        ALOG_ASSERT(refCount > 0);
    }
}




InputDispatcher::EventEntry::EventEntry(int32_t type, nsecs_t eventTime, uint32_t policyFlags) :
        refCount(1), type(type), eventTime(eventTime), policyFlags(policyFlags),
        injectionState(NULL), dispatchInProgress(false) {
}

InputDispatcher::EventEntry::~EventEntry() {
    releaseInjectionState();
}

void InputDispatcher::EventEntry::release() {
    refCount -= 1;
    if (refCount == 0) {
        delete this;
    } else {
        ALOG_ASSERT(refCount > 0);
    }
}

void InputDispatcher::EventEntry::releaseInjectionState() {
    if (injectionState) {
        injectionState->release();
        injectionState = NULL;
    }
}




InputDispatcher::ConfigurationChangedEntry::ConfigurationChangedEntry(nsecs_t eventTime) :
        EventEntry(TYPE_CONFIGURATION_CHANGED, eventTime, 0) {
}

InputDispatcher::ConfigurationChangedEntry::~ConfigurationChangedEntry() {
}




InputDispatcher::DeviceResetEntry::DeviceResetEntry(nsecs_t eventTime, int32_t deviceId) :
        EventEntry(TYPE_DEVICE_RESET, eventTime, 0),
        deviceId(deviceId) {
}

InputDispatcher::DeviceResetEntry::~DeviceResetEntry() {
}




InputDispatcher::KeyEntry::KeyEntry(nsecs_t eventTime,
        int32_t deviceId, uint32_t source, uint32_t policyFlags, int32_t action,
        int32_t flags, int32_t keyCode, int32_t scanCode, int32_t metaState,
        int32_t repeatCount, nsecs_t downTime) :
        EventEntry(TYPE_KEY, eventTime, policyFlags),
        deviceId(deviceId), source(source), action(action), flags(flags),
        keyCode(keyCode), scanCode(scanCode), metaState(metaState),
        repeatCount(repeatCount), downTime(downTime),
        syntheticRepeat(false), interceptKeyResult(KeyEntry::INTERCEPT_KEY_RESULT_UNKNOWN),
        interceptKeyWakeupTime(0) {
}

InputDispatcher::KeyEntry::~KeyEntry() {
}

void InputDispatcher::KeyEntry::recycle() {
    releaseInjectionState();

    dispatchInProgress = false;
    syntheticRepeat = false;
    interceptKeyResult = KeyEntry::INTERCEPT_KEY_RESULT_UNKNOWN;
    interceptKeyWakeupTime = 0;
}




InputDispatcher::MotionSample::MotionSample(nsecs_t eventTime,
        const PointerCoords* pointerCoords, uint32_t pointerCount) :
        next(NULL), eventTime(eventTime), eventTimeBeforeCoalescing(eventTime) {
    for (uint32_t i = 0; i < pointerCount; i++) {
        this->pointerCoords[i].copyFrom(pointerCoords[i]);
    }
}




InputDispatcher::MotionEntry::MotionEntry(nsecs_t eventTime,
        int32_t deviceId, uint32_t source, uint32_t policyFlags, int32_t action, int32_t flags,
        int32_t metaState, int32_t buttonState,
        int32_t edgeFlags, float xPrecision, float yPrecision,
        nsecs_t downTime, uint32_t pointerCount,
        const PointerProperties* pointerProperties, const PointerCoords* pointerCoords) :
        EventEntry(TYPE_MOTION, eventTime, policyFlags),
        deviceId(deviceId), source(source), action(action), flags(flags),
        metaState(metaState), buttonState(buttonState), edgeFlags(edgeFlags),
        xPrecision(xPrecision), yPrecision(yPrecision),
        downTime(downTime), pointerCount(pointerCount),
        firstSample(eventTime, pointerCoords, pointerCount),
        lastSample(&firstSample) {
    for (uint32_t i = 0; i < pointerCount; i++) {
        this->pointerProperties[i].copyFrom(pointerProperties[i]);
    }
}

InputDispatcher::MotionEntry::~MotionEntry() {
    for (MotionSample* sample = firstSample.next; sample != NULL; ) {
        MotionSample* next = sample->next;
        delete sample;
        sample = next;
    }
}

uint32_t InputDispatcher::MotionEntry::countSamples() const {
    uint32_t count = 1;
    for (MotionSample* sample = firstSample.next; sample != NULL; sample = sample->next) {
        count += 1;
    }
    return count;
}

bool InputDispatcher::MotionEntry::canAppendSamples(int32_t action, uint32_t pointerCount,
        const PointerProperties* pointerProperties) const {
    if (this->action != action
            || this->pointerCount != pointerCount
            || this->isInjected()) {
        return false;
    }
    for (uint32_t i = 0; i < pointerCount; i++) {
        if (this->pointerProperties[i] != pointerProperties[i]) {
            return false;
        }
    }
    return true;
}

void InputDispatcher::MotionEntry::appendSample(
        nsecs_t eventTime, const PointerCoords* pointerCoords) {
    MotionSample* sample = new MotionSample(eventTime, pointerCoords, pointerCount);

    lastSample->next = sample;
    lastSample = sample;
}




InputDispatcher::DispatchEntry::DispatchEntry(EventEntry* eventEntry,
        int32_t targetFlags, float xOffset, float yOffset, float scaleFactor) :
        eventEntry(eventEntry), targetFlags(targetFlags),
        xOffset(xOffset), yOffset(yOffset), scaleFactor(scaleFactor),
        inProgress(false),
        resolvedAction(0), resolvedFlags(0),
        headMotionSample(NULL), tailMotionSample(NULL) {
    eventEntry->refCount += 1;
}

InputDispatcher::DispatchEntry::~DispatchEntry() {
    eventEntry->release();
}




InputDispatcher::InputState::InputState() {
}

InputDispatcher::InputState::~InputState() {
}

bool InputDispatcher::InputState::isNeutral() const {
    return mKeyMementos.isEmpty() && mMotionMementos.isEmpty();
}

bool InputDispatcher::InputState::isHovering(int32_t deviceId, uint32_t source) const {
    for (size_t i = 0; i < mMotionMementos.size(); i++) {
        const MotionMemento& memento = mMotionMementos.itemAt(i);
        if (memento.deviceId == deviceId
                && memento.source == source
                && memento.hovering) {
            return true;
        }
    }
    return false;
}

bool InputDispatcher::InputState::trackKey(const KeyEntry* entry,
        int32_t action, int32_t flags) {
    switch (action) {
    case AKEY_EVENT_ACTION_UP: {
        if (entry->flags & AKEY_EVENT_FLAG_FALLBACK) {
            for (size_t i = 0; i < mFallbackKeys.size(); ) {
                if (mFallbackKeys.valueAt(i) == entry->keyCode) {
                    mFallbackKeys.removeItemsAt(i);
                } else {
                    i += 1;
                }
            }
        }
        ssize_t index = findKeyMemento(entry);
        if (index >= 0) {
            mKeyMementos.removeAt(index);
            return true;
        }
        















        return true;
    }

    case AKEY_EVENT_ACTION_DOWN: {
        ssize_t index = findKeyMemento(entry);
        if (index >= 0) {
            mKeyMementos.removeAt(index);
        }
        addKeyMemento(entry, flags);
        return true;
    }

    default:
        return true;
    }
}

bool InputDispatcher::InputState::trackMotion(const MotionEntry* entry,
        int32_t action, int32_t flags) {
    int32_t actionMasked = action & AMOTION_EVENT_ACTION_MASK;
    switch (actionMasked) {
    case AMOTION_EVENT_ACTION_UP:
    case AMOTION_EVENT_ACTION_CANCEL: {
        ssize_t index = findMotionMemento(entry, false );
        if (index >= 0) {
            mMotionMementos.removeAt(index);
            return true;
        }
#if DEBUG_OUTBOUND_EVENT_DETAILS
        ALOGD("Dropping inconsistent motion up or cancel event: deviceId=%d, source=%08x, "
                "actionMasked=%d",
                entry->deviceId, entry->source, actionMasked);
#endif
        return false;
    }

    case AMOTION_EVENT_ACTION_DOWN: {
        ssize_t index = findMotionMemento(entry, false );
        if (index >= 0) {
            mMotionMementos.removeAt(index);
        }
        addMotionMemento(entry, flags, false );
        return true;
    }

    case AMOTION_EVENT_ACTION_POINTER_UP:
    case AMOTION_EVENT_ACTION_POINTER_DOWN:
    case AMOTION_EVENT_ACTION_MOVE: {
        ssize_t index = findMotionMemento(entry, false );
        if (index >= 0) {
            MotionMemento& memento = mMotionMementos.editItemAt(index);
            memento.setPointers(entry);
            return true;
        }
        if (actionMasked == AMOTION_EVENT_ACTION_MOVE
                && (entry->source & (AINPUT_SOURCE_CLASS_JOYSTICK
                        | AINPUT_SOURCE_CLASS_NAVIGATION))) {
            
            return true;
        }
#if DEBUG_OUTBOUND_EVENT_DETAILS
        ALOGD("Dropping inconsistent motion pointer up/down or move event: "
                "deviceId=%d, source=%08x, actionMasked=%d",
                entry->deviceId, entry->source, actionMasked);
#endif
        return false;
    }

    case AMOTION_EVENT_ACTION_HOVER_EXIT: {
        ssize_t index = findMotionMemento(entry, true );
        if (index >= 0) {
            mMotionMementos.removeAt(index);
            return true;
        }
#if DEBUG_OUTBOUND_EVENT_DETAILS
        ALOGD("Dropping inconsistent motion hover exit event: deviceId=%d, source=%08x",
                entry->deviceId, entry->source);
#endif
        return false;
    }

    case AMOTION_EVENT_ACTION_HOVER_ENTER:
    case AMOTION_EVENT_ACTION_HOVER_MOVE: {
        ssize_t index = findMotionMemento(entry, true );
        if (index >= 0) {
            mMotionMementos.removeAt(index);
        }
        addMotionMemento(entry, flags, true );
        return true;
    }

    default:
        return true;
    }
}

ssize_t InputDispatcher::InputState::findKeyMemento(const KeyEntry* entry) const {
    for (size_t i = 0; i < mKeyMementos.size(); i++) {
        const KeyMemento& memento = mKeyMementos.itemAt(i);
        if (memento.deviceId == entry->deviceId
                && memento.source == entry->source
                && memento.keyCode == entry->keyCode
                && memento.scanCode == entry->scanCode) {
            return i;
        }
    }
    return -1;
}

ssize_t InputDispatcher::InputState::findMotionMemento(const MotionEntry* entry,
        bool hovering) const {
    for (size_t i = 0; i < mMotionMementos.size(); i++) {
        const MotionMemento& memento = mMotionMementos.itemAt(i);
        if (memento.deviceId == entry->deviceId
                && memento.source == entry->source
                && memento.hovering == hovering) {
            return i;
        }
    }
    return -1;
}

void InputDispatcher::InputState::addKeyMemento(const KeyEntry* entry, int32_t flags) {
    mKeyMementos.push();
    KeyMemento& memento = mKeyMementos.editTop();
    memento.deviceId = entry->deviceId;
    memento.source = entry->source;
    memento.keyCode = entry->keyCode;
    memento.scanCode = entry->scanCode;
    memento.flags = flags;
    memento.downTime = entry->downTime;
}

void InputDispatcher::InputState::addMotionMemento(const MotionEntry* entry,
        int32_t flags, bool hovering) {
    mMotionMementos.push();
    MotionMemento& memento = mMotionMementos.editTop();
    memento.deviceId = entry->deviceId;
    memento.source = entry->source;
    memento.flags = flags;
    memento.xPrecision = entry->xPrecision;
    memento.yPrecision = entry->yPrecision;
    memento.downTime = entry->downTime;
    memento.setPointers(entry);
    memento.hovering = hovering;
}

void InputDispatcher::InputState::MotionMemento::setPointers(const MotionEntry* entry) {
    pointerCount = entry->pointerCount;
    for (uint32_t i = 0; i < entry->pointerCount; i++) {
        pointerProperties[i].copyFrom(entry->pointerProperties[i]);
        pointerCoords[i].copyFrom(entry->lastSample->pointerCoords[i]);
    }
}

void InputDispatcher::InputState::synthesizeCancelationEvents(nsecs_t currentTime,
        Vector<EventEntry*>& outEvents, const CancelationOptions& options) {
    for (size_t i = 0; i < mKeyMementos.size(); i++) {
        const KeyMemento& memento = mKeyMementos.itemAt(i);
        if (shouldCancelKey(memento, options)) {
            outEvents.push(new KeyEntry(currentTime,
                    memento.deviceId, memento.source, 0,
                    AKEY_EVENT_ACTION_UP, memento.flags | AKEY_EVENT_FLAG_CANCELED,
                    memento.keyCode, memento.scanCode, 0, 0, memento.downTime));
        }
    }

    for (size_t i = 0; i < mMotionMementos.size(); i++) {
        const MotionMemento& memento = mMotionMementos.itemAt(i);
        if (shouldCancelMotion(memento, options)) {
            outEvents.push(new MotionEntry(currentTime,
                    memento.deviceId, memento.source, 0,
                    memento.hovering
                            ? AMOTION_EVENT_ACTION_HOVER_EXIT
                            : AMOTION_EVENT_ACTION_CANCEL,
                    memento.flags, 0, 0, 0,
                    memento.xPrecision, memento.yPrecision, memento.downTime,
                    memento.pointerCount, memento.pointerProperties, memento.pointerCoords));
        }
    }
}

void InputDispatcher::InputState::clear() {
    mKeyMementos.clear();
    mMotionMementos.clear();
    mFallbackKeys.clear();
}

void InputDispatcher::InputState::copyPointerStateTo(InputState& other) const {
    for (size_t i = 0; i < mMotionMementos.size(); i++) {
        const MotionMemento& memento = mMotionMementos.itemAt(i);
        if (memento.source & AINPUT_SOURCE_CLASS_POINTER) {
            for (size_t j = 0; j < other.mMotionMementos.size(); ) {
                const MotionMemento& otherMemento = other.mMotionMementos.itemAt(j);
                if (memento.deviceId == otherMemento.deviceId
                        && memento.source == otherMemento.source) {
                    other.mMotionMementos.removeAt(j);
                } else {
                    j += 1;
                }
            }
            other.mMotionMementos.push(memento);
        }
    }
}

int32_t InputDispatcher::InputState::getFallbackKey(int32_t originalKeyCode) {
    ssize_t index = mFallbackKeys.indexOfKey(originalKeyCode);
    return index >= 0 ? mFallbackKeys.valueAt(index) : -1;
}

void InputDispatcher::InputState::setFallbackKey(int32_t originalKeyCode,
        int32_t fallbackKeyCode) {
    ssize_t index = mFallbackKeys.indexOfKey(originalKeyCode);
    if (index >= 0) {
        mFallbackKeys.replaceValueAt(index, fallbackKeyCode);
    } else {
        mFallbackKeys.add(originalKeyCode, fallbackKeyCode);
    }
}

void InputDispatcher::InputState::removeFallbackKey(int32_t originalKeyCode) {
    mFallbackKeys.removeItem(originalKeyCode);
}

bool InputDispatcher::InputState::shouldCancelKey(const KeyMemento& memento,
        const CancelationOptions& options) {
    if (options.keyCode != -1 && memento.keyCode != options.keyCode) {
        return false;
    }

    if (options.deviceId != -1 && memento.deviceId != options.deviceId) {
        return false;
    }

    switch (options.mode) {
    case CancelationOptions::CANCEL_ALL_EVENTS:
    case CancelationOptions::CANCEL_NON_POINTER_EVENTS:
        return true;
    case CancelationOptions::CANCEL_FALLBACK_EVENTS:
        return memento.flags & AKEY_EVENT_FLAG_FALLBACK;
    default:
        return false;
    }
}

bool InputDispatcher::InputState::shouldCancelMotion(const MotionMemento& memento,
        const CancelationOptions& options) {
    if (options.deviceId != -1 && memento.deviceId != options.deviceId) {
        return false;
    }

    switch (options.mode) {
    case CancelationOptions::CANCEL_ALL_EVENTS:
        return true;
    case CancelationOptions::CANCEL_POINTER_EVENTS:
        return memento.source & AINPUT_SOURCE_CLASS_POINTER;
    case CancelationOptions::CANCEL_NON_POINTER_EVENTS:
        return !(memento.source & AINPUT_SOURCE_CLASS_POINTER);
    default:
        return false;
    }
}




InputDispatcher::Connection::Connection(const sp<InputChannel>& inputChannel,
        const sp<InputWindowHandle>& inputWindowHandle, bool monitor) :
        status(STATUS_NORMAL), inputChannel(inputChannel), inputWindowHandle(inputWindowHandle),
        monitor(monitor),
        inputPublisher(inputChannel),
        lastEventTime(LONG_LONG_MAX), lastDispatchTime(LONG_LONG_MAX) {
}

InputDispatcher::Connection::~Connection() {
}

status_t InputDispatcher::Connection::initialize() {
    return inputPublisher.initialize();
}

const char* InputDispatcher::Connection::getStatusLabel() const {
    switch (status) {
    case STATUS_NORMAL:
        return "NORMAL";

    case STATUS_BROKEN:
        return "BROKEN";

    case STATUS_ZOMBIE:
        return "ZOMBIE";

    default:
        return "UNKNOWN";
    }
}

InputDispatcher::DispatchEntry* InputDispatcher::Connection::findQueuedDispatchEntryForEvent(
        const EventEntry* eventEntry) const {
    for (DispatchEntry* dispatchEntry = outboundQueue.tail; dispatchEntry;
            dispatchEntry = dispatchEntry->prev) {
        if (dispatchEntry->eventEntry == eventEntry) {
            return dispatchEntry;
        }
    }
    return NULL;
}




InputDispatcher::CommandEntry::CommandEntry(Command command) :
    command(command), eventTime(0), keyEntry(NULL), userActivityEventType(0), handled(false) {
}

InputDispatcher::CommandEntry::~CommandEntry() {
}




InputDispatcher::TouchState::TouchState() :
    down(false), split(false), deviceId(-1), source(0) {
}

InputDispatcher::TouchState::~TouchState() {
}

void InputDispatcher::TouchState::reset() {
    down = false;
    split = false;
    deviceId = -1;
    source = 0;
    windows.clear();
}

void InputDispatcher::TouchState::copyFrom(const TouchState& other) {
    down = other.down;
    split = other.split;
    deviceId = other.deviceId;
    source = other.source;
    windows = other.windows;
}

void InputDispatcher::TouchState::addOrUpdateWindow(const sp<InputWindowHandle>& windowHandle,
        int32_t targetFlags, BitSet32 pointerIds) {
    if (targetFlags & InputTarget::FLAG_SPLIT) {
        split = true;
    }

    for (size_t i = 0; i < windows.size(); i++) {
        TouchedWindow& touchedWindow = windows.editItemAt(i);
        if (touchedWindow.windowHandle == windowHandle) {
            touchedWindow.targetFlags |= targetFlags;
            if (targetFlags & InputTarget::FLAG_DISPATCH_AS_SLIPPERY_EXIT) {
                touchedWindow.targetFlags &= ~InputTarget::FLAG_DISPATCH_AS_IS;
            }
            touchedWindow.pointerIds.value |= pointerIds.value;
            return;
        }
    }

    windows.push();

    TouchedWindow& touchedWindow = windows.editTop();
    touchedWindow.windowHandle = windowHandle;
    touchedWindow.targetFlags = targetFlags;
    touchedWindow.pointerIds = pointerIds;
}

void InputDispatcher::TouchState::filterNonAsIsTouchWindows() {
    for (size_t i = 0 ; i < windows.size(); ) {
        TouchedWindow& window = windows.editItemAt(i);
        if (window.targetFlags & (InputTarget::FLAG_DISPATCH_AS_IS
                | InputTarget::FLAG_DISPATCH_AS_SLIPPERY_ENTER)) {
            window.targetFlags &= ~InputTarget::FLAG_DISPATCH_MASK;
            window.targetFlags |= InputTarget::FLAG_DISPATCH_AS_IS;
            i += 1;
        } else {
            windows.removeAt(i);
        }
    }
}

sp<InputWindowHandle> InputDispatcher::TouchState::getFirstForegroundWindowHandle() const {
    for (size_t i = 0; i < windows.size(); i++) {
        const TouchedWindow& window = windows.itemAt(i);
        if (window.targetFlags & InputTarget::FLAG_FOREGROUND) {
            return window.windowHandle;
        }
    }
    return NULL;
}

bool InputDispatcher::TouchState::isSlippery() const {
    
    bool haveSlipperyForegroundWindow = false;
    for (size_t i = 0; i < windows.size(); i++) {
        const TouchedWindow& window = windows.itemAt(i);
        if (window.targetFlags & InputTarget::FLAG_FOREGROUND) {
            if (haveSlipperyForegroundWindow
                    || !(window.windowHandle->getInfo()->layoutParamsFlags
                            & InputWindowInfo::FLAG_SLIPPERY)) {
                return false;
            }
            haveSlipperyForegroundWindow = true;
        }
    }
    return haveSlipperyForegroundWindow;
}




InputDispatcherThread::InputDispatcherThread(const sp<InputDispatcherInterface>& dispatcher) :
        Thread( true), mDispatcher(dispatcher) {
}

InputDispatcherThread::~InputDispatcherThread() {
}

bool InputDispatcherThread::threadLoop() {
    mDispatcher->dispatchOnce();
    return true;
}

} 
