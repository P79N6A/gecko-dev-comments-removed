















#ifndef _UI_INPUT_READER_H
#define _UI_INPUT_READER_H

#include "EventHub.h"
#ifdef HAVE_ANDROID_OS
#include "PointerController.h"
#endif
#include "InputListener.h"

#include "Input.h"
#include "DisplayInfo.h"
#include <utils/KeyedVector.h>
#include <utils/threads.h>
#include <utils/Timers.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/BitSet.h>

#include <stddef.h>
#include <unistd.h>

namespace android {

class InputDevice;
class InputMapper;







struct InputReaderConfiguration {
    
    enum {
        
        CHANGE_POINTER_SPEED = 1 << 0,

        
        CHANGE_POINTER_GESTURE_ENABLEMENT = 1 << 1,

        
        CHANGE_DISPLAY_INFO = 1 << 2,

        
        CHANGE_SHOW_TOUCHES = 1 << 3,

        
        CHANGE_MUST_REOPEN = 1 << 31,
    };

    
    
    
    nsecs_t virtualKeyQuietTime;

    
    
    Vector<String8> excludedDeviceNames;

    
    VelocityControlParameters pointerVelocityControlParameters;

    
    VelocityControlParameters wheelVelocityControlParameters;

    
    bool pointerGesturesEnabled;

    
    
    
    nsecs_t pointerGestureQuietInterval;

    
    
    
    float pointerGestureDragMinSwitchSpeed; 

    
    
    nsecs_t pointerGestureTapInterval;

    
    
    
    
    
    
    
    nsecs_t pointerGestureTapDragInterval;

    
    
    float pointerGestureTapSlop; 

    
    
    
    nsecs_t pointerGestureMultitouchSettleInterval;

    
    
    float pointerGestureMultitouchMinDistance; 

    
    
    
    
    
    float pointerGestureSwipeTransitionAngleCosine;

    
    
    
    
    float pointerGestureSwipeMaxWidthRatio;

    
    
    
    
    float pointerGestureMovementSpeedRatio;

    
    
    
    
    
    float pointerGestureZoomSpeedRatio;

    
    bool showTouches;

    InputReaderConfiguration() :
            virtualKeyQuietTime(0),
            pointerVelocityControlParameters(1.0f, 500.0f, 3000.0f, 3.0f),
            wheelVelocityControlParameters(1.0f, 15.0f, 50.0f, 4.0f),
            pointerGesturesEnabled(true),
            pointerGestureQuietInterval(100 * 1000000LL), 
            pointerGestureDragMinSwitchSpeed(50), 
            pointerGestureTapInterval(150 * 1000000LL), 
            pointerGestureTapDragInterval(150 * 1000000LL), 
            pointerGestureTapSlop(10.0f), 
            pointerGestureMultitouchSettleInterval(100 * 1000000LL), 
            pointerGestureMultitouchMinDistance(15), 
            pointerGestureSwipeTransitionAngleCosine(0.2588f), 
            pointerGestureSwipeMaxWidthRatio(0.25f),
            pointerGestureMovementSpeedRatio(0.8f),
            pointerGestureZoomSpeedRatio(0.3f),
            showTouches(false) { }

    bool getDisplayInfo(int32_t displayId, bool external,
            int32_t* width, int32_t* height, int32_t* orientation) const;

    void setDisplayInfo(int32_t displayId, bool external,
            int32_t width, int32_t height, int32_t orientation);

private:
    struct DisplayInfo {
        int32_t width;
        int32_t height;
        int32_t orientation;

        DisplayInfo() :
            width(-1), height(-1), orientation(DISPLAY_ORIENTATION_0) {
        }
    };

    DisplayInfo mInternalDisplay;
    DisplayInfo mExternalDisplay;
};














class InputReaderPolicyInterface : public virtual RefBase {
protected:
    InputReaderPolicyInterface() { }
    virtual ~InputReaderPolicyInterface() { }

public:
    
    virtual void getReaderConfiguration(InputReaderConfiguration* outConfig) = 0;

#ifdef HAVE_ANDROID_OS
    
    virtual sp<PointerControllerInterface> obtainPointerController(int32_t deviceId) = 0;
#endif
};



class InputReaderInterface : public virtual RefBase {
protected:
    InputReaderInterface() { }
    virtual ~InputReaderInterface() { }

public:
    


    virtual void dump(String8& dump) = 0;

    
    virtual void monitor() = 0;

    




    virtual void loopOnce() = 0;

    



    virtual void getInputConfiguration(InputConfiguration* outConfiguration) = 0;

    





    virtual status_t getInputDeviceInfo(int32_t deviceId, InputDeviceInfo* outDeviceInfo) = 0;

    
    virtual void getInputDeviceIds(Vector<int32_t>& outDeviceIds) = 0;

    
    virtual int32_t getScanCodeState(int32_t deviceId, uint32_t sourceMask,
            int32_t scanCode) = 0;
    virtual int32_t getKeyCodeState(int32_t deviceId, uint32_t sourceMask,
            int32_t keyCode) = 0;
    virtual int32_t getSwitchState(int32_t deviceId, uint32_t sourceMask,
            int32_t sw) = 0;

    
    virtual bool hasKeys(int32_t deviceId, uint32_t sourceMask,
            size_t numCodes, const int32_t* keyCodes, uint8_t* outFlags) = 0;

    


    virtual void requestRefreshConfiguration(uint32_t changes) = 0;
};





class InputReaderContext {
public:
    InputReaderContext() { }
    virtual ~InputReaderContext() { }

    virtual void updateGlobalMetaState() = 0;
    virtual int32_t getGlobalMetaState() = 0;

