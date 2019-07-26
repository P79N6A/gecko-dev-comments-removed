















#ifndef _ANDROIDFW_INPUT_H
#define _ANDROIDFW_INPUT_H





#include "android_input.h"
#include <utils/Vector.h>
#include <utils/KeyedVector.h>
#include <utils/Timers.h>
#include <utils/RefBase.h>
#include <utils/String8.h>

#ifdef HAVE_ANDROID_OS
class SkMatrix;
#endif




enum {
    
    AKEY_EVENT_FLAG_PREDISPATCH = 0x20000000,

    
    AKEY_EVENT_FLAG_START_TRACKING = 0x40000000,

    
    AKEY_EVENT_FLAG_TAINTED = 0x80000000,
};

enum {
    
    AMOTION_EVENT_FLAG_TAINTED = 0x80000000,
};

enum {
    

    ADISPLAY_ID_NONE = -1,

    
    ADISPLAY_ID_DEFAULT = 0,
};

enum {
    




    AINPUT_SOURCE_SWITCH = 0x80000000,
};




enum {
    ASYSTEM_UI_VISIBILITY_STATUS_BAR_VISIBLE = 0,
    ASYSTEM_UI_VISIBILITY_STATUS_BAR_HIDDEN = 0x00000001,
};







#define MAX_POINTERS 16






#define MAX_POINTER_ID 31




struct AInputEvent {
    virtual ~AInputEvent() { }
};




struct AInputDevice {
    virtual ~AInputDevice() { }
};


namespace android {

#ifdef HAVE_ANDROID_OS
class Parcel;
#endif







enum {
    


    POLICY_FLAG_WAKE = 0x00000001,
    POLICY_FLAG_WAKE_DROPPED = 0x00000002,
    POLICY_FLAG_SHIFT = 0x00000004,
    POLICY_FLAG_CAPS_LOCK = 0x00000008,
    POLICY_FLAG_ALT = 0x00000010,
    POLICY_FLAG_ALT_GR = 0x00000020,
    POLICY_FLAG_MENU = 0x00000040,
    POLICY_FLAG_LAUNCHER = 0x00000080,
    POLICY_FLAG_VIRTUAL = 0x00000100,
    POLICY_FLAG_FUNCTION = 0x00000200,

    POLICY_FLAG_RAW_MASK = 0x0000ffff,

    

    
    POLICY_FLAG_INJECTED = 0x01000000,

    
    
    POLICY_FLAG_TRUSTED = 0x02000000,

    
    POLICY_FLAG_FILTERED = 0x04000000,

    
    POLICY_FLAG_DISABLE_KEY_REPEAT = 0x08000000,

    

    
    
    POLICY_FLAG_WOKE_HERE = 0x10000000,

    
    
    POLICY_FLAG_BRIGHT_HERE = 0x20000000,

    
    
    
    POLICY_FLAG_PASS_TO_USER = 0x40000000,
};




struct PointerCoords {
    enum { MAX_AXES = 14 }; 

    
    uint64_t bits;

    
    
    float values[MAX_AXES];

    inline void clear() {
        bits = 0;
    }

    float getAxisValue(int32_t axis) const;
    status_t setAxisValue(int32_t axis, float value);

    void scale(float scale);

    inline float getX() const {
        return getAxisValue(AMOTION_EVENT_AXIS_X);
    }

    inline float getY() const {
        return getAxisValue(AMOTION_EVENT_AXIS_Y);
    }

#ifdef HAVE_ANDROID_OS
    status_t readFromParcel(Parcel* parcel);
    status_t writeToParcel(Parcel* parcel) const;
#endif

    bool operator==(const PointerCoords& other) const;
    inline bool operator!=(const PointerCoords& other) const {
        return !(*this == other);
    }

    void copyFrom(const PointerCoords& other);

private:
    void tooManyAxes(int axis);
};




struct PointerProperties {
    
    int32_t id;

    
    int32_t toolType;

    inline void clear() {
        id = -1;
        toolType = 0;
    }

    bool operator==(const PointerProperties& other) const;
    inline bool operator!=(const PointerProperties& other) const {
        return !(*this == other);
    }

