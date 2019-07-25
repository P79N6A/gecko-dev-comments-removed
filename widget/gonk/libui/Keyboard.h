















#ifndef _UI_KEYBOARD_H
#define _UI_KEYBOARD_H

#include "Input.h"
#include <utils/Errors.h>
#include <utils/String8.h>
#include <utils/PropertyMap.h>

namespace android {

enum {
    
    DEVICE_ID_BUILT_IN_KEYBOARD = 0,

    

    DEVICE_ID_VIRTUAL_KEYBOARD = -1,
};

class KeyLayoutMap;
class KeyCharacterMap;




class KeyMap {
public:
    String8 keyLayoutFile;
    KeyLayoutMap* keyLayoutMap;

    String8 keyCharacterMapFile;
    KeyCharacterMap* keyCharacterMap;

    KeyMap();
    ~KeyMap();

    status_t load(const InputDeviceIdentifier& deviceIdenfier,
            const PropertyMap* deviceConfiguration);

    inline bool haveKeyLayout() const {
        return !keyLayoutFile.isEmpty();
    }

    inline bool haveKeyCharacterMap() const {
        return !keyCharacterMapFile.isEmpty();
    }

    inline bool isComplete() const {
        return haveKeyLayout() && haveKeyCharacterMap();
    }

private:
    bool probeKeyMap(const InputDeviceIdentifier& deviceIdentifier, const String8& name);
    status_t loadKeyLayout(const InputDeviceIdentifier& deviceIdentifier, const String8& name);
    status_t loadKeyCharacterMap(const InputDeviceIdentifier& deviceIdentifier,
            const String8& name);
    String8 getPath(const InputDeviceIdentifier& deviceIdentifier,
            const String8& name, InputDeviceConfigurationFileType type);
};




extern bool isEligibleBuiltInKeyboard(const InputDeviceIdentifier& deviceIdentifier,
        const PropertyMap* deviceConfiguration, const KeyMap* keyMap);





extern int32_t getKeyCodeByLabel(const char* label);





extern uint32_t getKeyFlagByLabel(const char* label);





extern int32_t getAxisByLabel(const char* label);





extern const char* getAxisLabel(int32_t axisId);




extern int32_t updateMetaState(int32_t keyCode, bool down, int32_t oldMetaState);




extern bool isMetaKey(int32_t keyCode);

} 

#endif 
