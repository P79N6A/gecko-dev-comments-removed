















#ifndef _ANDROIDFW_KEY_CHARACTER_MAP_H
#define _ANDROIDFW_KEY_CHARACTER_MAP_H

#include <stdint.h>

#if HAVE_ANDROID_OS
#include <binder/IBinder.h>
#endif

#include "Input.h"
#include <utils/Errors.h>
#include <utils/KeyedVector.h>
#include "Tokenizer.h"
#include <utils/String8.h>
#include <utils/Unicode.h>
#include <utils/RefBase.h>

namespace android {








class KeyCharacterMap : public RefBase {
public:
    enum KeyboardType {
        KEYBOARD_TYPE_UNKNOWN = 0,
        KEYBOARD_TYPE_NUMERIC = 1,
        KEYBOARD_TYPE_PREDICTIVE = 2,
        KEYBOARD_TYPE_ALPHA = 3,
        KEYBOARD_TYPE_FULL = 4,
        KEYBOARD_TYPE_SPECIAL_FUNCTION = 5,
        KEYBOARD_TYPE_OVERLAY = 6,
    };

    enum Format {
        
        FORMAT_BASE = 0,
        
        
        FORMAT_OVERLAY = 1,
        
        FORMAT_ANY = 2,
    };

    
    struct FallbackAction {
        int32_t keyCode;
        int32_t metaState;
    };

    
    static status_t load(const String8& filename, Format format, sp<KeyCharacterMap>* outMap);

    
    static status_t loadContents(const String8& filename,
            const char* contents, Format format, sp<KeyCharacterMap>* outMap);

    
    static sp<KeyCharacterMap> combine(const sp<KeyCharacterMap>& base,
            const sp<KeyCharacterMap>& overlay);

    
    static sp<KeyCharacterMap> empty();

    
    int32_t getKeyboardType() const;

    

    char16_t getDisplayLabel(int32_t keyCode) const;

    



    char16_t getNumber(int32_t keyCode) const;

    


    char16_t getCharacter(int32_t keyCode, int32_t metaState) const;

    



    bool getFallbackAction(int32_t keyCode, int32_t metaState,
            FallbackAction* outFallbackAction) const;

    



    char16_t getMatch(int32_t keyCode, const char16_t* chars,
            size_t numChars, int32_t metaState) const;

    


    bool getEvents(int32_t deviceId, const char16_t* chars, size_t numChars,
            Vector<KeyEvent>& outEvents) const;

    

    status_t mapKey(int32_t scanCode, int32_t usageCode, int32_t* outKeyCode) const;

#if HAVE_ANDROID_OS
    
    static sp<KeyCharacterMap> readFromParcel(Parcel* parcel);

    
    void writeToParcel(Parcel* parcel) const;
#endif

protected:
    virtual ~KeyCharacterMap();

private:
    struct Behavior {
        Behavior();
        Behavior(const Behavior& other);

        
        Behavior* next;

        
        int32_t metaState;

        
        char16_t character;

        
        int32_t fallbackKeyCode;
    };

    struct Key {
        Key();
        Key(const Key& other);
        ~Key();

        
        char16_t label;

        
        char16_t number;

        

        Behavior* firstBehavior;
    };

    class Parser {
        enum State {
            STATE_TOP = 0,
            STATE_KEY = 1,
        };

        enum {
            PROPERTY_LABEL = 1,
            PROPERTY_NUMBER = 2,
            PROPERTY_META = 3,
        };

        struct Property {
            inline Property(int32_t property = 0, int32_t metaState = 0) :
                    property(property), metaState(metaState) { }

            int32_t property;
            int32_t metaState;
        };

        KeyCharacterMap* mMap;
        Tokenizer* mTokenizer;
        Format mFormat;
        State mState;
        int32_t mKeyCode;

    public:
        Parser(KeyCharacterMap* map, Tokenizer* tokenizer, Format format);
        ~Parser();
        status_t parse();

    private:
        status_t parseType();
        status_t parseMap();
        status_t parseMapKey();
        status_t parseKey();
        status_t parseKeyProperty();
        status_t finishKey(Key* key);
        status_t parseModifier(const String8& token, int32_t* outMetaState);
        status_t parseCharacterLiteral(char16_t* outCharacter);
    };

    static sp<KeyCharacterMap> sEmpty;

    KeyedVector<int32_t, Key*> mKeys;
    int mType;

    KeyedVector<int32_t, int32_t> mKeysByScanCode;
    KeyedVector<int32_t, int32_t> mKeysByUsageCode;

    KeyCharacterMap();
    KeyCharacterMap(const KeyCharacterMap& other);

    bool getKey(int32_t keyCode, const Key** outKey) const;
    bool getKeyBehavior(int32_t keyCode, int32_t metaState,
            const Key** outKey, const Behavior** outBehavior) const;
    static bool matchesMetaState(int32_t eventMetaState, int32_t behaviorMetaState);

    bool findKey(char16_t ch, int32_t* outKeyCode, int32_t* outMetaState) const;

    static status_t load(Tokenizer* tokenizer, Format format, sp<KeyCharacterMap>* outMap);

    static void addKey(Vector<KeyEvent>& outEvents,
            int32_t deviceId, int32_t keyCode, int32_t metaState, bool down, nsecs_t time);
    static void addMetaKeys(Vector<KeyEvent>& outEvents,
            int32_t deviceId, int32_t metaState, bool down, nsecs_t time,
            int32_t* currentMetaState);
    static bool addSingleEphemeralMetaKey(Vector<KeyEvent>& outEvents,
            int32_t deviceId, int32_t metaState, bool down, nsecs_t time,
            int32_t keyCode, int32_t keyMetaState,
            int32_t* currentMetaState);
    static void addDoubleEphemeralMetaKey(Vector<KeyEvent>& outEvents,
            int32_t deviceId, int32_t metaState, bool down, nsecs_t time,
            int32_t leftKeyCode, int32_t leftKeyMetaState,
            int32_t rightKeyCode, int32_t rightKeyMetaState,
            int32_t eitherKeyMetaState,
            int32_t* currentMetaState);
    static void addLockedMetaKey(Vector<KeyEvent>& outEvents,
            int32_t deviceId, int32_t metaState, nsecs_t time,
            int32_t keyCode, int32_t keyMetaState,
            int32_t* currentMetaState);
};

} 

#endif