    void copyFrom(const PointerProperties& other);
};




class InputEvent : public AInputEvent {
public:
    virtual ~InputEvent() { }

    virtual int32_t getType() const = 0;

    inline int32_t getDeviceId() const { return mDeviceId; }

    inline int32_t getSource() const { return mSource; }

    inline void setSource(int32_t source) { mSource = source; }

protected:
    void initialize(int32_t deviceId, int32_t source);
    void initialize(const InputEvent& from);

    int32_t mDeviceId;
    int32_t mSource;
};




class KeyEvent : public InputEvent {
public:
    virtual ~KeyEvent() { }

    virtual int32_t getType() const { return AINPUT_EVENT_TYPE_KEY; }

    inline int32_t getAction() const { return mAction; }

    inline int32_t getFlags() const { return mFlags; }

    inline void setFlags(int32_t flags) { mFlags = flags; }

    inline int32_t getKeyCode() const { return mKeyCode; }

    inline int32_t getScanCode() const { return mScanCode; }

    inline int32_t getMetaState() const { return mMetaState; }

    inline int32_t getRepeatCount() const { return mRepeatCount; }

    inline nsecs_t getDownTime() const { return mDownTime; }

    inline nsecs_t getEventTime() const { return mEventTime; }

    
    static bool hasDefaultAction(int32_t keyCode);
    bool hasDefaultAction() const;

    
    static bool isSystemKey(int32_t keyCode);
    bool isSystemKey() const;
    
    void initialize(
            int32_t deviceId,
            int32_t source,
            int32_t action,
            int32_t flags,
            int32_t keyCode,
            int32_t scanCode,
            int32_t metaState,
            int32_t repeatCount,
            nsecs_t downTime,
            nsecs_t eventTime);
    void initialize(const KeyEvent& from);

protected:
    int32_t mAction;
    int32_t mFlags;
    int32_t mKeyCode;
    int32_t mScanCode;
    int32_t mMetaState;
    int32_t mRepeatCount;
    nsecs_t mDownTime;
    nsecs_t mEventTime;
};




class MotionEvent : public InputEvent {
public:
    virtual ~MotionEvent() { }

    virtual int32_t getType() const { return AINPUT_EVENT_TYPE_MOTION; }

    inline int32_t getAction() const { return mAction; }

    inline int32_t getActionMasked() const { return mAction & AMOTION_EVENT_ACTION_MASK; }