    virtual void disableVirtualKeysUntil(nsecs_t time) = 0;
    virtual bool shouldDropVirtualKey(nsecs_t now,
            InputDevice* device, int32_t keyCode, int32_t scanCode) = 0;

    virtual void fadePointer() = 0;

    virtual void requestTimeoutAtTime(nsecs_t when) = 0;

    virtual InputReaderPolicyInterface* getPolicy() = 0;
    virtual InputListenerInterface* getListener() = 0;
    virtual EventHubInterface* getEventHub() = 0;
};













class InputReader : public InputReaderInterface {
public:
    InputReader(const sp<EventHubInterface>& eventHub,
            const sp<InputReaderPolicyInterface>& policy,
            const sp<InputListenerInterface>& listener);
    virtual ~InputReader();

    virtual void dump(String8& dump);
    virtual void monitor();

    virtual void loopOnce();

    virtual void getInputConfiguration(InputConfiguration* outConfiguration);

    virtual status_t getInputDeviceInfo(int32_t deviceId, InputDeviceInfo* outDeviceInfo);
    virtual void getInputDeviceIds(Vector<int32_t>& outDeviceIds);

    virtual int32_t getScanCodeState(int32_t deviceId, uint32_t sourceMask,
            int32_t scanCode);
    virtual int32_t getKeyCodeState(int32_t deviceId, uint32_t sourceMask,
            int32_t keyCode);
    virtual int32_t getSwitchState(int32_t deviceId, uint32_t sourceMask,
            int32_t sw);

    virtual bool hasKeys(int32_t deviceId, uint32_t sourceMask,
            size_t numCodes, const int32_t* keyCodes, uint8_t* outFlags);

    virtual void requestRefreshConfiguration(uint32_t changes);

protected:
    
    virtual InputDevice* createDeviceLocked(int32_t deviceId,
            const String8& name, uint32_t classes);

    class ContextImpl : public InputReaderContext {
        InputReader* mReader;

    public:
        ContextImpl(InputReader* reader);

        virtual void updateGlobalMetaState();
        virtual int32_t getGlobalMetaState();
        virtual void disableVirtualKeysUntil(nsecs_t time);
        virtual bool shouldDropVirtualKey(nsecs_t now,
                InputDevice* device, int32_t keyCode, int32_t scanCode);
        virtual void fadePointer();
        virtual void requestTimeoutAtTime(nsecs_t when);
        virtual InputReaderPolicyInterface* getPolicy();
        virtual InputListenerInterface* getListener();
        virtual EventHubInterface* getEventHub();
    } mContext;

    friend class ContextImpl;

private:
    Mutex mLock;

    sp<EventHubInterface> mEventHub;
    sp<InputReaderPolicyInterface> mPolicy;
    sp<QueuedInputListener> mQueuedListener;

    InputReaderConfiguration mConfig;

    
    static const int EVENT_BUFFER_SIZE = 256;
    RawEvent mEventBuffer[EVENT_BUFFER_SIZE];

    KeyedVector<int32_t, InputDevice*> mDevices;

    
    void processEventsLocked(const RawEvent* rawEvents, size_t count);

    void addDeviceLocked(nsecs_t when, int32_t deviceId);
    void removeDeviceLocked(nsecs_t when, int32_t deviceId);
    void processEventsForDeviceLocked(int32_t deviceId, const RawEvent* rawEvents, size_t count);
    void timeoutExpiredLocked(nsecs_t when);

    void handleConfigurationChangedLocked(nsecs_t when);

    int32_t mGlobalMetaState;
    void updateGlobalMetaStateLocked();
    int32_t getGlobalMetaStateLocked();

    void fadePointerLocked();

    InputConfiguration mInputConfiguration;
    void updateInputConfigurationLocked();

    nsecs_t mDisableVirtualKeysTimeout;
    void disableVirtualKeysUntilLocked(nsecs_t time);
    bool shouldDropVirtualKeyLocked(nsecs_t now,
            InputDevice* device, int32_t keyCode, int32_t scanCode);

    nsecs_t mNextTimeout;
    void requestTimeoutAtTimeLocked(nsecs_t when);

    uint32_t mConfigurationChangesToRefresh;
    void refreshConfigurationLocked(uint32_t changes);

    
    typedef int32_t (InputDevice::*GetStateFunc)(uint32_t sourceMask, int32_t code);
    int32_t getStateLocked(int32_t deviceId, uint32_t sourceMask, int32_t code,
            GetStateFunc getStateFunc);
    bool markSupportedKeyCodesLocked(int32_t deviceId, uint32_t sourceMask, size_t numCodes,
            const int32_t* keyCodes, uint8_t* outFlags);
};



class InputReaderThread : public Thread {
public:
    InputReaderThread(const sp<InputReaderInterface>& reader);
    virtual ~InputReaderThread();

private:
    uint32_t mFoo;
    sp<InputReaderInterface> mReader;

    virtual bool threadLoop();
};



class InputDevice {
public:
    InputDevice(InputReaderContext* context, int32_t id, const String8& name, uint32_t classes);
    ~InputDevice();

    inline InputReaderContext* getContext() { return mContext; }
    inline int32_t getId() { return mId; }
    inline const String8& getName() { return mName; }
    inline uint32_t getClasses() { return mClasses; }
    inline uint32_t getSources() { return mSources; }

    inline bool isExternal() { return mIsExternal; }
    inline void setExternal(bool external) { mIsExternal = external; }

    inline bool isIgnored() { return mMappers.isEmpty(); }

    void dump(String8& dump);
    void addMapper(InputMapper* mapper);
    void configure(nsecs_t when, const InputReaderConfiguration* config, uint32_t changes);
    void reset(nsecs_t when);
    void process(const RawEvent* rawEvents, size_t count);
    void timeoutExpired(nsecs_t when);

