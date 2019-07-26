















#ifndef _UI_INPUT_DISPATCHER_H
#define _UI_INPUT_DISPATCHER_H

#include "Input.h"
#include "InputTransport.h"
#include <utils/KeyedVector.h>
#include <utils/Vector.h>
#include <utils/threads.h>
#include <utils/Timers.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/Looper.h>
#include <utils/BitSet.h>
#include <cutils/atomic.h>

#include <stddef.h>
#include <unistd.h>
#include <limits.h>

#include "InputWindow.h"
#include "InputApplication.h"
#include "InputListener.h"


namespace android {




enum {
    
    INPUT_EVENT_INJECTION_PENDING = -1,

    
    INPUT_EVENT_INJECTION_SUCCEEDED = 0,

    

    INPUT_EVENT_INJECTION_PERMISSION_DENIED = 1,

    
    INPUT_EVENT_INJECTION_FAILED = 2,

    
    INPUT_EVENT_INJECTION_TIMED_OUT = 3
};




enum {
    
    INPUT_EVENT_INJECTION_SYNC_NONE = 0,

    


    INPUT_EVENT_INJECTION_SYNC_WAIT_FOR_RESULT = 1,

    
    INPUT_EVENT_INJECTION_SYNC_WAIT_FOR_FINISHED = 2,
};








struct InputTarget {
    enum {
        
        FLAG_FOREGROUND = 1 << 0,

        


        FLAG_WINDOW_IS_OBSCURED = 1 << 1,

        
        FLAG_SPLIT = 1 << 2,

        



        FLAG_ZERO_COORDS = 1 << 3,

        

        FLAG_DISPATCH_AS_IS = 1 << 8,

        


        FLAG_DISPATCH_AS_OUTSIDE = 1 << 9,

        

        FLAG_DISPATCH_AS_HOVER_ENTER = 1 << 10,

        



        FLAG_DISPATCH_AS_HOVER_EXIT = 1 << 11,

        


        FLAG_DISPATCH_AS_SLIPPERY_EXIT = 1 << 12,

        


        FLAG_DISPATCH_AS_SLIPPERY_ENTER = 1 << 13,

        
        FLAG_DISPATCH_MASK = FLAG_DISPATCH_AS_IS
                | FLAG_DISPATCH_AS_OUTSIDE
                | FLAG_DISPATCH_AS_HOVER_ENTER
                | FLAG_DISPATCH_AS_HOVER_EXIT
                | FLAG_DISPATCH_AS_SLIPPERY_EXIT
                | FLAG_DISPATCH_AS_SLIPPERY_ENTER,
    };

    
    sp<InputChannel> inputChannel;

    
    int32_t flags;

    
    
    float xOffset, yOffset;

    
    
    float scaleFactor;

    
    
    BitSet32 pointerIds;
};









struct InputDispatcherConfiguration {
    
    nsecs_t keyRepeatTimeout;

    
    nsecs_t keyRepeatDelay;

    InputDispatcherConfiguration() :
            keyRepeatTimeout(500 * 1000000LL),
            keyRepeatDelay(50 * 1000000LL) { }
};











class InputDispatcherPolicyInterface : public virtual RefBase {
protected:
    InputDispatcherPolicyInterface() { }
    virtual ~InputDispatcherPolicyInterface() { }

public:
    
    virtual void notifyConfigurationChanged(nsecs_t when) = 0;

    

    virtual nsecs_t notifyANR(const sp<InputApplicationHandle>& inputApplicationHandle,
            const sp<InputWindowHandle>& inputWindowHandle) = 0;

    
    virtual void notifyInputChannelBroken(const sp<InputWindowHandle>& inputWindowHandle) = 0;

    
    virtual void getDispatcherConfiguration(InputDispatcherConfiguration* outConfig) = 0;

    
    virtual bool isKeyRepeatEnabled() = 0;

    




    virtual bool filterInputEvent(const InputEvent* inputEvent, uint32_t policyFlags) = 0;

    






    virtual void interceptKeyBeforeQueueing(const KeyEvent* keyEvent, uint32_t& policyFlags) = 0;

    






    virtual void interceptMotionBeforeQueueing(nsecs_t when, uint32_t& policyFlags) = 0;

    
    virtual nsecs_t interceptKeyBeforeDispatching(const sp<InputWindowHandle>& inputWindowHandle,
            const KeyEvent* keyEvent, uint32_t policyFlags) = 0;

    