    inline int32_t getActionIndex() const {
        return (mAction & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
    }

    inline void setAction(int32_t action) { mAction = action; }

    inline int32_t getFlags() const { return mFlags; }

    inline void setFlags(int32_t flags) { mFlags = flags; }

    inline int32_t getEdgeFlags() const { return mEdgeFlags; }

    inline void setEdgeFlags(int32_t edgeFlags) { mEdgeFlags = edgeFlags; }

    inline int32_t getMetaState() const { return mMetaState; }

    inline void setMetaState(int32_t metaState) { mMetaState = metaState; }

    inline int32_t getButtonState() const { return mButtonState; }

    inline float getXOffset() const { return mXOffset; }

    inline float getYOffset() const { return mYOffset; }

    inline float getXPrecision() const { return mXPrecision; }

    inline float getYPrecision() const { return mYPrecision; }

    inline nsecs_t getDownTime() const { return mDownTime; }

    inline void setDownTime(nsecs_t downTime) { mDownTime = downTime; }

    inline size_t getPointerCount() const { return mPointerProperties.size(); }

    inline const PointerProperties* getPointerProperties(size_t pointerIndex) const {
        return &mPointerProperties[pointerIndex];
    }

    inline int32_t getPointerId(size_t pointerIndex) const {
        return mPointerProperties[pointerIndex].id;
    }

    inline int32_t getToolType(size_t pointerIndex) const {
        return mPointerProperties[pointerIndex].toolType;
    }

    inline nsecs_t getEventTime() const { return mSampleEventTimes[getHistorySize()]; }

    const PointerCoords* getRawPointerCoords(size_t pointerIndex) const;

    float getRawAxisValue(int32_t axis, size_t pointerIndex) const;

    inline float getRawX(size_t pointerIndex) const {
        return getRawAxisValue(AMOTION_EVENT_AXIS_X, pointerIndex);
    }

    inline float getRawY(size_t pointerIndex) const {
        return getRawAxisValue(AMOTION_EVENT_AXIS_Y, pointerIndex);
    }

    float getAxisValue(int32_t axis, size_t pointerIndex) const;

    inline float getX(size_t pointerIndex) const {
        return getAxisValue(AMOTION_EVENT_AXIS_X, pointerIndex);
    }

    inline float getY(size_t pointerIndex) const {
        return getAxisValue(AMOTION_EVENT_AXIS_Y, pointerIndex);
    }

    inline float getPressure(size_t pointerIndex) const {
        return getAxisValue(AMOTION_EVENT_AXIS_PRESSURE, pointerIndex);
    }

    inline float getSize(size_t pointerIndex) const {
        return getAxisValue(AMOTION_EVENT_AXIS_SIZE, pointerIndex);
    }

    inline float getTouchMajor(size_t pointerIndex) const {
        return getAxisValue(AMOTION_EVENT_AXIS_TOUCH_MAJOR, pointerIndex);
    }

    inline float getTouchMinor(size_t pointerIndex) const {
        return getAxisValue(AMOTION_EVENT_AXIS_TOUCH_MINOR, pointerIndex);
    }

    inline float getToolMajor(size_t pointerIndex) const {
        return getAxisValue(AMOTION_EVENT_AXIS_TOOL_MAJOR, pointerIndex);
    }

    inline float getToolMinor(size_t pointerIndex) const {
        return getAxisValue(AMOTION_EVENT_AXIS_TOOL_MINOR, pointerIndex);
    }

    inline float getOrientation(size_t pointerIndex) const {
        return getAxisValue(AMOTION_EVENT_AXIS_ORIENTATION, pointerIndex);
    }

    inline size_t getHistorySize() const { return mSampleEventTimes.size() - 1; }

    inline nsecs_t getHistoricalEventTime(size_t historicalIndex) const {
        return mSampleEventTimes[historicalIndex];
    }

    const PointerCoords* getHistoricalRawPointerCoords(
            size_t pointerIndex, size_t historicalIndex) const;

    float getHistoricalRawAxisValue(int32_t axis, size_t pointerIndex,
            size_t historicalIndex) const;

    inline float getHistoricalRawX(size_t pointerIndex, size_t historicalIndex) const {
        return getHistoricalRawAxisValue(
                AMOTION_EVENT_AXIS_X, pointerIndex, historicalIndex);
    }

    inline float getHistoricalRawY(size_t pointerIndex, size_t historicalIndex) const {
        return getHistoricalRawAxisValue(
                AMOTION_EVENT_AXIS_Y, pointerIndex, historicalIndex);
    }

    float getHistoricalAxisValue(int32_t axis, size_t pointerIndex, size_t historicalIndex) const;

    inline float getHistoricalX(size_t pointerIndex, size_t historicalIndex) const {
        return getHistoricalAxisValue(
                AMOTION_EVENT_AXIS_X, pointerIndex, historicalIndex);
    }

    inline float getHistoricalY(size_t pointerIndex, size_t historicalIndex) const {
        return getHistoricalAxisValue(
                AMOTION_EVENT_AXIS_Y, pointerIndex, historicalIndex);
    }

    inline float getHistoricalPressure(size_t pointerIndex, size_t historicalIndex) const {
        return getHistoricalAxisValue(
                AMOTION_EVENT_AXIS_PRESSURE, pointerIndex, historicalIndex);
    }

    inline float getHistoricalSize(size_t pointerIndex, size_t historicalIndex) const {
        return getHistoricalAxisValue(
                AMOTION_EVENT_AXIS_SIZE, pointerIndex, historicalIndex);
    }

    inline float getHistoricalTouchMajor(size_t pointerIndex, size_t historicalIndex) const {
        return getHistoricalAxisValue(
                AMOTION_EVENT_AXIS_TOUCH_MAJOR, pointerIndex, historicalIndex);
    }

    inline float getHistoricalTouchMinor(size_t pointerIndex, size_t historicalIndex) const {
        return getHistoricalAxisValue(
                AMOTION_EVENT_AXIS_TOUCH_MINOR, pointerIndex, historicalIndex);
    }

    inline float getHistoricalToolMajor(size_t pointerIndex, size_t historicalIndex) const {
        return getHistoricalAxisValue(
                AMOTION_EVENT_AXIS_TOOL_MAJOR, pointerIndex, historicalIndex);
    }

    inline float getHistoricalToolMinor(size_t pointerIndex, size_t historicalIndex) const {
        return getHistoricalAxisValue(
                AMOTION_EVENT_AXIS_TOOL_MINOR, pointerIndex, historicalIndex);
    }

    inline float getHistoricalOrientation(size_t pointerIndex, size_t historicalIndex) const {
        return getHistoricalAxisValue(
                AMOTION_EVENT_AXIS_ORIENTATION, pointerIndex, historicalIndex);
    }

    ssize_t findPointerIndex(int32_t pointerId) const;

    void initialize(
            int32_t deviceId,
            int32_t source,
            int32_t action,
            int32_t flags,
            int32_t edgeFlags,
            int32_t metaState,
            int32_t buttonState,
            float xOffset,
            float yOffset,
            float xPrecision,
            float yPrecision,
            nsecs_t downTime,
            nsecs_t eventTime,
            size_t pointerCount,
            const PointerProperties* pointerProperties,
            const PointerCoords* pointerCoords);

    void copyFrom(const MotionEvent* other, bool keepHistory);

    void addSample(
            nsecs_t eventTime,
            const PointerCoords* pointerCoords);

    void offsetLocation(float xOffset, float yOffset);

    void scale(float scaleFactor);

#ifdef HAVE_ANDROID_OS
    void transform(const SkMatrix* matrix);

    status_t readFromParcel(Parcel* parcel);
    status_t writeToParcel(Parcel* parcel) const;
#endif

    static bool isTouchEvent(int32_t source, int32_t action);
    inline bool isTouchEvent() const {
        return isTouchEvent(mSource, mAction);
    }

    
    inline const PointerProperties* getPointerProperties() const {
        return mPointerProperties.array();
    }
    inline const nsecs_t* getSampleEventTimes() const { return mSampleEventTimes.array(); }
    inline const PointerCoords* getSamplePointerCoords() const {
            return mSamplePointerCoords.array();
    }

protected:
    int32_t mAction;
    int32_t mFlags;
    int32_t mEdgeFlags;
    int32_t mMetaState;
    int32_t mButtonState;
    float mXOffset;
    float mYOffset;
    float mXPrecision;
    float mYPrecision;
    nsecs_t mDownTime;
    Vector<PointerProperties> mPointerProperties;
    Vector<nsecs_t> mSampleEventTimes;
    Vector<PointerCoords> mSamplePointerCoords;
};




class InputEventFactoryInterface {
protected:
    virtual ~InputEventFactoryInterface() { }

public:
    InputEventFactoryInterface() { }

    virtual KeyEvent* createKeyEvent() = 0;
    virtual MotionEvent* createMotionEvent() = 0;
};





class PreallocatedInputEventFactory : public InputEventFactoryInterface {
public:
    PreallocatedInputEventFactory() { }
    virtual ~PreallocatedInputEventFactory() { }

    virtual KeyEvent* createKeyEvent() { return & mKeyEvent; }
    virtual MotionEvent* createMotionEvent() { return & mMotionEvent; }

private:
    KeyEvent mKeyEvent;
    MotionEvent mMotionEvent;
};




class PooledInputEventFactory : public InputEventFactoryInterface {
public:
    PooledInputEventFactory(size_t maxPoolSize = 20);
    virtual ~PooledInputEventFactory();

    virtual KeyEvent* createKeyEvent();
    virtual MotionEvent* createMotionEvent();

    void recycle(InputEvent* event);

private:
    const size_t mMaxPoolSize;

    Vector<KeyEvent*> mKeyEventPool;
    Vector<MotionEvent*> mMotionEventPool;
};

} 

#endif 