    void getDeviceInfo(InputDeviceInfo* outDeviceInfo);
    int32_t getKeyCodeState(uint32_t sourceMask, int32_t keyCode);
    int32_t getScanCodeState(uint32_t sourceMask, int32_t scanCode);
    int32_t getSwitchState(uint32_t sourceMask, int32_t switchCode);
    bool markSupportedKeyCodes(uint32_t sourceMask, size_t numCodes,
            const int32_t* keyCodes, uint8_t* outFlags);

    int32_t getMetaState();

    void fadePointer();

    void notifyReset(nsecs_t when);

    inline const PropertyMap& getConfiguration() { return mConfiguration; }
    inline EventHubInterface* getEventHub() { return mContext->getEventHub(); }

    bool hasKey(int32_t code) {
        return getEventHub()->hasScanCode(mId, code);
    }

    bool isKeyPressed(int32_t code) {
        return getEventHub()->getScanCodeState(mId, code) == AKEY_STATE_DOWN;
    }

    int32_t getAbsoluteAxisValue(int32_t code) {
        int32_t value;
        getEventHub()->getAbsoluteAxisValue(mId, code, &value);
        return value;
    }

private:
    InputReaderContext* mContext;
    int32_t mId;
    String8 mName;
    uint32_t mClasses;

    Vector<InputMapper*> mMappers;

    uint32_t mSources;
    bool mIsExternal;
    bool mDropUntilNextSync;

    typedef int32_t (InputMapper::*GetStateFunc)(uint32_t sourceMask, int32_t code);
    int32_t getState(uint32_t sourceMask, int32_t code, GetStateFunc getStateFunc);

    PropertyMap mConfiguration;
};



class CursorButtonAccumulator {
public:
    CursorButtonAccumulator();
    void reset(InputDevice* device);

    void process(const RawEvent* rawEvent);

    uint32_t getButtonState() const;

private:
    bool mBtnLeft;
    bool mBtnRight;
    bool mBtnMiddle;
    bool mBtnBack;
    bool mBtnSide;
    bool mBtnForward;
    bool mBtnExtra;
    bool mBtnTask;

    void clearButtons();
};




class CursorMotionAccumulator {
public:
    CursorMotionAccumulator();
    void reset(InputDevice* device);

    void process(const RawEvent* rawEvent);
    void finishSync();

    inline int32_t getRelativeX() const { return mRelX; }
    inline int32_t getRelativeY() const { return mRelY; }

private:
    int32_t mRelX;
    int32_t mRelY;

    void clearRelativeAxes();
};




class CursorScrollAccumulator {
public:
    CursorScrollAccumulator();
    void configure(InputDevice* device);
    void reset(InputDevice* device);

    void process(const RawEvent* rawEvent);
    void finishSync();

    inline bool haveRelativeVWheel() const { return mHaveRelWheel; }
    inline bool haveRelativeHWheel() const { return mHaveRelHWheel; }

    inline int32_t getRelativeX() const { return mRelX; }
    inline int32_t getRelativeY() const { return mRelY; }
    inline int32_t getRelativeVWheel() const { return mRelWheel; }
    inline int32_t getRelativeHWheel() const { return mRelHWheel; }

private:
    bool mHaveRelWheel;
    bool mHaveRelHWheel;

    int32_t mRelX;
    int32_t mRelY;
    int32_t mRelWheel;
    int32_t mRelHWheel;

    void clearRelativeAxes();
};



class TouchButtonAccumulator {
public:
    TouchButtonAccumulator();
    void configure(InputDevice* device);
    void reset(InputDevice* device);

    void process(const RawEvent* rawEvent);

    uint32_t getButtonState() const;
    int32_t getToolType() const;
    bool isToolActive() const;
    bool isHovering() const;

private:
    bool mHaveBtnTouch;

    bool mBtnTouch;
    bool mBtnStylus;
    bool mBtnStylus2;
    bool mBtnToolFinger;
    bool mBtnToolPen;
    bool mBtnToolRubber;
    bool mBtnToolBrush;
    bool mBtnToolPencil;
    bool mBtnToolAirbrush;
    bool mBtnToolMouse;
    bool mBtnToolLens;
    bool mBtnToolDoubleTap;
    bool mBtnToolTripleTap;
    bool mBtnToolQuadTap;

    void clearButtons();
};



struct RawPointerAxes {
    RawAbsoluteAxisInfo x;
    RawAbsoluteAxisInfo y;
    RawAbsoluteAxisInfo pressure;
    RawAbsoluteAxisInfo touchMajor;
    RawAbsoluteAxisInfo touchMinor;
    RawAbsoluteAxisInfo toolMajor;
    RawAbsoluteAxisInfo toolMinor;
    RawAbsoluteAxisInfo orientation;
    RawAbsoluteAxisInfo distance;
    RawAbsoluteAxisInfo tiltX;
    RawAbsoluteAxisInfo tiltY;
    RawAbsoluteAxisInfo trackingId;
    RawAbsoluteAxisInfo slot;

    RawPointerAxes();
    void clear();
};



struct RawPointerData {
    struct Pointer {
        uint32_t id;
        int32_t x;
        int32_t y;
        int32_t pressure;
        int32_t touchMajor;
        int32_t touchMinor;
        int32_t toolMajor;
        int32_t toolMinor;
        int32_t orientation;
        int32_t distance;
        int32_t tiltX;
        int32_t tiltY;
        int32_t toolType; 
        bool isHovering;
    };

    uint32_t pointerCount;
    Pointer pointers[MAX_POINTERS];
    BitSet32 hoveringIdBits, touchingIdBits;
    uint32_t idToIndex[MAX_POINTER_ID + 1];