    virtual bool dispatchUnhandledKey(const sp<InputWindowHandle>& inputWindowHandle,
            const KeyEvent* keyEvent, uint32_t policyFlags, KeyEvent* outFallbackKeyEvent) = 0;

    

    virtual void notifySwitch(nsecs_t when,
            uint32_t switchValues, uint32_t switchMask, uint32_t policyFlags) = 0;

    
    virtual void pokeUserActivity(nsecs_t eventTime, int32_t eventType) = 0;

    





    virtual bool checkInjectEventsPermissionNonReentrant(
            int32_t injectorPid, int32_t injectorUid) = 0;
};




class InputDispatcherInterface : public virtual RefBase, public InputListenerInterface {
protected:
    InputDispatcherInterface() { }
    virtual ~InputDispatcherInterface() { }

public:
    


    virtual void dump(String8& dump) = 0;

    
    virtual void monitor() = 0;

    




    virtual void dispatchOnce() = 0;

    






    virtual int32_t injectInputEvent(const InputEvent* event,
            int32_t injectorPid, int32_t injectorUid, int32_t syncMode, int32_t timeoutMillis,
            uint32_t policyFlags) = 0;

    



    virtual void setInputWindows(const Vector<sp<InputWindowHandle> >& inputWindowHandles) = 0;

    



    virtual void setFocusedApplication(
            const sp<InputApplicationHandle>& inputApplicationHandle) = 0;

    



    virtual void setInputDispatchMode(bool enabled, bool frozen) = 0;

    





    virtual void setInputFilterEnabled(bool enabled) = 0;

    




    virtual bool transferTouchFocus(const sp<InputChannel>& fromChannel,
            const sp<InputChannel>& toChannel) = 0;

    




    virtual status_t registerInputChannel(const sp<InputChannel>& inputChannel,
            const sp<InputWindowHandle>& inputWindowHandle, bool monitor) = 0;
    virtual status_t unregisterInputChannel(const sp<InputChannel>& inputChannel) = 0;
};


















class InputDispatcher : public InputDispatcherInterface {
protected:
    virtual ~InputDispatcher();

public:
    explicit InputDispatcher(const sp<InputDispatcherPolicyInterface>& policy);

    virtual void dump(String8& dump);
    virtual void monitor();

    virtual void dispatchOnce();

    virtual void notifyConfigurationChanged(const NotifyConfigurationChangedArgs* args);
    virtual void notifyKey(const NotifyKeyArgs* args);
    virtual void notifyMotion(const NotifyMotionArgs* args);
    virtual void notifySwitch(const NotifySwitchArgs* args);
    virtual void notifyDeviceReset(const NotifyDeviceResetArgs* args);

    virtual int32_t injectInputEvent(const InputEvent* event,
            int32_t injectorPid, int32_t injectorUid, int32_t syncMode, int32_t timeoutMillis,
            uint32_t policyFlags);

    virtual void setInputWindows(const Vector<sp<InputWindowHandle> >& inputWindowHandles);
    virtual void setFocusedApplication(const sp<InputApplicationHandle>& inputApplicationHandle);
    virtual void setInputDispatchMode(bool enabled, bool frozen);
    virtual void setInputFilterEnabled(bool enabled);

    virtual bool transferTouchFocus(const sp<InputChannel>& fromChannel,
            const sp<InputChannel>& toChannel);

    virtual status_t registerInputChannel(const sp<InputChannel>& inputChannel,
            const sp<InputWindowHandle>& inputWindowHandle, bool monitor);
    virtual status_t unregisterInputChannel(const sp<InputChannel>& inputChannel);

private:
    template <typename T>
    struct Link {
        T* next;
        T* prev;

    protected:
        inline Link() : next(NULL), prev(NULL) { }
    };

    struct InjectionState {
        mutable int32_t refCount;

        int32_t injectorPid;
        int32_t injectorUid;
        int32_t injectionResult;  
        bool injectionIsAsync; 
        int32_t pendingForegroundDispatches; 

        InjectionState(int32_t injectorPid, int32_t injectorUid);
        void release();

    private:
        ~InjectionState();
    };

    struct EventEntry : Link<EventEntry> {
        enum {
            TYPE_CONFIGURATION_CHANGED,
            TYPE_DEVICE_RESET,
            TYPE_KEY,
            TYPE_MOTION
        };

