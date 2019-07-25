















#ifndef _UI_KEY_CHARACTER_MAP_H
#define _UI_KEY_CHARACTER_MAP_H

#include <stdint.h>

#include "Input.h"
#include <utils/Errors.h>
#include <utils/KeyedVector.h>
#include <utils/Tokenizer.h>
#include <utils/String8.h>
#include <utils/Unicode.h>

namespace android {






class KeyCharacterMap {
public:
    enum KeyboardType {
        KEYBOARD_TYPE_UNKNOWN = 0,
        KEYBOARD_TYPE_NUMERIC = 1,
        KEYBOARD_TYPE_PREDICTIVE = 2,
        KEYBOARD_TYPE_ALPHA = 3,
        KEYBOARD_TYPE_FULL = 4,
        KEYBOARD_TYPE_SPECIAL_FUNCTION = 5,
    };

    
    struct FallbackAction {
        int32_t keyCode;
        int32_t metaState;
    };

    ~KeyCharacterMap();

    static status_t load(const String8& filename, KeyCharacterMap** outMap);

    
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

private:
    struct Behavior {
        Behavior();

        
        Behavior* next;

        
        int32_t metaState;

        
        char16_t character;

        
        int32_t fallbackKeyCode;
    };

    struct Key {
        Key();
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
        State mState;
        int32_t mKeyCode;

    public:
        Parser(KeyCharacterMap* map, Tokenizer* tokenizer);
        ~Parser();
        status_t parse();

    private:
        status_t parseType();
        status_t parseKey();
        status_t parseKeyProperty();
        status_t parseModifier(const String8& token, int32_t* outMetaState);
        status_t parseCharacterLiteral(char16_t* outCharacter);
    };

    KeyedVector<int32_t, Key*> mKeys;
    int mType;

    KeyCharacterMap();

    bool getKey(int32_t keyCode, const Key** outKey) const;
    bool getKeyBehavior(int32_t keyCode, int32_t metaState,
            const Key** outKey, const Behavior** outBehavior) const;

    bool findKey(char16_t ch, int32_t* outKeyCode, int32_t* outMetaState) const;

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