    RawPointerData();
    void clear();
    void copyFrom(const RawPointerData& other);
    void getCentroidOfTouchingPointers(float* outX, float* outY) const;

    inline void markIdBit(uint32_t id, bool isHovering) {
        if (isHovering) {
            hoveringIdBits.markBit(id);
        } else {
            touchingIdBits.markBit(id);
        }
    }

    inline void clearIdBits() {
        hoveringIdBits.clear();
        touchingIdBits.clear();
    }

    inline const Pointer& pointerForId(uint32_t id) const {
        return pointers[idToIndex[id]];
    }

    inline bool isHovering(uint32_t pointerIndex) {
        return pointers[pointerIndex].isHovering;
    }
};



struct CookedPointerData {
    uint32_t pointerCount;
    PointerProperties pointerProperties[MAX_POINTERS];
    PointerCoords pointerCoords[MAX_POINTERS];
    BitSet32 hoveringIdBits, touchingIdBits;
    uint32_t idToIndex[MAX_POINTER_ID + 1];

    CookedPointerData();
    void clear();
    void copyFrom(const CookedPointerData& other);

    inline bool isHovering(uint32_t pointerIndex) {
        return hoveringIdBits.hasBit(pointerProperties[pointerIndex].id);
    }
};



class SingleTouchMotionAccumulator {
public:
    SingleTouchMotionAccumulator();

    void process(const RawEvent* rawEvent);
    void reset(InputDevice* device);

    inline int32_t getAbsoluteX() const { return mAbsX; }
    inline int32_t getAbsoluteY() const { return mAbsY; }
    inline int32_t getAbsolutePressure() const { return mAbsPressure; }
    inline int32_t getAbsoluteToolWidth() const { return mAbsToolWidth; }
    inline int32_t getAbsoluteDistance() const { return mAbsDistance; }
    inline int32_t getAbsoluteTiltX() const { return mAbsTiltX; }
    inline int32_t getAbsoluteTiltY() const { return mAbsTiltY; }

private:
    int32_t mAbsX;
    int32_t mAbsY;
    int32_t mAbsPressure;
    int32_t mAbsToolWidth;
    int32_t mAbsDistance;
    int32_t mAbsTiltX;
    int32_t mAbsTiltY;

    void clearAbsoluteAxes();
};



class MultiTouchMotionAccumulator {
public:
    class Slot {
    public:
        inline bool isInUse() const { return mInUse; }
        inline int32_t getX() const { return mAbsMTPositionX; }
        inline int32_t getY() const { return mAbsMTPositionY; }
        inline int32_t getTouchMajor() const { return mAbsMTTouchMajor; }
        inline int32_t getTouchMinor() const {
            return mHaveAbsMTTouchMinor ? mAbsMTTouchMinor : mAbsMTTouchMajor; }
        inline int32_t getToolMajor() const { return mAbsMTWidthMajor; }
        inline int32_t getToolMinor() const {
            return mHaveAbsMTWidthMinor ? mAbsMTWidthMinor : mAbsMTWidthMajor; }
        inline int32_t getOrientation() const { return mAbsMTOrientation; }
        inline int32_t getTrackingId() const { return mAbsMTTrackingId; }
        inline int32_t getPressure() const { return mAbsMTPressure; }
        inline int32_t getDistance() const { return mAbsMTDistance; }
        inline int32_t getToolType() const;

    private:
        friend class MultiTouchMotionAccumulator;

        bool mInUse;
        bool mHaveAbsMTTouchMinor;
        bool mHaveAbsMTWidthMinor;
        bool mHaveAbsMTToolType;

        int32_t mAbsMTPositionX;
        int32_t mAbsMTPositionY;
        int32_t mAbsMTTouchMajor;
        int32_t mAbsMTTouchMinor;
        int32_t mAbsMTWidthMajor;
        int32_t mAbsMTWidthMinor;
        int32_t mAbsMTOrientation;
        int32_t mAbsMTTrackingId;
        int32_t mAbsMTPressure;
        int32_t mAbsMTDistance;
        int32_t mAbsMTToolType;

        Slot();
        void clear();
    };

    MultiTouchMotionAccumulator();
    ~MultiTouchMotionAccumulator();

    void configure(size_t slotCount, bool usingSlotsProtocol);
    void reset(InputDevice* device);
    void process(const RawEvent* rawEvent);
    void finishSync();

    inline size_t getSlotCount() const { return mSlotCount; }
    inline const Slot* getSlot(size_t index) const { return &mSlots[index]; }

private:
    int32_t mCurrentSlot;
    Slot* mSlots;
    size_t mSlotCount;
    bool mUsingSlotsProtocol;

    void clearSlots(int32_t initialSlot);
};














class InputMapper {
public:
    InputMapper(InputDevice* device);
    virtual ~InputMapper();

    inline InputDevice* getDevice() { return mDevice; }
    inline int32_t getDeviceId() { return mDevice->getId(); }
    inline const String8 getDeviceName() { return mDevice->getName(); }
    inline InputReaderContext* getContext() { return mContext; }
    inline InputReaderPolicyInterface* getPolicy() { return mContext->getPolicy(); }
    inline InputListenerInterface* getListener() { return mContext->getListener(); }
    inline EventHubInterface* getEventHub() { return mContext->getEventHub(); }

    virtual uint32_t getSources() = 0;
    virtual void populateDeviceInfo(InputDeviceInfo* deviceInfo);
    virtual void dump(String8& dump);
    virtual void configure(nsecs_t when, const InputReaderConfiguration* config, uint32_t changes);
    virtual void reset(nsecs_t when);
    virtual void process(const RawEvent* rawEvent) = 0;
    virtual void timeoutExpired(nsecs_t when);