        mutable int32_t refCount;
        int32_t type;
        nsecs_t eventTime;
        uint32_t policyFlags;
        InjectionState* injectionState;

        bool dispatchInProgress; 

        inline bool isInjected() const { return injectionState != NULL; }

        void release();

        virtual void appendDescription(String8& msg) const = 0;

    protected:
        EventEntry(int32_t type, nsecs_t eventTime, uint32_t policyFlags);
        virtual ~EventEntry();
        void releaseInjectionState();
    };

    struct ConfigurationChangedEntry : EventEntry {
        ConfigurationChangedEntry(nsecs_t eventTime);
        virtual void appendDescription(String8& msg) const;

    protected:
        virtual ~ConfigurationChangedEntry();
    };

    struct DeviceResetEntry : EventEntry {
        int32_t deviceId;

        DeviceResetEntry(nsecs_t eventTime, int32_t deviceId);
        virtual void appendDescription(String8& msg) const;

    protected:
        virtual ~DeviceResetEntry();
    };

    struct KeyEntry : EventEntry {
        int32_t deviceId;
        uint32_t source;
        int32_t action;
        int32_t flags;
        int32_t keyCode;
        int32_t scanCode;
        int32_t metaState;
        int32_t repeatCount;
        nsecs_t downTime;

        bool syntheticRepeat; 

        enum InterceptKeyResult {
            INTERCEPT_KEY_RESULT_UNKNOWN,
            INTERCEPT_KEY_RESULT_SKIP,
            INTERCEPT_KEY_RESULT_CONTINUE,
            INTERCEPT_KEY_RESULT_TRY_AGAIN_LATER,
        };
        InterceptKeyResult interceptKeyResult; 
        nsecs_t interceptKeyWakeupTime; 

        KeyEntry(nsecs_t eventTime,
                int32_t deviceId, uint32_t source, uint32_t policyFlags, int32_t action,
                int32_t flags, int32_t keyCode, int32_t scanCode, int32_t metaState,
                int32_t repeatCount, nsecs_t downTime);
        virtual void appendDescription(String8& msg) const;
        void recycle();

    protected:
        virtual ~KeyEntry();
    };

    struct MotionEntry : EventEntry {
        nsecs_t eventTime;
        int32_t deviceId;
        uint32_t source;
        int32_t action;
        int32_t flags;
        int32_t metaState;
        int32_t buttonState;
        int32_t edgeFlags;
        float xPrecision;
        float yPrecision;
        nsecs_t downTime;
        int32_t displayId;
        uint32_t pointerCount;
        PointerProperties pointerProperties[MAX_POINTERS];
        PointerCoords pointerCoords[MAX_POINTERS];

        MotionEntry(nsecs_t eventTime,
                int32_t deviceId, uint32_t source, uint32_t policyFlags,
                int32_t action, int32_t flags,
                int32_t metaState, int32_t buttonState, int32_t edgeFlags,
                float xPrecision, float yPrecision,
                nsecs_t downTime, int32_t displayId, uint32_t pointerCount,
                const PointerProperties* pointerProperties, const PointerCoords* pointerCoords);
        virtual void appendDescription(String8& msg) const;

    protected:
        virtual ~MotionEntry();
    };

    
    struct DispatchEntry : Link<DispatchEntry> {
        const uint32_t seq; 

        EventEntry* eventEntry; 
        int32_t targetFlags;
        float xOffset;
        float yOffset;
        float scaleFactor;
        nsecs_t deliveryTime; 

        
        int32_t resolvedAction;
        int32_t resolvedFlags;

        DispatchEntry(EventEntry* eventEntry,
                int32_t targetFlags, float xOffset, float yOffset, float scaleFactor);
        ~DispatchEntry();

        inline bool hasForegroundTarget() const {
            return targetFlags & InputTarget::FLAG_FOREGROUND;
        }

        inline bool isSplit() const {
            return targetFlags & InputTarget::FLAG_SPLIT;
        }

    private:
        static volatile int32_t sNextSeqAtomic;

        static uint32_t nextSeq();
    };

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    struct CommandEntry;
    typedef void (InputDispatcher::*Command)(CommandEntry* commandEntry);

    class Connection;
    struct CommandEntry : Link<CommandEntry> {
        CommandEntry(Command command);
        ~CommandEntry();

