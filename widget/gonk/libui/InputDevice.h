















#ifndef _ANDROIDFW_INPUT_DEVICE_H
#define _ANDROIDFW_INPUT_DEVICE_H

#include "Input.h"
#include "KeyCharacterMap.h"

namespace android {




struct InputDeviceIdentifier {
    inline InputDeviceIdentifier() :
            bus(0), vendor(0), product(0), version(0) {
    }

    
    String8 name;
    String8 location;
    String8 uniqueId;
    uint16_t bus;
    uint16_t vendor;
    uint16_t product;
    uint16_t version;

    
    
    
    
    
    
    String8 descriptor;
};




class InputDeviceInfo {
public:
    InputDeviceInfo();
    InputDeviceInfo(const InputDeviceInfo& other);
    ~InputDeviceInfo();

    struct MotionRange {
        int32_t axis;
        uint32_t source;
        float min;
        float max;
        float flat;
        float fuzz;
        float resolution;
    };

    void initialize(int32_t id, int32_t generation, const InputDeviceIdentifier& identifier,
            const String8& alias, bool isExternal);

    inline int32_t getId() const { return mId; }
    inline int32_t getGeneration() const { return mGeneration; }
    inline const InputDeviceIdentifier& getIdentifier() const { return mIdentifier; }
    inline const String8& getAlias() const { return mAlias; }
    inline const String8& getDisplayName() const {
        return mAlias.isEmpty() ? mIdentifier.name : mAlias;
    }
    inline bool isExternal() const { return mIsExternal; }
    inline uint32_t getSources() const { return mSources; }

    const MotionRange* getMotionRange(int32_t axis, uint32_t source) const;

    void addSource(uint32_t source);
    void addMotionRange(int32_t axis, uint32_t source,
            float min, float max, float flat, float fuzz, float resolution);
    void addMotionRange(const MotionRange& range);

    inline void setKeyboardType(int32_t keyboardType) { mKeyboardType = keyboardType; }
    inline int32_t getKeyboardType() const { return mKeyboardType; }

    inline void setKeyCharacterMap(const sp<KeyCharacterMap>& value) {
        mKeyCharacterMap = value;
    }

    inline sp<KeyCharacterMap> getKeyCharacterMap() const {
        return mKeyCharacterMap;
    }

    inline void setVibrator(bool hasVibrator) { mHasVibrator = hasVibrator; }
    inline bool hasVibrator() const { return mHasVibrator; }

    inline const Vector<MotionRange>& getMotionRanges() const {
        return mMotionRanges;
    }

private:
    int32_t mId;
    int32_t mGeneration;
    InputDeviceIdentifier mIdentifier;
    String8 mAlias;
    bool mIsExternal;
    uint32_t mSources;
    int32_t mKeyboardType;
    sp<KeyCharacterMap> mKeyCharacterMap;
    bool mHasVibrator;

    Vector<MotionRange> mMotionRanges;
};


enum InputDeviceConfigurationFileType {
    INPUT_DEVICE_CONFIGURATION_FILE_TYPE_CONFIGURATION = 0,     
    INPUT_DEVICE_CONFIGURATION_FILE_TYPE_KEY_LAYOUT = 1,        
    INPUT_DEVICE_CONFIGURATION_FILE_TYPE_KEY_CHARACTER_MAP = 2, 
};










extern String8 getInputDeviceConfigurationFilePathByDeviceIdentifier(
        const InputDeviceIdentifier& deviceIdentifier,
        InputDeviceConfigurationFileType type);










extern String8 getInputDeviceConfigurationFilePathByName(
        const String8& name, InputDeviceConfigurationFileType type);

} 

#endif 