    virtual int32_t getKeyCodeState(uint32_t sourceMask, int32_t keyCode);
    virtual int32_t getScanCodeState(uint32_t sourceMask, int32_t scanCode);
    virtual int32_t getSwitchState(uint32_t sourceMask, int32_t switchCode);
    virtual bool markSupportedKeyCodes(uint32_t sourceMask, size_t numCodes,
            const int32_t* keyCodes, uint8_t* outFlags);

    virtual int32_t getMetaState();

    virtual void fadePointer();

protected:
    InputDevice* mDevice;
    InputReaderContext* mContext;

    status_t getAbsoluteAxisInfo(int32_t axis, RawAbsoluteAxisInfo* axisInfo);

    static void dumpRawAbsoluteAxisInfo(String8& dump,
            const RawAbsoluteAxisInfo& axis, const char* name);
};


class SwitchInputMapper : public InputMapper {
public:
    SwitchInputMapper(InputDevice* device);
    virtual ~SwitchInputMapper();

    virtual uint32_t getSources();
    virtual void process(const RawEvent* rawEvent);

    virtual int32_t getSwitchState(uint32_t sourceMask, int32_t switchCode);

private:
    void processSwitch(nsecs_t when, int32_t switchCode, int32_t switchValue);
};


class KeyboardInputMapper : public InputMapper {
public:
    KeyboardInputMapper(InputDevice* device, uint32_t source, int32_t keyboardType);
    virtual ~KeyboardInputMapper();

    virtual uint32_t getSources();
    virtual void populateDeviceInfo(InputDeviceInfo* deviceInfo);
    virtual void dump(String8& dump);
    virtual void configure(nsecs_t when, const InputReaderConfiguration* config, uint32_t changes);
    virtual void reset(nsecs_t when);
    virtual void process(const RawEvent* rawEvent);

    virtual int32_t getKeyCodeState(uint32_t sourceMask, int32_t keyCode);
    virtual int32_t getScanCodeState(uint32_t sourceMask, int32_t scanCode);
    virtual bool markSupportedKeyCodes(uint32_t sourceMask, size_t numCodes,
            const int32_t* keyCodes, uint8_t* outFlags);

    virtual int32_t getMetaState();

private:
    struct KeyDown {
        int32_t keyCode;
        int32_t scanCode;
    };

    uint32_t mSource;
    int32_t mKeyboardType;

    int32_t mOrientation; 

    Vector<KeyDown> mKeyDowns; 
    int32_t mMetaState;
    nsecs_t mDownTime; 

    struct LedState {
        bool avail; 
        bool on;    
    };
    LedState mCapsLockLedState;
    LedState mNumLockLedState;
    LedState mScrollLockLedState;

    
    struct Parameters {
        int32_t associatedDisplayId;
        bool orientationAware;
    } mParameters;

    void configureParameters();
    void dumpParameters(String8& dump);

    bool isKeyboardOrGamepadKey(int32_t scanCode);

    void processKey(nsecs_t when, bool down, int32_t keyCode, int32_t scanCode,
            uint32_t policyFlags);

    ssize_t findKeyDown(int32_t scanCode);

    void resetLedState();
    void initializeLedState(LedState& ledState, int32_t led);
    void updateLedState(bool reset);
    void updateLedStateForModifier(LedState& ledState, int32_t led,
            int32_t modifier, bool reset);
};


#ifdef HAVE_ANDROID_OS
class CursorInputMapper : public InputMapper {
public:
    CursorInputMapper(InputDevice* device);
    virtual ~CursorInputMapper();

    virtual uint32_t getSources();
    virtual void populateDeviceInfo(InputDeviceInfo* deviceInfo);
    virtual void dump(String8& dump);
    virtual void configure(nsecs_t when, const InputReaderConfiguration* config, uint32_t changes);
    virtual void reset(nsecs_t when);
    virtual void process(const RawEvent* rawEvent);

    virtual int32_t getScanCodeState(uint32_t sourceMask, int32_t scanCode);

    virtual void fadePointer();

private:
    
    static const int32_t TRACKBALL_MOVEMENT_THRESHOLD = 6;

    
    struct Parameters {
        enum Mode {
            MODE_POINTER,
            MODE_NAVIGATION,
        };

        Mode mode;
        int32_t associatedDisplayId;
        bool orientationAware;
    } mParameters;

    CursorButtonAccumulator mCursorButtonAccumulator;
    CursorMotionAccumulator mCursorMotionAccumulator;
    CursorScrollAccumulator mCursorScrollAccumulator;

    int32_t mSource;
    float mXScale;
    float mYScale;
    float mXPrecision;
    float mYPrecision;

    float mVWheelScale;
    float mHWheelScale;

    
    
    VelocityControl mPointerVelocityControl;
    VelocityControl mWheelXVelocityControl;
    VelocityControl mWheelYVelocityControl;

    int32_t mOrientation;

    sp<PointerControllerInterface> mPointerController;

    int32_t mButtonState;
    nsecs_t mDownTime;

    void configureParameters();
    void dumpParameters(String8& dump);

    void sync(nsecs_t when);
};
#endif 


class TouchInputMapper : public InputMapper {
public:
    TouchInputMapper(InputDevice* device);
    virtual ~TouchInputMapper();

    virtual uint32_t getSources();
    virtual void populateDeviceInfo(InputDeviceInfo* deviceInfo);
    virtual void dump(String8& dump);
    virtual void configure(nsecs_t when, const InputReaderConfiguration* config, uint32_t changes);
    virtual void reset(nsecs_t when);
    virtual void process(const RawEvent* rawEvent);

