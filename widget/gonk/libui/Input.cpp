




#define LOG_TAG "Input"




#define DEBUG_PROBE 0


#define DEBUG_VELOCITY 0


#define DEBUG_LEAST_SQUARES 0


#define DEBUG_ACCELERATION 0


#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include "utils_Log.h"
#include "Input.h"

#include <math.h>
#include <limits.h>

#ifdef HAVE_ANDROID_OS
#include <binder/Parcel.h>

#include "SkPoint.h"
#include "SkMatrix.h"
#include "SkScalar.h"
#endif

namespace android {

static const char* CONFIGURATION_FILE_DIR[] = {
        "idc/",
        "keylayout/",
        "keychars/",
};

static const char* CONFIGURATION_FILE_EXTENSION[] = {
        ".idc",
        ".kl",
        ".kcm",
};

static bool isValidNameChar(char ch) {
    return isascii(ch) && (isdigit(ch) || isalpha(ch) || ch == '-' || ch == '_');
}

static void appendInputDeviceConfigurationFileRelativePath(String8& path,
        const String8& name, InputDeviceConfigurationFileType type) {
    path.append(CONFIGURATION_FILE_DIR[type]);
    for (size_t i = 0; i < name.length(); i++) {
        char ch = name[i];
        if (!isValidNameChar(ch)) {
            ch = '_';
        }
        path.append(&ch, 1);
    }
    path.append(CONFIGURATION_FILE_EXTENSION[type]);
}

String8 getInputDeviceConfigurationFilePathByDeviceIdentifier(
        const InputDeviceIdentifier& deviceIdentifier,
        InputDeviceConfigurationFileType type) {
    if (deviceIdentifier.vendor !=0 && deviceIdentifier.product != 0) {
        if (deviceIdentifier.version != 0) {
            
            String8 versionPath(getInputDeviceConfigurationFilePathByName(
                    String8::format("Vendor_%04x_Product_%04x_Version_%04x",
                            deviceIdentifier.vendor, deviceIdentifier.product,
                            deviceIdentifier.version),
                    type));
            if (!versionPath.isEmpty()) {
                return versionPath;
            }
        }

        
        String8 productPath(getInputDeviceConfigurationFilePathByName(
                String8::format("Vendor_%04x_Product_%04x",
                        deviceIdentifier.vendor, deviceIdentifier.product),
                type));
        if (!productPath.isEmpty()) {
            return productPath;
        }
    }

    
    return getInputDeviceConfigurationFilePathByName(deviceIdentifier.name, type);
}

String8 getInputDeviceConfigurationFilePathByName(
        const String8& name, InputDeviceConfigurationFileType type) {
    
    String8 path;
    path.setTo(getenv("ANDROID_ROOT"));
    path.append("/usr/");
    appendInputDeviceConfigurationFileRelativePath(path, name, type);
#if DEBUG_PROBE
    ALOGD("Probing for system provided input device configuration file: path='%s'", path.string());
#endif
    if (!access(path.string(), R_OK)) {
#if DEBUG_PROBE
        ALOGD("Found");
#endif
        return path;
    }

    
    
    path.setTo(getenv("ANDROID_DATA"));
    path.append("/system/devices/");
    appendInputDeviceConfigurationFileRelativePath(path, name, type);
#if DEBUG_PROBE
    ALOGD("Probing for system user input device configuration file: path='%s'", path.string());
#endif
    if (!access(path.string(), R_OK)) {
#if DEBUG_PROBE
        ALOGD("Found");
#endif
        return path;
    }

    
#if DEBUG_PROBE
    ALOGD("Probe failed to find input device configuration file: name='%s', type=%d",
            name.string(), type);
#endif
    return String8();
}




void InputEvent::initialize(int32_t deviceId, int32_t source) {
    mDeviceId = deviceId;
    mSource = source;
}

void InputEvent::initialize(const InputEvent& from) {
    mDeviceId = from.mDeviceId;
    mSource = from.mSource;
}



bool KeyEvent::hasDefaultAction(int32_t keyCode) {
    switch (keyCode) {
        case AKEYCODE_HOME:
        case AKEYCODE_BACK:
        case AKEYCODE_CALL:
        case AKEYCODE_ENDCALL:
        case AKEYCODE_VOLUME_UP:
        case AKEYCODE_VOLUME_DOWN:
        case AKEYCODE_VOLUME_MUTE:
        case AKEYCODE_POWER:
        case AKEYCODE_CAMERA:
        case AKEYCODE_HEADSETHOOK:
        case AKEYCODE_MENU:
        case AKEYCODE_NOTIFICATION:
        case AKEYCODE_FOCUS:
        case AKEYCODE_SEARCH:
        case AKEYCODE_MEDIA_PLAY:
        case AKEYCODE_MEDIA_PAUSE:
        case AKEYCODE_MEDIA_PLAY_PAUSE:
        case AKEYCODE_MEDIA_STOP:
        case AKEYCODE_MEDIA_NEXT:
        case AKEYCODE_MEDIA_PREVIOUS:
        case AKEYCODE_MEDIA_REWIND:
        case AKEYCODE_MEDIA_RECORD:
        case AKEYCODE_MEDIA_FAST_FORWARD:
        case AKEYCODE_MUTE:
            return true;
    }
    
    return false;
}

bool KeyEvent::hasDefaultAction() const {
    return hasDefaultAction(getKeyCode());
}

bool KeyEvent::isSystemKey(int32_t keyCode) {
    switch (keyCode) {
        case AKEYCODE_MENU:
        case AKEYCODE_SOFT_RIGHT:
        case AKEYCODE_HOME:
        case AKEYCODE_BACK:
        case AKEYCODE_CALL:
        case AKEYCODE_ENDCALL:
        case AKEYCODE_VOLUME_UP:
        case AKEYCODE_VOLUME_DOWN:
        case AKEYCODE_VOLUME_MUTE:
        case AKEYCODE_MUTE:
        case AKEYCODE_POWER:
        case AKEYCODE_HEADSETHOOK:
        case AKEYCODE_MEDIA_PLAY:
        case AKEYCODE_MEDIA_PAUSE:
        case AKEYCODE_MEDIA_PLAY_PAUSE:
        case AKEYCODE_MEDIA_STOP:
        case AKEYCODE_MEDIA_NEXT:
        case AKEYCODE_MEDIA_PREVIOUS:
        case AKEYCODE_MEDIA_REWIND:
        case AKEYCODE_MEDIA_RECORD:
        case AKEYCODE_MEDIA_FAST_FORWARD:
        case AKEYCODE_CAMERA:
        case AKEYCODE_FOCUS:
        case AKEYCODE_SEARCH:
            return true;
    }
    
    return false;
}

bool KeyEvent::isSystemKey() const {
    return isSystemKey(getKeyCode());
}

void KeyEvent::initialize(
        int32_t deviceId,
        int32_t source,
        int32_t action,
        int32_t flags,
        int32_t keyCode,
        int32_t scanCode,
        int32_t metaState,
        int32_t repeatCount,
        nsecs_t downTime,
        nsecs_t eventTime) {
    InputEvent::initialize(deviceId, source);
    mAction = action;
    mFlags = flags;
    mKeyCode = keyCode;
    mScanCode = scanCode;
    mMetaState = metaState;
    mRepeatCount = repeatCount;
    mDownTime = downTime;
    mEventTime = eventTime;
}

void KeyEvent::initialize(const KeyEvent& from) {
    InputEvent::initialize(from);
    mAction = from.mAction;
    mFlags = from.mFlags;
    mKeyCode = from.mKeyCode;
    mScanCode = from.mScanCode;
    mMetaState = from.mMetaState;
    mRepeatCount = from.mRepeatCount;
    mDownTime = from.mDownTime;
    mEventTime = from.mEventTime;
}




float PointerCoords::getAxisValue(int32_t axis) const {
    if (axis < 0 || axis > 63) {
        return 0;
    }

    uint64_t axisBit = 1LL << axis;
    if (!(bits & axisBit)) {
        return 0;
    }
    uint32_t index = __builtin_popcountll(bits & (axisBit - 1LL));
    return values[index];
}

status_t PointerCoords::setAxisValue(int32_t axis, float value) {
    if (axis < 0 || axis > 63) {
        return NAME_NOT_FOUND;
    }

    uint64_t axisBit = 1LL << axis;
    uint32_t index = __builtin_popcountll(bits & (axisBit - 1LL));
    if (!(bits & axisBit)) {
        if (value == 0) {
            return OK; 
        }
        uint32_t count = __builtin_popcountll(bits);
        if (count >= MAX_AXES) {
            tooManyAxes(axis);
            return NO_MEMORY;
        }
        bits |= axisBit;
        for (uint32_t i = count; i > index; i--) {
            values[i] = values[i - 1];
        }
    }
    values[index] = value;
    return OK;
}

static inline void scaleAxisValue(PointerCoords& c, int axis, float scaleFactor) {
    float value = c.getAxisValue(axis);
    if (value != 0) {
        c.setAxisValue(axis, value * scaleFactor);
    }
}

void PointerCoords::scale(float scaleFactor) {
    
    
    scaleAxisValue(*this, AMOTION_EVENT_AXIS_X, scaleFactor);
    scaleAxisValue(*this, AMOTION_EVENT_AXIS_Y, scaleFactor);
    scaleAxisValue(*this, AMOTION_EVENT_AXIS_TOUCH_MAJOR, scaleFactor);
    scaleAxisValue(*this, AMOTION_EVENT_AXIS_TOUCH_MINOR, scaleFactor);
    scaleAxisValue(*this, AMOTION_EVENT_AXIS_TOOL_MAJOR, scaleFactor);
    scaleAxisValue(*this, AMOTION_EVENT_AXIS_TOOL_MINOR, scaleFactor);
}

#ifdef HAVE_ANDROID_OS
status_t PointerCoords::readFromParcel(Parcel* parcel) {
    bits = parcel->readInt64();

    uint32_t count = __builtin_popcountll(bits);
    if (count > MAX_AXES) {
        return BAD_VALUE;
    }

    for (uint32_t i = 0; i < count; i++) {
        values[i] = parcel->readInt32();
    }
    return OK;
}

status_t PointerCoords::writeToParcel(Parcel* parcel) const {
    parcel->writeInt64(bits);

    uint32_t count = __builtin_popcountll(bits);
    for (uint32_t i = 0; i < count; i++) {
        parcel->writeInt32(values[i]);
    }
    return OK;
}
#endif

void PointerCoords::tooManyAxes(int axis) {
    ALOGW("Could not set value for axis %d because the PointerCoords structure is full and "
            "cannot contain more than %d axis values.", axis, int(MAX_AXES));
}

bool PointerCoords::operator==(const PointerCoords& other) const {
    if (bits != other.bits) {
        return false;
    }
    uint32_t count = __builtin_popcountll(bits);
    for (uint32_t i = 0; i < count; i++) {
        if (values[i] != other.values[i]) {
            return false;
        }
    }
    return true;
}

void PointerCoords::copyFrom(const PointerCoords& other) {
    bits = other.bits;
    uint32_t count = __builtin_popcountll(bits);
    for (uint32_t i = 0; i < count; i++) {
        values[i] = other.values[i];
    }
}




bool PointerProperties::operator==(const PointerProperties& other) const {
    return id == other.id
            && toolType == other.toolType;
}

void PointerProperties::copyFrom(const PointerProperties& other) {
    id = other.id;
    toolType = other.toolType;
}




void MotionEvent::initialize(
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
        const PointerCoords* pointerCoords) {
    InputEvent::initialize(deviceId, source);
    mAction = action;
    mFlags = flags;
    mEdgeFlags = edgeFlags;
    mMetaState = metaState;
    mButtonState = buttonState;
    mXOffset = xOffset;
    mYOffset = yOffset;
    mXPrecision = xPrecision;
    mYPrecision = yPrecision;
    mDownTime = downTime;
    mPointerProperties.clear();
    mPointerProperties.appendArray(pointerProperties, pointerCount);
    mSampleEventTimes.clear();
    mSamplePointerCoords.clear();
    addSample(eventTime, pointerCoords);
}

void MotionEvent::copyFrom(const MotionEvent* other, bool keepHistory) {
    InputEvent::initialize(other->mDeviceId, other->mSource);
    mAction = other->mAction;
    mFlags = other->mFlags;
    mEdgeFlags = other->mEdgeFlags;
    mMetaState = other->mMetaState;
    mButtonState = other->mButtonState;
    mXOffset = other->mXOffset;
    mYOffset = other->mYOffset;
    mXPrecision = other->mXPrecision;
    mYPrecision = other->mYPrecision;
    mDownTime = other->mDownTime;
    mPointerProperties = other->mPointerProperties;

    if (keepHistory) {
        mSampleEventTimes = other->mSampleEventTimes;
        mSamplePointerCoords = other->mSamplePointerCoords;
    } else {
        mSampleEventTimes.clear();
        mSampleEventTimes.push(other->getEventTime());
        mSamplePointerCoords.clear();
        size_t pointerCount = other->getPointerCount();
        size_t historySize = other->getHistorySize();
        mSamplePointerCoords.appendArray(other->mSamplePointerCoords.array()
                + (historySize * pointerCount), pointerCount);
    }
}

void MotionEvent::addSample(
        int64_t eventTime,
        const PointerCoords* pointerCoords) {
    mSampleEventTimes.push(eventTime);
    mSamplePointerCoords.appendArray(pointerCoords, getPointerCount());
}

const PointerCoords* MotionEvent::getRawPointerCoords(size_t pointerIndex) const {
    return &mSamplePointerCoords[getHistorySize() * getPointerCount() + pointerIndex];
}

float MotionEvent::getRawAxisValue(int32_t axis, size_t pointerIndex) const {
    return getRawPointerCoords(pointerIndex)->getAxisValue(axis);
}

float MotionEvent::getAxisValue(int32_t axis, size_t pointerIndex) const {
    float value = getRawPointerCoords(pointerIndex)->getAxisValue(axis);
    switch (axis) {
    case AMOTION_EVENT_AXIS_X:
        return value + mXOffset;
    case AMOTION_EVENT_AXIS_Y:
        return value + mYOffset;
    }
    return value;
}

const PointerCoords* MotionEvent::getHistoricalRawPointerCoords(
        size_t pointerIndex, size_t historicalIndex) const {
    return &mSamplePointerCoords[historicalIndex * getPointerCount() + pointerIndex];
}

float MotionEvent::getHistoricalRawAxisValue(int32_t axis, size_t pointerIndex,
        size_t historicalIndex) const {
    return getHistoricalRawPointerCoords(pointerIndex, historicalIndex)->getAxisValue(axis);
}

float MotionEvent::getHistoricalAxisValue(int32_t axis, size_t pointerIndex,
        size_t historicalIndex) const {
    float value = getHistoricalRawPointerCoords(pointerIndex, historicalIndex)->getAxisValue(axis);
    switch (axis) {
    case AMOTION_EVENT_AXIS_X:
        return value + mXOffset;
    case AMOTION_EVENT_AXIS_Y:
        return value + mYOffset;
    }
    return value;
}

ssize_t MotionEvent::findPointerIndex(int32_t pointerId) const {
    size_t pointerCount = mPointerProperties.size();
    for (size_t i = 0; i < pointerCount; i++) {
        if (mPointerProperties.itemAt(i).id == pointerId) {
            return i;
        }
    }
    return -1;
}

void MotionEvent::offsetLocation(float xOffset, float yOffset) {
    mXOffset += xOffset;
    mYOffset += yOffset;
}

void MotionEvent::scale(float scaleFactor) {
    mXOffset *= scaleFactor;
    mYOffset *= scaleFactor;
    mXPrecision *= scaleFactor;
    mYPrecision *= scaleFactor;

    size_t numSamples = mSamplePointerCoords.size();
    for (size_t i = 0; i < numSamples; i++) {
        mSamplePointerCoords.editItemAt(i).scale(scaleFactor);
    }
}

#ifdef HAVE_ANDROID_OS
static inline float transformAngle(const SkMatrix* matrix, float angleRadians) {
    
    
    SkPoint vector;
    vector.fX = SkFloatToScalar(sinf(angleRadians));
    vector.fY = SkFloatToScalar(-cosf(angleRadians));
    matrix->mapVectors(& vector, 1);

    
    float result = atan2f(SkScalarToFloat(vector.fX), SkScalarToFloat(-vector.fY));
    if (result < - M_PI_2) {
        result += M_PI;
    } else if (result > M_PI_2) {
        result -= M_PI;
    }
    return result;
}

void MotionEvent::transform(const SkMatrix* matrix) {
    float oldXOffset = mXOffset;
    float oldYOffset = mYOffset;

    
    
    
    SkPoint point;
    float rawX = getRawX(0);
    float rawY = getRawY(0);
    matrix->mapXY(SkFloatToScalar(rawX + oldXOffset), SkFloatToScalar(rawY + oldYOffset),
            & point);
    float newX = SkScalarToFloat(point.fX);
    float newY = SkScalarToFloat(point.fY);
    float newXOffset = newX - rawX;
    float newYOffset = newY - rawY;

    mXOffset = newXOffset;
    mYOffset = newYOffset;

    
    size_t numSamples = mSamplePointerCoords.size();
    for (size_t i = 0; i < numSamples; i++) {
        PointerCoords& c = mSamplePointerCoords.editItemAt(i);
        float x = c.getAxisValue(AMOTION_EVENT_AXIS_X) + oldXOffset;
        float y = c.getAxisValue(AMOTION_EVENT_AXIS_Y) + oldYOffset;
        matrix->mapXY(SkFloatToScalar(x), SkFloatToScalar(y), &point);
        c.setAxisValue(AMOTION_EVENT_AXIS_X, SkScalarToFloat(point.fX) - newXOffset);
        c.setAxisValue(AMOTION_EVENT_AXIS_Y, SkScalarToFloat(point.fY) - newYOffset);

        float orientation = c.getAxisValue(AMOTION_EVENT_AXIS_ORIENTATION);
        c.setAxisValue(AMOTION_EVENT_AXIS_ORIENTATION, transformAngle(matrix, orientation));
    }
}

status_t MotionEvent::readFromParcel(Parcel* parcel) {
    size_t pointerCount = parcel->readInt32();
    size_t sampleCount = parcel->readInt32();
    if (pointerCount == 0 || pointerCount > MAX_POINTERS || sampleCount == 0) {
        return BAD_VALUE;
    }

    mDeviceId = parcel->readInt32();
    mSource = parcel->readInt32();
    mAction = parcel->readInt32();
    mFlags = parcel->readInt32();
    mEdgeFlags = parcel->readInt32();
    mMetaState = parcel->readInt32();
    mButtonState = parcel->readInt32();
    mXOffset = parcel->readFloat();
    mYOffset = parcel->readFloat();
    mXPrecision = parcel->readFloat();
    mYPrecision = parcel->readFloat();
    mDownTime = parcel->readInt64();

    mPointerProperties.clear();
    mPointerProperties.setCapacity(pointerCount);
    mSampleEventTimes.clear();
    mSampleEventTimes.setCapacity(sampleCount);
    mSamplePointerCoords.clear();
    mSamplePointerCoords.setCapacity(sampleCount * pointerCount);

    for (size_t i = 0; i < pointerCount; i++) {
        mPointerProperties.push();
        PointerProperties& properties = mPointerProperties.editTop();
        properties.id = parcel->readInt32();
        properties.toolType = parcel->readInt32();
    }

    while (sampleCount-- > 0) {
        mSampleEventTimes.push(parcel->readInt64());
        for (size_t i = 0; i < pointerCount; i++) {
            mSamplePointerCoords.push();
            status_t status = mSamplePointerCoords.editTop().readFromParcel(parcel);
            if (status) {
                return status;
            }
        }
    }
    return OK;
}

status_t MotionEvent::writeToParcel(Parcel* parcel) const {
    size_t pointerCount = mPointerProperties.size();
    size_t sampleCount = mSampleEventTimes.size();

    parcel->writeInt32(pointerCount);
    parcel->writeInt32(sampleCount);

    parcel->writeInt32(mDeviceId);
    parcel->writeInt32(mSource);
    parcel->writeInt32(mAction);
    parcel->writeInt32(mFlags);
    parcel->writeInt32(mEdgeFlags);
    parcel->writeInt32(mMetaState);
    parcel->writeInt32(mButtonState);
    parcel->writeFloat(mXOffset);
    parcel->writeFloat(mYOffset);
    parcel->writeFloat(mXPrecision);
    parcel->writeFloat(mYPrecision);
    parcel->writeInt64(mDownTime);

    for (size_t i = 0; i < pointerCount; i++) {
        const PointerProperties& properties = mPointerProperties.itemAt(i);
        parcel->writeInt32(properties.id);
        parcel->writeInt32(properties.toolType);
    }

    const PointerCoords* pc = mSamplePointerCoords.array();
    for (size_t h = 0; h < sampleCount; h++) {
        parcel->writeInt64(mSampleEventTimes.itemAt(h));
        for (size_t i = 0; i < pointerCount; i++) {
            status_t status = (pc++)->writeToParcel(parcel);
            if (status) {
                return status;
            }
        }
    }
    return OK;
}
#endif

bool MotionEvent::isTouchEvent(int32_t source, int32_t action) {
    if (source & AINPUT_SOURCE_CLASS_POINTER) {
        
        switch (action & AMOTION_EVENT_ACTION_MASK) {
        case AMOTION_EVENT_ACTION_DOWN:
        case AMOTION_EVENT_ACTION_MOVE:
        case AMOTION_EVENT_ACTION_UP:
        case AMOTION_EVENT_ACTION_POINTER_DOWN:
        case AMOTION_EVENT_ACTION_POINTER_UP:
        case AMOTION_EVENT_ACTION_CANCEL:
        case AMOTION_EVENT_ACTION_OUTSIDE:
            return true;
        }
    }
    return false;
}




const uint32_t VelocityTracker::DEFAULT_DEGREE;
const nsecs_t VelocityTracker::DEFAULT_HORIZON;
const uint32_t VelocityTracker::HISTORY_SIZE;

static inline float vectorDot(const float* a, const float* b, uint32_t m) {
    float r = 0;
    while (m--) {
        r += *(a++) * *(b++);
    }
    return r;
}

static inline float vectorNorm(const float* a, uint32_t m) {
    float r = 0;
    while (m--) {
        float t = *(a++);
        r += t * t;
    }
    return sqrtf(r);
}

#if DEBUG_LEAST_SQUARES || DEBUG_VELOCITY
static String8 vectorToString(const float* a, uint32_t m) {
    String8 str;
    str.append("[");
    while (m--) {
        str.appendFormat(" %f", *(a++));
        if (m) {
            str.append(",");
        }
    }
    str.append(" ]");
    return str;
}

static String8 matrixToString(const float* a, uint32_t m, uint32_t n, bool rowMajor) {
    String8 str;
    str.append("[");
    for (size_t i = 0; i < m; i++) {
        if (i) {
            str.append(",");
        }
        str.append(" [");
        for (size_t j = 0; j < n; j++) {
            if (j) {
                str.append(",");
            }
            str.appendFormat(" %f", a[rowMajor ? i * n + j : j * m + i]);
        }
        str.append(" ]");
    }
    str.append(" ]");
    return str;
}
#endif

VelocityTracker::VelocityTracker() {
    clear();
}

void VelocityTracker::clear() {
    mIndex = 0;
    mMovements[0].idBits.clear();
    mActivePointerId = -1;
}

void VelocityTracker::clearPointers(BitSet32 idBits) {
    BitSet32 remainingIdBits(mMovements[mIndex].idBits.value & ~idBits.value);
    mMovements[mIndex].idBits = remainingIdBits;

    if (mActivePointerId >= 0 && idBits.hasBit(mActivePointerId)) {
        mActivePointerId = !remainingIdBits.isEmpty() ? remainingIdBits.firstMarkedBit() : -1;
    }
}

void VelocityTracker::addMovement(nsecs_t eventTime, BitSet32 idBits, const Position* positions) {
    if (++mIndex == HISTORY_SIZE) {
        mIndex = 0;
    }

    while (idBits.count() > MAX_POINTERS) {
        idBits.clearLastMarkedBit();
    }

    Movement& movement = mMovements[mIndex];
    movement.eventTime = eventTime;
    movement.idBits = idBits;
    uint32_t count = idBits.count();
    for (uint32_t i = 0; i < count; i++) {
        movement.positions[i] = positions[i];
    }

    if (mActivePointerId < 0 || !idBits.hasBit(mActivePointerId)) {
        mActivePointerId = count != 0 ? idBits.firstMarkedBit() : -1;
    }

#if DEBUG_VELOCITY
    ALOGD("VelocityTracker: addMovement eventTime=%lld, idBits=0x%08x, activePointerId=%d",
            eventTime, idBits.value, mActivePointerId);
    for (BitSet32 iterBits(idBits); !iterBits.isEmpty(); ) {
        uint32_t id = iterBits.firstMarkedBit();
        uint32_t index = idBits.getIndexOfBit(id);
        iterBits.clearBit(id);
        Estimator estimator;
        getEstimator(id, DEFAULT_DEGREE, DEFAULT_HORIZON, &estimator);
        ALOGD("  %d: position (%0.3f, %0.3f), "
                "estimator (degree=%d, xCoeff=%s, yCoeff=%s, confidence=%f)",
                id, positions[index].x, positions[index].y,
                int(estimator.degree),
                vectorToString(estimator.xCoeff, estimator.degree).string(),
                vectorToString(estimator.yCoeff, estimator.degree).string(),
                estimator.confidence);
    }
#endif
}

void VelocityTracker::addMovement(const MotionEvent* event) {
    int32_t actionMasked = event->getActionMasked();

    switch (actionMasked) {
    case AMOTION_EVENT_ACTION_DOWN:
    case AMOTION_EVENT_ACTION_HOVER_ENTER:
        
        clear();
        break;
    case AMOTION_EVENT_ACTION_POINTER_DOWN: {
        
        
        
        BitSet32 downIdBits;
        downIdBits.markBit(event->getPointerId(event->getActionIndex()));
        clearPointers(downIdBits);
        break;
    }
    case AMOTION_EVENT_ACTION_MOVE:
    case AMOTION_EVENT_ACTION_HOVER_MOVE:
        break;
    default:
        
        
        
        
        
        
        
        
        return;
    }

    size_t pointerCount = event->getPointerCount();
    if (pointerCount > MAX_POINTERS) {
        pointerCount = MAX_POINTERS;
    }

    BitSet32 idBits;
    for (size_t i = 0; i < pointerCount; i++) {
        idBits.markBit(event->getPointerId(i));
    }

    nsecs_t eventTime;
    Position positions[pointerCount];

    size_t historySize = event->getHistorySize();
    for (size_t h = 0; h < historySize; h++) {
        eventTime = event->getHistoricalEventTime(h);
        for (size_t i = 0; i < pointerCount; i++) {
            positions[i].x = event->getHistoricalX(i, h);
            positions[i].y = event->getHistoricalY(i, h);
        }
        addMovement(eventTime, idBits, positions);
    }

    eventTime = event->getEventTime();
    for (size_t i = 0; i < pointerCount; i++) {
        positions[i].x = event->getX(i);
        positions[i].y = event->getY(i);
    }
    addMovement(eventTime, idBits, positions);
}




































static bool solveLeastSquares(const float* x, const float* y, uint32_t m, uint32_t n,
        float* outB, float* outDet) {
#if DEBUG_LEAST_SQUARES
    ALOGD("solveLeastSquares: m=%d, n=%d, x=%s, y=%s", int(m), int(n),
            vectorToString(x, m).string(), vectorToString(y, m).string());
#endif

    
    float a[n][m]; 
    for (uint32_t h = 0; h < m; h++) {
        a[0][h] = 1;
        for (uint32_t i = 1; i < n; i++) {
            a[i][h] = a[i - 1][h] * x[h];
        }
    }
#if DEBUG_LEAST_SQUARES
    ALOGD("  - a=%s", matrixToString(&a[0][0], m, n, false ).string());
#endif

    
    float q[n][m]; 
    float r[n][n]; 
    for (uint32_t j = 0; j < n; j++) {
        for (uint32_t h = 0; h < m; h++) {
            q[j][h] = a[j][h];
        }
        for (uint32_t i = 0; i < j; i++) {
            float dot = vectorDot(&q[j][0], &q[i][0], m);
            for (uint32_t h = 0; h < m; h++) {
                q[j][h] -= dot * q[i][h];
            }
        }

        float norm = vectorNorm(&q[j][0], m);
        if (norm < 0.000001f) {
            
#if DEBUG_LEAST_SQUARES
            ALOGD("  - no solution, norm=%f", norm);
#endif
            return false;
        }

        float invNorm = 1.0f / norm;
        for (uint32_t h = 0; h < m; h++) {
            q[j][h] *= invNorm;
        }
        for (uint32_t i = 0; i < n; i++) {
            r[j][i] = i < j ? 0 : vectorDot(&q[j][0], &a[i][0], m);
        }
    }
#if DEBUG_LEAST_SQUARES
    ALOGD("  - q=%s", matrixToString(&q[0][0], m, n, false ).string());
    ALOGD("  - r=%s", matrixToString(&r[0][0], n, n, true ).string());

    
    float qr[n][m];
    for (uint32_t h = 0; h < m; h++) {
        for (uint32_t i = 0; i < n; i++) {
            qr[i][h] = 0;
            for (uint32_t j = 0; j < n; j++) {
                qr[i][h] += q[j][h] * r[j][i];
            }
        }
    }
    ALOGD("  - qr=%s", matrixToString(&qr[0][0], m, n, false ).string());
#endif

    
    
    for (uint32_t i = n; i-- != 0; ) {
        outB[i] = vectorDot(&q[i][0], y, m);
        for (uint32_t j = n - 1; j > i; j--) {
            outB[i] -= r[i][j] * outB[j];
        }
        outB[i] /= r[i][i];
    }
#if DEBUG_LEAST_SQUARES
    ALOGD("  - b=%s", vectorToString(outB, n).string());
#endif

    
    
    
    float ymean = 0;
    for (uint32_t h = 0; h < m; h++) {
        ymean += y[h];
    }
    ymean /= m;

    float sserr = 0;
    float sstot = 0;
    for (uint32_t h = 0; h < m; h++) {
        float err = y[h] - outB[0];
        float term = 1;
        for (uint32_t i = 1; i < n; i++) {
            term *= x[h];
            err -= term * outB[i];
        }
        sserr += err * err;
        float var = y[h] - ymean;
        sstot += var * var;
    }
    *outDet = sstot > 0.000001f ? 1.0f - (sserr / sstot) : 1;
#if DEBUG_LEAST_SQUARES
    ALOGD("  - sserr=%f", sserr);
    ALOGD("  - sstot=%f", sstot);
    ALOGD("  - det=%f", *outDet);
#endif
    return true;
}

bool VelocityTracker::getVelocity(uint32_t id, float* outVx, float* outVy) const {
    Estimator estimator;
    if (getEstimator(id, DEFAULT_DEGREE, DEFAULT_HORIZON, &estimator)) {
        if (estimator.degree >= 1) {
            *outVx = estimator.xCoeff[1];
            *outVy = estimator.yCoeff[1];
            return true;
        }
    }
    *outVx = 0;
    *outVy = 0;
    return false;
}

bool VelocityTracker::getEstimator(uint32_t id, uint32_t degree, nsecs_t horizon,
        Estimator* outEstimator) const {
    outEstimator->clear();

    
    float x[HISTORY_SIZE];
    float y[HISTORY_SIZE];
    float time[HISTORY_SIZE];
    uint32_t m = 0;
    uint32_t index = mIndex;
    const Movement& newestMovement = mMovements[mIndex];
    do {
        const Movement& movement = mMovements[index];
        if (!movement.idBits.hasBit(id)) {
            break;
        }

        nsecs_t age = newestMovement.eventTime - movement.eventTime;
        if (age > horizon) {
            break;
        }

        const Position& position = movement.getPosition(id);
        x[m] = position.x;
        y[m] = position.y;
        time[m] = -age * 0.000000001f;
        index = (index == 0 ? HISTORY_SIZE : index) - 1;
    } while (++m < HISTORY_SIZE);

    if (m == 0) {
        return false; 
    }

    
    if (degree > Estimator::MAX_DEGREE) {
        degree = Estimator::MAX_DEGREE;
    }
    if (degree > m - 1) {
        degree = m - 1;
    }
    if (degree >= 1) {
        float xdet, ydet;
        uint32_t n = degree + 1;
        if (solveLeastSquares(time, x, m, n, outEstimator->xCoeff, &xdet)
                && solveLeastSquares(time, y, m, n, outEstimator->yCoeff, &ydet)) {
            outEstimator->degree = degree;
            outEstimator->confidence = xdet * ydet;
#if DEBUG_LEAST_SQUARES
            ALOGD("estimate: degree=%d, xCoeff=%s, yCoeff=%s, confidence=%f",
                    int(outEstimator->degree),
                    vectorToString(outEstimator->xCoeff, n).string(),
                    vectorToString(outEstimator->yCoeff, n).string(),
                    outEstimator->confidence);
#endif
            return true;
        }
    }

    
    outEstimator->xCoeff[0] = x[0];
    outEstimator->yCoeff[0] = y[0];
    outEstimator->degree = 0;
    outEstimator->confidence = 1;
    return true;
}




const nsecs_t VelocityControl::STOP_TIME;

VelocityControl::VelocityControl() {
    reset();
}

void VelocityControl::setParameters(const VelocityControlParameters& parameters) {
    mParameters = parameters;
    reset();
}

void VelocityControl::reset() {
    mLastMovementTime = LLONG_MIN;
    mRawPosition.x = 0;
    mRawPosition.y = 0;
    mVelocityTracker.clear();
}

void VelocityControl::move(nsecs_t eventTime, float* deltaX, float* deltaY) {
    if ((deltaX && *deltaX) || (deltaY && *deltaY)) {
        if (eventTime >= mLastMovementTime + STOP_TIME) {
#if DEBUG_ACCELERATION
            ALOGD("VelocityControl: stopped, last movement was %0.3fms ago",
                    (eventTime - mLastMovementTime) * 0.000001f);
#endif
            reset();
        }

        mLastMovementTime = eventTime;
        if (deltaX) {
            mRawPosition.x += *deltaX;
        }
        if (deltaY) {
            mRawPosition.y += *deltaY;
        }
        mVelocityTracker.addMovement(eventTime, BitSet32(BitSet32::valueForBit(0)), &mRawPosition);

        float vx, vy;
        float scale = mParameters.scale;
        if (mVelocityTracker.getVelocity(0, &vx, &vy)) {
            float speed = hypotf(vx, vy) * scale;
            if (speed >= mParameters.highThreshold) {
                
                scale *= mParameters.acceleration;
            } else if (speed > mParameters.lowThreshold) {
                
                
                scale *= 1 + (speed - mParameters.lowThreshold)
                        / (mParameters.highThreshold - mParameters.lowThreshold)
                        * (mParameters.acceleration - 1);
            }

#if DEBUG_ACCELERATION
            ALOGD("VelocityControl(%0.3f, %0.3f, %0.3f, %0.3f): "
                    "vx=%0.3f, vy=%0.3f, speed=%0.3f, accel=%0.3f",
                    mParameters.scale, mParameters.lowThreshold, mParameters.highThreshold,
                    mParameters.acceleration,
                    vx, vy, speed, scale / mParameters.scale);
#endif
        } else {
#if DEBUG_ACCELERATION
            ALOGD("VelocityControl(%0.3f, %0.3f, %0.3f, %0.3f): unknown velocity",
                    mParameters.scale, mParameters.lowThreshold, mParameters.highThreshold,
                    mParameters.acceleration);
#endif
        }

        if (deltaX) {
            *deltaX *= scale;
        }
        if (deltaY) {
            *deltaY *= scale;
        }
    }
}




InputDeviceInfo::InputDeviceInfo() {
    initialize(-1, String8("uninitialized device info"));
}

InputDeviceInfo::InputDeviceInfo(const InputDeviceInfo& other) :
        mId(other.mId), mName(other.mName), mSources(other.mSources),
        mKeyboardType(other.mKeyboardType),
        mMotionRanges(other.mMotionRanges) {
}

InputDeviceInfo::~InputDeviceInfo() {
}

void InputDeviceInfo::initialize(int32_t id, const String8& name) {
    mId = id;
    mName = name;
    mSources = 0;
    mKeyboardType = AINPUT_KEYBOARD_TYPE_NONE;
    mMotionRanges.clear();
}

const InputDeviceInfo::MotionRange* InputDeviceInfo::getMotionRange(
        int32_t axis, uint32_t source) const {
    size_t numRanges = mMotionRanges.size();
    for (size_t i = 0; i < numRanges; i++) {
        const MotionRange& range = mMotionRanges.itemAt(i);
        if (range.axis == axis && range.source == source) {
            return &range;
        }
    }
    return NULL;
}

void InputDeviceInfo::addSource(uint32_t source) {
    mSources |= source;
}

void InputDeviceInfo::addMotionRange(int32_t axis, uint32_t source, float min, float max,
        float flat, float fuzz) {
    MotionRange range = { axis, source, min, max, flat, fuzz };
    mMotionRanges.add(range);
}

void InputDeviceInfo::addMotionRange(const MotionRange& range) {
    mMotionRanges.add(range);
}

} 