        Command command;

        
        sp<Connection> connection;
        nsecs_t eventTime;
        KeyEntry* keyEntry;
        sp<InputApplicationHandle> inputApplicationHandle;
        sp<InputWindowHandle> inputWindowHandle;
        int32_t userActivityEventType;
        uint32_t seq;
        bool handled;
    };

    
    template <typename T>
    struct Queue {
        T* head;
        T* tail;

        inline Queue() : head(NULL), tail(NULL) {
        }

        inline bool isEmpty() const {
            return !head;
        }

        inline void enqueueAtTail(T* entry) {
            entry->prev = tail;
            if (tail) {
                tail->next = entry;
            } else {
                head = entry;
            }
            entry->next = NULL;
            tail = entry;
        }

        inline void enqueueAtHead(T* entry) {
            entry->next = head;
            if (head) {
                head->prev = entry;
            } else {
                tail = entry;
            }
            entry->prev = NULL;
            head = entry;
        }

        inline void dequeue(T* entry) {
            if (entry->prev) {
                entry->prev->next = entry->next;
            } else {
                head = entry->next;
            }
            if (entry->next) {
                entry->next->prev = entry->prev;
            } else {
                tail = entry->prev;
            }
        }

        inline T* dequeueAtHead() {
            T* entry = head;
            head = entry->next;
            if (head) {
                head->prev = NULL;
            } else {
                tail = NULL;
            }
            return entry;
        }

        uint32_t count() const;
    };

    
    struct CancelationOptions {
        enum Mode {
            CANCEL_ALL_EVENTS = 0,
            CANCEL_POINTER_EVENTS = 1,
            CANCEL_NON_POINTER_EVENTS = 2,
            CANCEL_FALLBACK_EVENTS = 3,
        };

        
        Mode mode;

        
        const char* reason;

        
        int32_t keyCode;

        
        int32_t deviceId;

        CancelationOptions(Mode mode, const char* reason) :
                mode(mode), reason(reason), keyCode(-1), deviceId(-1) { }
    };

    

    class InputState {
    public:
        InputState();
        ~InputState();

        
        bool isNeutral() const;

        
        
        bool isHovering(int32_t deviceId, uint32_t source, int32_t displayId) const;

        
        
        
        bool trackKey(const KeyEntry* entry, int32_t action, int32_t flags);

        
        
        
        bool trackMotion(const MotionEntry* entry, int32_t action, int32_t flags);

        
        void synthesizeCancelationEvents(nsecs_t currentTime,
                Vector<EventEntry*>& outEvents, const CancelationOptions& options);

        
        void clear();

        
        void copyPointerStateTo(InputState& other) const;

        
        
        
        int32_t getFallbackKey(int32_t originalKeyCode);

        
        void setFallbackKey(int32_t originalKeyCode, int32_t fallbackKeyCode);

        
        void removeFallbackKey(int32_t originalKeyCode);

        inline const KeyedVector<int32_t, int32_t>& getFallbackKeys() const {
            return mFallbackKeys;
        }

    private:
        struct KeyMemento {
            int32_t deviceId;
            uint32_t source;
            int32_t keyCode;
            int32_t scanCode;
            int32_t metaState;
            int32_t flags;
            nsecs_t downTime;
            uint32_t policyFlags;
        };

        struct MotionMemento {
            int32_t deviceId;
            uint32_t source;
            int32_t flags;
            float xPrecision;
            float yPrecision;
            nsecs_t downTime;
            int32_t displayId;
            uint32_t pointerCount;
            PointerProperties pointerProperties[MAX_POINTERS];
            PointerCoords pointerCoords[MAX_POINTERS];
            bool hovering;
            uint32_t policyFlags;

            void setPointers(const MotionEntry* entry);
        };

        Vector<KeyMemento> mKeyMementos;
        Vector<MotionMemento> mMotionMementos;
        KeyedVector<int32_t, int32_t> mFallbackKeys;

        ssize_t findKeyMemento(const KeyEntry* entry) const;
        ssize_t findMotionMemento(const MotionEntry* entry, bool hovering) const;

        void addKeyMemento(const KeyEntry* entry, int32_t flags);
        void addMotionMemento(const MotionEntry* entry, int32_t flags, bool hovering);

        static bool shouldCancelKey(const KeyMemento& memento,
                const CancelationOptions& options);
        static bool shouldCancelMotion(const MotionMemento& memento,
                const CancelationOptions& options);
    };

    
    class Connection : public RefBase {
    protected:
        virtual ~Connection();