    virtual int32_t getKeyCodeState(uint32_t sourceMask, int32_t keyCode);
    virtual int32_t getScanCodeState(uint32_t sourceMask, int32_t scanCode);
    virtual bool markSupportedKeyCodes(uint32_t sourceMask, size_t numCodes,
            const int32_t* keyCodes, uint8_t* outFlags);

    virtual void fadePointer();
    virtual void timeoutExpired(nsecs_t when);

protected:
    CursorButtonAccumulator mCursorButtonAccumulator;
    CursorScrollAccumulator mCursorScrollAccumulator;
    TouchButtonAccumulator mTouchButtonAccumulator;

    struct VirtualKey {
        int32_t keyCode;
        int32_t scanCode;
        uint32_t flags;

        
        int32_t hitLeft;
        int32_t hitTop;
        int32_t hitRight;
        int32_t hitBottom;

        inline bool isHit(int32_t x, int32_t y) const {
            return x >= hitLeft && x <= hitRight && y >= hitTop && y <= hitBottom;
        }
    };

    
    uint32_t mSource;

    enum DeviceMode {
        DEVICE_MODE_DISABLED, 
        DEVICE_MODE_DIRECT, 
        DEVICE_MODE_UNSCALED, 
        DEVICE_MODE_POINTER, 
    };
    DeviceMode mDeviceMode;

    
    InputReaderConfiguration mConfig;

    
    struct Parameters {
        enum DeviceType {
            DEVICE_TYPE_TOUCH_SCREEN,
            DEVICE_TYPE_TOUCH_PAD,
            DEVICE_TYPE_POINTER,
        };

        DeviceType deviceType;
        int32_t associatedDisplayId;
        bool associatedDisplayIsExternal;
        bool orientationAware;

        enum GestureMode {
            GESTURE_MODE_POINTER,
            GESTURE_MODE_SPOTS,
        };
        GestureMode gestureMode;
    } mParameters;

    
    struct Calibration {
        
        enum SizeCalibration {
            SIZE_CALIBRATION_DEFAULT,
            SIZE_CALIBRATION_NONE,
            SIZE_CALIBRATION_GEOMETRIC,
            SIZE_CALIBRATION_DIAMETER,
            SIZE_CALIBRATION_AREA,
        };

        SizeCalibration sizeCalibration;

        bool haveSizeScale;
        float sizeScale;
        bool haveSizeBias;
        float sizeBias;
        bool haveSizeIsSummed;
        bool sizeIsSummed;

        
        enum PressureCalibration {
            PRESSURE_CALIBRATION_DEFAULT,
            PRESSURE_CALIBRATION_NONE,
            PRESSURE_CALIBRATION_PHYSICAL,
            PRESSURE_CALIBRATION_AMPLITUDE,
        };

        PressureCalibration pressureCalibration;
        bool havePressureScale;
        float pressureScale;

        
        enum OrientationCalibration {
            ORIENTATION_CALIBRATION_DEFAULT,
            ORIENTATION_CALIBRATION_NONE,
            ORIENTATION_CALIBRATION_INTERPOLATED,
            ORIENTATION_CALIBRATION_VECTOR,
        };

        OrientationCalibration orientationCalibration;

        
        enum DistanceCalibration {
            DISTANCE_CALIBRATION_DEFAULT,
            DISTANCE_CALIBRATION_NONE,
            DISTANCE_CALIBRATION_SCALED,
        };

        DistanceCalibration distanceCalibration;
        bool haveDistanceScale;
        float distanceScale;

        inline void applySizeScaleAndBias(float* outSize) const {
            if (haveSizeScale) {
                *outSize *= sizeScale;
            }
            if (haveSizeBias) {
                *outSize += sizeBias;
            }
        }
    } mCalibration;

    
    RawPointerAxes mRawPointerAxes;

    
    RawPointerData mCurrentRawPointerData;
    RawPointerData mLastRawPointerData;

    
    CookedPointerData mCurrentCookedPointerData;
    CookedPointerData mLastCookedPointerData;

    
    int32_t mCurrentButtonState;
    int32_t mLastButtonState;

    
    int32_t mCurrentRawVScroll;
    int32_t mCurrentRawHScroll;

    
    BitSet32 mCurrentFingerIdBits; 
    BitSet32 mLastFingerIdBits;
    BitSet32 mCurrentStylusIdBits; 
    BitSet32 mLastStylusIdBits;
    BitSet32 mCurrentMouseIdBits; 
    BitSet32 mLastMouseIdBits;

    
    bool mSentHoverEnter;

    
    nsecs_t mDownTime;

#ifdef HAVE_ANDROID_OS
    
    sp<PointerControllerInterface> mPointerController;
#endif

    Vector<VirtualKey> mVirtualKeys;

    virtual void configureParameters();
    virtual void dumpParameters(String8& dump);
    virtual void configureRawPointerAxes();
    virtual void dumpRawPointerAxes(String8& dump);
    virtual void configureSurface(nsecs_t when, bool* outResetNeeded);
    virtual void dumpSurface(String8& dump);
    virtual void configureVirtualKeys();
    virtual void dumpVirtualKeys(String8& dump);
    virtual void parseCalibration();
    virtual void resolveCalibration();
    virtual void dumpCalibration(String8& dump);

    virtual void syncTouch(nsecs_t when, bool* outHavePointerIds) = 0;

private:
    
    int32_t mSurfaceOrientation;
    int32_t mSurfaceWidth;
    int32_t mSurfaceHeight;

    
    int32_t mAssociatedDisplayOrientation;
    int32_t mAssociatedDisplayWidth;
    int32_t mAssociatedDisplayHeight;

    
    float mXScale;
    float mXPrecision;

