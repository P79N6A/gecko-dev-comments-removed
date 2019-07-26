















#define LOG_TAG "InputDevice"

#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include "InputDevice.h"

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




InputDeviceInfo::InputDeviceInfo() {
    initialize(-1, -1, InputDeviceIdentifier(), String8(), false);
}

InputDeviceInfo::InputDeviceInfo(const InputDeviceInfo& other) :
        mId(other.mId), mGeneration(other.mGeneration), mIdentifier(other.mIdentifier),
        mAlias(other.mAlias), mIsExternal(other.mIsExternal), mSources(other.mSources),
        mKeyboardType(other.mKeyboardType),
        mKeyCharacterMap(other.mKeyCharacterMap),
        mHasVibrator(other.mHasVibrator),
        mMotionRanges(other.mMotionRanges) {
}

InputDeviceInfo::~InputDeviceInfo() {
}

void InputDeviceInfo::initialize(int32_t id, int32_t generation,
        const InputDeviceIdentifier& identifier, const String8& alias, bool isExternal) {
    mId = id;
    mGeneration = generation;
    mIdentifier = identifier;
    mAlias = alias;
    mIsExternal = isExternal;
    mSources = 0;
    mKeyboardType = AINPUT_KEYBOARD_TYPE_NONE;
    mHasVibrator = false;
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
        float flat, float fuzz, float resolution) {
    MotionRange range = { axis, source, min, max, flat, fuzz, resolution };
    mMotionRanges.add(range);
}

void InputDeviceInfo::addMotionRange(const MotionRange& range) {
    mMotionRanges.add(range);
}

} 