    public:
        enum Status {
            
            STATUS_NORMAL,
            
            STATUS_BROKEN,
            
            STATUS_ZOMBIE
        };

        Status status;
        sp<InputChannel> inputChannel; 
        sp<InputWindowHandle> inputWindowHandle; 
        bool monitor;
        InputPublisher inputPublisher;
        InputState inputState;

        
        
        bool inputPublisherBlocked;

        
        Queue<DispatchEntry> outboundQueue;

        
        
        Queue<DispatchEntry> waitQueue;

        explicit Connection(const sp<InputChannel>& inputChannel,
                const sp<InputWindowHandle>& inputWindowHandle, bool monitor);

        inline const char* getInputChannelName() const { return inputChannel->getName().string(); }

        const char* getWindowName() const;
        const char* getStatusLabel() const;

        DispatchEntry* findWaitQueueEntry(uint32_t seq);
    };

    enum DropReason {
        DROP_REASON_NOT_DROPPED = 0,
        DROP_REASON_POLICY = 1,
        DROP_REASON_APP_SWITCH = 2,
        DROP_REASON_DISABLED = 3,
        DROP_REASON_BLOCKED = 4,
        DROP_REASON_STALE = 5,
    };

    sp<InputDispatcherPolicyInterface> mPolicy;
    InputDispatcherConfiguration mConfig;

    Mutex mLock;

    Condition mDispatcherIsAliveCondition;

    sp<Looper> mLooper;

    EventEntry* mPendingEvent;
    Queue<EventEntry> mInboundQueue;
    Queue<CommandEntry> mCommandQueue;

    void dispatchOnceInnerLocked(nsecs_t* nextWakeupTime);

    
    bool enqueueInboundEventLocked(EventEntry* entry);

    
    void dropInboundEventLocked(EventEntry* entry, DropReason dropReason);

    
    bool mAppSwitchSawKeyDown;
    nsecs_t mAppSwitchDueTime;

    static bool isAppSwitchKeyCode(int32_t keyCode);
    bool isAppSwitchKeyEventLocked(KeyEntry* keyEntry);
    bool isAppSwitchPendingLocked();
    void resetPendingAppSwitchLocked(bool handled);

    
    static bool isStaleEventLocked(nsecs_t currentTime, EventEntry* entry);

    
    
    EventEntry* mNextUnblockedEvent;

    sp<InputWindowHandle> findTouchedWindowAtLocked(int32_t displayId, int32_t x, int32_t y);

    
    KeyedVector<int, sp<Connection> > mConnectionsByFd;

    ssize_t getConnectionIndexLocked(const sp<InputChannel>& inputChannel);

    
    Vector<sp<InputChannel> > mMonitoringChannels;

    
    Condition mInjectionResultAvailableCondition;
    bool hasInjectionPermission(int32_t injectorPid, int32_t injectorUid);
    void setInjectionResultLocked(EventEntry* entry, int32_t injectionResult);

    Condition mInjectionSyncFinishedCondition;
    void incrementPendingForegroundDispatchesLocked(EventEntry* entry);
    void decrementPendingForegroundDispatchesLocked(EventEntry* entry);

    
    struct KeyRepeatState {
        KeyEntry* lastKeyEntry; 
        nsecs_t nextRepeatTime;
    } mKeyRepeatState;

    void resetKeyRepeatLocked();
    KeyEntry* synthesizeKeyRepeatLocked(nsecs_t currentTime);

    
    bool haveCommandsLocked() const;
    bool runCommandsLockedInterruptible();
    CommandEntry* postCommandLocked(Command command);

    
    bool shouldSendKeyToInputFilterLocked(const NotifyKeyArgs* args);
    bool shouldSendMotionToInputFilterLocked(const NotifyMotionArgs* args);

    
    void drainInboundQueueLocked();
    void releasePendingEventLocked();
    void releaseInboundEventLocked(EventEntry* entry);

    
    bool mDispatchEnabled;
    bool mDispatchFrozen;
    bool mInputFilterEnabled;

    Vector<sp<InputWindowHandle> > mWindowHandles;