    float mYScale;
    float mYPrecision;

    float mGeometricScale;

    float mPressureScale;

    float mSizeScale;

    float mOrientationCenter;
    float mOrientationScale;

    float mDistanceScale;

    bool mHaveTilt;
    float mTiltXCenter;
    float mTiltXScale;
    float mTiltYCenter;
    float mTiltYScale;

    
    struct OrientedRanges {
        InputDeviceInfo::MotionRange x;
        InputDeviceInfo::MotionRange y;
        InputDeviceInfo::MotionRange pressure;

        bool haveSize;
        InputDeviceInfo::MotionRange size;

        bool haveTouchSize;
        InputDeviceInfo::MotionRange touchMajor;
        InputDeviceInfo::MotionRange touchMinor;

        bool haveToolSize;
        InputDeviceInfo::MotionRange toolMajor;
        InputDeviceInfo::MotionRange toolMinor;

        bool haveOrientation;
        InputDeviceInfo::MotionRange orientation;

        bool haveDistance;
        InputDeviceInfo::MotionRange distance;

        bool haveTilt;
        InputDeviceInfo::MotionRange tilt;

        OrientedRanges() {
            clear();
        }

        void clear() {
            haveSize = false;
            haveTouchSize = false;
            haveToolSize = false;
            haveOrientation = false;
            haveDistance = false;
            haveTilt = false;
        }
    } mOrientedRanges;

    
    float mOrientedSurfaceWidth;
    float mOrientedSurfaceHeight;
    float mOrientedXPrecision;
    float mOrientedYPrecision;

    struct CurrentVirtualKeyState {
        bool down;
        bool ignored;
        nsecs_t downTime;
        int32_t keyCode;
        int32_t scanCode;
    } mCurrentVirtualKey;

    
    float mPointerXMovementScale;
    float mPointerYMovementScale;

    
    float mPointerXZoomScale;
    float mPointerYZoomScale;

    
    float mPointerGestureMaxSwipeWidth;

    struct PointerDistanceHeapElement {
        uint32_t currentPointerIndex : 8;
        uint32_t lastPointerIndex : 8;
        uint64_t distance : 48; 
    };

    enum PointerUsage {
        POINTER_USAGE_NONE,
        POINTER_USAGE_GESTURES,
        POINTER_USAGE_STYLUS,
        POINTER_USAGE_MOUSE,
    };
    PointerUsage mPointerUsage;

    struct PointerGesture {
        enum Mode {
            
            
            NEUTRAL,

            
            
            
            TAP,

            
            
            
            
            
            TAP_DRAG,

            
            
            
            BUTTON_CLICK_OR_DRAG,

            
            
            
            
            
            HOVER,

            
            
            
            
            
            PRESS,

            
            
            
            
            SWIPE,

            
            
            
            
            FREEFORM,

            
            QUIET,
        };

        
        nsecs_t firstTouchTime;

        
        int32_t activeTouchId; 

        
        int32_t activeGestureId; 

        
        Mode currentGestureMode;
        BitSet32 currentGestureIdBits;
        uint32_t currentGestureIdToIndex[MAX_POINTER_ID + 1];
        PointerProperties currentGestureProperties[MAX_POINTERS];
        PointerCoords currentGestureCoords[MAX_POINTERS];

        Mode lastGestureMode;
        BitSet32 lastGestureIdBits;
        uint32_t lastGestureIdToIndex[MAX_POINTER_ID + 1];
        PointerProperties lastGestureProperties[MAX_POINTERS];
        PointerCoords lastGestureCoords[MAX_POINTERS];

        
        nsecs_t downTime;

        
        nsecs_t tapDownTime;

        
        nsecs_t tapUpTime;

        
        float tapX, tapY;

        
        nsecs_t quietTime;

        
        float referenceTouchX;    
        float referenceTouchY;
        float referenceGestureX;  
        float referenceGestureY;

        
        
        BitSet32 referenceIdBits;
        struct Delta {
            float dx, dy;
        };
        Delta referenceDeltas[MAX_POINTER_ID + 1];

        
        uint32_t freeformTouchToGestureIdMap[MAX_POINTER_ID + 1];

        
        VelocityTracker velocityTracker;

        void reset() {
            firstTouchTime = LLONG_MIN;
            activeTouchId = -1;
            activeGestureId = -1;
            currentGestureMode = NEUTRAL;
            currentGestureIdBits.clear();
            lastGestureMode = NEUTRAL;
            lastGestureIdBits.clear();
            downTime = 0;
            velocityTracker.clear();
            resetTap();
            resetQuietTime();
        }

        void resetTap() {
            tapDownTime = LLONG_MIN;
            tapUpTime = LLONG_MIN;
        }

        void resetQuietTime() {
            quietTime = LLONG_MIN;
        }
    } mPointerGesture;

    struct PointerSimple {
        PointerCoords currentCoords;
        PointerProperties currentProperties;
        PointerCoords lastCoords;
        PointerProperties lastProperties;

        
        bool down;

        
        bool hovering;

        
        nsecs_t downTime;

        void reset() {
            currentCoords.clear();
            currentProperties.clear();
            lastCoords.clear();
            lastProperties.clear();
            down = false;
            hovering = false;
            downTime = 0;
        }
    } mPointerSimple;

    
    VelocityControl mPointerVelocityControl;
    VelocityControl mWheelXVelocityControl;
    VelocityControl mWheelYVelocityControl;

    void sync(nsecs_t when);

    bool consumeRawTouches(nsecs_t when, uint32_t policyFlags);
    void dispatchVirtualKey(nsecs_t when, uint32_t policyFlags,
            int32_t keyEventAction, int32_t keyEventFlags);