    sp<InputWindowHandle> getWindowHandleLocked(const sp<InputChannel>& inputChannel) const;
    bool hasWindowHandleLocked(const sp<InputWindowHandle>& windowHandle) const;

    
    sp<InputWindowHandle> mFocusedWindowHandle;

    
    struct TouchedWindow {
        sp<InputWindowHandle> windowHandle;
        int32_t targetFlags;
        BitSet32 pointerIds;        
    };
    struct TouchState {
        bool down;
        bool split;
        int32_t deviceId; 
        uint32_t source;  
        int32_t displayId; 
        Vector<TouchedWindow> windows;

        TouchState();
        ~TouchState();
        void reset();
        void copyFrom(const TouchState& other);
        void addOrUpdateWindow(const sp<InputWindowHandle>& windowHandle,
                int32_t targetFlags, BitSet32 pointerIds);
        void removeWindow(const sp<InputWindowHandle>& windowHandle);
        void filterNonAsIsTouchWindows();
        sp<InputWindowHandle> getFirstForegroundWindowHandle() const;
        bool isSlippery() const;
    };

    TouchState mTouchState;
    TouchState mTempTouchState;

    
    sp<InputApplicationHandle> mFocusedApplicationHandle;

    
    String8 mLastANRState;

    
    bool dispatchConfigurationChangedLocked(
            nsecs_t currentTime, ConfigurationChangedEntry* entry);
    bool dispatchDeviceResetLocked(
            nsecs_t currentTime, DeviceResetEntry* entry);
    bool dispatchKeyLocked(
            nsecs_t currentTime, KeyEntry* entry,
            DropReason* dropReason, nsecs_t* nextWakeupTime);
    bool dispatchMotionLocked(
            nsecs_t currentTime, MotionEntry* entry,
            DropReason* dropReason, nsecs_t* nextWakeupTime);
    void dispatchEventLocked(nsecs_t currentTime, EventEntry* entry,
            const Vector<InputTarget>& inputTargets);

    void logOutboundKeyDetailsLocked(const char* prefix, const KeyEntry* entry);
    void logOutboundMotionDetailsLocked(const char* prefix, const MotionEntry* entry);

    
    enum InputTargetWaitCause {
        INPUT_TARGET_WAIT_CAUSE_NONE,
        INPUT_TARGET_WAIT_CAUSE_SYSTEM_NOT_READY,
        INPUT_TARGET_WAIT_CAUSE_APPLICATION_NOT_READY,
    };

    InputTargetWaitCause mInputTargetWaitCause;
    nsecs_t mInputTargetWaitStartTime;
    nsecs_t mInputTargetWaitTimeoutTime;
    bool mInputTargetWaitTimeoutExpired;
    sp<InputApplicationHandle> mInputTargetWaitApplicationHandle;

    
    sp<InputWindowHandle> mLastHoverWindowHandle;

    
    int32_t handleTargetsNotReadyLocked(nsecs_t currentTime, const EventEntry* entry,
            const sp<InputApplicationHandle>& applicationHandle,
            const sp<InputWindowHandle>& windowHandle,
            nsecs_t* nextWakeupTime, const char* reason);
    void resumeAfterTargetsNotReadyTimeoutLocked(nsecs_t newTimeout,
            const sp<InputChannel>& inputChannel);
    nsecs_t getTimeSpentWaitingForApplicationLocked(nsecs_t currentTime);
    void resetANRTimeoutsLocked();

    int32_t findFocusedWindowTargetsLocked(nsecs_t currentTime, const EventEntry* entry,
            Vector<InputTarget>& inputTargets, nsecs_t* nextWakeupTime);
    int32_t findTouchedWindowTargetsLocked(nsecs_t currentTime, const MotionEntry* entry,
            Vector<InputTarget>& inputTargets, nsecs_t* nextWakeupTime,
            bool* outConflictingPointerActions);

    void addWindowTargetLocked(const sp<InputWindowHandle>& windowHandle,
            int32_t targetFlags, BitSet32 pointerIds, Vector<InputTarget>& inputTargets);
    void addMonitoringTargetsLocked(Vector<InputTarget>& inputTargets);

    void pokeUserActivityLocked(const EventEntry* eventEntry);
    bool checkInjectionPermission(const sp<InputWindowHandle>& windowHandle,
            const InjectionState* injectionState);
    bool isWindowObscuredAtPointLocked(const sp<InputWindowHandle>& windowHandle,
            int32_t x, int32_t y) const;
    bool isWindowReadyForMoreInputLocked(nsecs_t currentTime,
            const sp<InputWindowHandle>& windowHandle, const EventEntry* eventEntry);
    String8 getApplicationWindowLabelLocked(const sp<InputApplicationHandle>& applicationHandle,
            const sp<InputWindowHandle>& windowHandle);

    
    
    
    
    void prepareDispatchCycleLocked(nsecs_t currentTime, const sp<Connection>& connection,
            EventEntry* eventEntry, const InputTarget* inputTarget);
    void enqueueDispatchEntriesLocked(nsecs_t currentTime, const sp<Connection>& connection,
            EventEntry* eventEntry, const InputTarget* inputTarget);
    void enqueueDispatchEntryLocked(const sp<Connection>& connection,
            EventEntry* eventEntry, const InputTarget* inputTarget, int32_t dispatchMode);
    void startDispatchCycleLocked(nsecs_t currentTime, const sp<Connection>& connection);
    void finishDispatchCycleLocked(nsecs_t currentTime, const sp<Connection>& connection,
            uint32_t seq, bool handled);
    void abortBrokenDispatchCycleLocked(nsecs_t currentTime, const sp<Connection>& connection,
            bool notify);
    void drainDispatchQueueLocked(Queue<DispatchEntry>* queue);
    void releaseDispatchEntryLocked(DispatchEntry* dispatchEntry);
    static int handleReceiveCallback(int fd, int events, void* data);

    void synthesizeCancelationEventsForAllConnectionsLocked(
            const CancelationOptions& options);
    void synthesizeCancelationEventsForInputChannelLocked(const sp<InputChannel>& channel,
            const CancelationOptions& options);
    void synthesizeCancelationEventsForConnectionLocked(const sp<Connection>& connection,
            const CancelationOptions& options);

    
    MotionEntry* splitMotionEvent(const MotionEntry* originalMotionEntry, BitSet32 pointerIds);

    
    void resetAndDropEverythingLocked(const char* reason);

    
    void dumpDispatchStateLocked(String8& dump);
    void logDispatchStateLocked();

    
    void removeMonitorChannelLocked(const sp<InputChannel>& inputChannel);
    status_t unregisterInputChannelLocked(const sp<InputChannel>& inputChannel, bool notify);

    
    void activateConnectionLocked(Connection* connection);
    void deactivateConnectionLocked(Connection* connection);

    
    void onDispatchCycleFinishedLocked(
            nsecs_t currentTime, const sp<Connection>& connection, uint32_t seq, bool handled);
    void onDispatchCycleBrokenLocked(
            nsecs_t currentTime, const sp<Connection>& connection);
    void onANRLocked(
            nsecs_t currentTime, const sp<InputApplicationHandle>& applicationHandle,
            const sp<InputWindowHandle>& windowHandle,
            nsecs_t eventTime, nsecs_t waitStartTime, const char* reason);

    
    void doNotifyConfigurationChangedInterruptible(CommandEntry* commandEntry);
    void doNotifyInputChannelBrokenLockedInterruptible(CommandEntry* commandEntry);
    void doNotifyANRLockedInterruptible(CommandEntry* commandEntry);
    void doInterceptKeyBeforeDispatchingLockedInterruptible(CommandEntry* commandEntry);
    void doDispatchCycleFinishedLockedInterruptible(CommandEntry* commandEntry);
    bool afterKeyEventLockedInterruptible(const sp<Connection>& connection,
            DispatchEntry* dispatchEntry, KeyEntry* keyEntry, bool handled);
    bool afterMotionEventLockedInterruptible(const sp<Connection>& connection,
            DispatchEntry* dispatchEntry, MotionEntry* motionEntry, bool handled);
    void doPokeUserActivityLockedInterruptible(CommandEntry* commandEntry);
    void initializeKeyEvent(KeyEvent* event, const KeyEntry* entry);

    
    void updateDispatchStatisticsLocked(nsecs_t currentTime, const EventEntry* entry,
            int32_t injectionResult, nsecs_t timeSpentWaitingForApplication);
    void traceInboundQueueLengthLocked();
    void traceOutboundQueueLengthLocked(const sp<Connection>& connection);
    void traceWaitQueueLengthLocked(const sp<Connection>& connection);
};


class InputDispatcherThread : public Thread {
public:
    explicit InputDispatcherThread(const sp<InputDispatcherInterface>& dispatcher);
    ~InputDispatcherThread();

private:
    virtual bool threadLoop();

    sp<InputDispatcherInterface> mDispatcher;
};

} 

#endif 