    void dispatchTouches(nsecs_t when, uint32_t policyFlags);
    void dispatchHoverExit(nsecs_t when, uint32_t policyFlags);
    void dispatchHoverEnterAndMove(nsecs_t when, uint32_t policyFlags);
    void cookPointerData();

    void dispatchPointerUsage(nsecs_t when, uint32_t policyFlags, PointerUsage pointerUsage);
    void abortPointerUsage(nsecs_t when, uint32_t policyFlags);

    void dispatchPointerGestures(nsecs_t when, uint32_t policyFlags, bool isTimeout);
    void abortPointerGestures(nsecs_t when, uint32_t policyFlags);
    bool preparePointerGestures(nsecs_t when,
            bool* outCancelPreviousGesture, bool* outFinishPreviousGesture,
            bool isTimeout);

    void dispatchPointerStylus(nsecs_t when, uint32_t policyFlags);
    void abortPointerStylus(nsecs_t when, uint32_t policyFlags);

    void dispatchPointerMouse(nsecs_t when, uint32_t policyFlags);
    void abortPointerMouse(nsecs_t when, uint32_t policyFlags);

    void dispatchPointerSimple(nsecs_t when, uint32_t policyFlags,
            bool down, bool hovering);
    void abortPointerSimple(nsecs_t when, uint32_t policyFlags);

    
    
    
    
    void dispatchMotion(nsecs_t when, uint32_t policyFlags, uint32_t source,
            int32_t action, int32_t flags, int32_t metaState, int32_t buttonState,
            int32_t edgeFlags,
            const PointerProperties* properties, const PointerCoords* coords,
            const uint32_t* idToIndex, BitSet32 idBits,
            int32_t changedId, float xPrecision, float yPrecision, nsecs_t downTime);

    
    
    bool updateMovedPointers(const PointerProperties* inProperties,
            const PointerCoords* inCoords, const uint32_t* inIdToIndex,
            PointerProperties* outProperties, PointerCoords* outCoords,
            const uint32_t* outIdToIndex, BitSet32 idBits) const;

    bool isPointInsideSurface(int32_t x, int32_t y);
    const VirtualKey* findVirtualKeyHit(int32_t x, int32_t y);

    void assignPointerIds();
};


class SingleTouchInputMapper : public TouchInputMapper {
public:
    SingleTouchInputMapper(InputDevice* device);
    virtual ~SingleTouchInputMapper();

    virtual void reset(nsecs_t when);
    virtual void process(const RawEvent* rawEvent);

protected:
    virtual void syncTouch(nsecs_t when, bool* outHavePointerIds);
    virtual void configureRawPointerAxes();

private:
    SingleTouchMotionAccumulator mSingleTouchMotionAccumulator;
};


class MultiTouchInputMapper : public TouchInputMapper {
public:
    MultiTouchInputMapper(InputDevice* device);
    virtual ~MultiTouchInputMapper();

    virtual void reset(nsecs_t when);
    virtual void process(const RawEvent* rawEvent);

protected:
    virtual void syncTouch(nsecs_t when, bool* outHavePointerIds);
    virtual void configureRawPointerAxes();

private:
    MultiTouchMotionAccumulator mMultiTouchMotionAccumulator;

    
    BitSet32 mPointerIdBits;
    int32_t mPointerTrackingIdMap[MAX_POINTER_ID + 1];
};


class JoystickInputMapper : public InputMapper {
public:
    JoystickInputMapper(InputDevice* device);
    virtual ~JoystickInputMapper();

    virtual uint32_t getSources();
    virtual void populateDeviceInfo(InputDeviceInfo* deviceInfo);
    virtual void dump(String8& dump);
    virtual void configure(nsecs_t when, const InputReaderConfiguration* config, uint32_t changes);
    virtual void reset(nsecs_t when);
    virtual void process(const RawEvent* rawEvent);

private:
    struct Axis {
        RawAbsoluteAxisInfo rawAxisInfo;
        AxisInfo axisInfo;

        bool explicitlyMapped; 

        float scale;   
        float offset;  
        float highScale;  
        float highOffset; 

        float min;     
        float max;     
        float flat;    
        float fuzz;    

        float filter;  
        float currentValue; 
        float newValue; 
        float highCurrentValue; 
        float highNewValue; 

        void initialize(const RawAbsoluteAxisInfo& rawAxisInfo, const AxisInfo& axisInfo,
                bool explicitlyMapped, float scale, float offset,
                float highScale, float highOffset,
                float min, float max, float flat, float fuzz) {
            this->rawAxisInfo = rawAxisInfo;
            this->axisInfo = axisInfo;
            this->explicitlyMapped = explicitlyMapped;
            this->scale = scale;
            this->offset = offset;
            this->highScale = highScale;
            this->highOffset = highOffset;
            this->min = min;
            this->max = max;
            this->flat = flat;
            this->fuzz = fuzz;
            this->filter = 0;
            resetValue();
        }

        void resetValue() {
            this->currentValue = 0;
            this->newValue = 0;
            this->highCurrentValue = 0;
            this->highNewValue = 0;
        }
    };

    
    KeyedVector<int32_t, Axis> mAxes;

    void sync(nsecs_t when, bool force);

    bool haveAxis(int32_t axisId);
    void pruneAxes(bool ignoreExplicitlyMappedAxes);
    bool filterAxes(bool force);

    static bool hasValueChangedSignificantly(float filter,
            float newValue, float currentValue, float min, float max);
    static bool hasMovedNearerToValueWithinFilteredRange(float filter,
            float newValue, float currentValue, float thresholdValue);

    static bool isCenteredAxis(int32_t axis);
};

} 

#endif 
