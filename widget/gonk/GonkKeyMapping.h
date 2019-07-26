














#ifndef GONKKEYMAPPING_H
#define GONKKEYMAPPING_H

#include "libui/android_keycodes.h"
#include "mozilla/EventForwards.h"

namespace mozilla {
namespace widget {


static const unsigned long kKeyMapping[] = {
    0,
    0, 
    0, 
    NS_VK_HOME, 
    0, 
    0, 
    NS_VK_SLEEP, 
    NS_VK_0,
    NS_VK_1,
    NS_VK_2,
    NS_VK_3,
    NS_VK_4,
    NS_VK_5,
    NS_VK_6,
    NS_VK_7,
    NS_VK_8,
    NS_VK_9,
    NS_VK_ASTERISK,
    NS_VK_HASH,
    NS_VK_UP,
    NS_VK_DOWN,
    NS_VK_LEFT,
    NS_VK_RIGHT,
    NS_VK_SELECT,
    NS_VK_PAGE_UP,   
    NS_VK_PAGE_DOWN, 
    NS_VK_SLEEP,     
    NS_VK_PRINTSCREEN, 
    NS_VK_CLEAR,
    NS_VK_A,
    NS_VK_B,
    NS_VK_C,
    NS_VK_D,
    NS_VK_E,
    NS_VK_F,
    NS_VK_G,
    NS_VK_H,
    NS_VK_I,
    NS_VK_J,
    NS_VK_K,
    NS_VK_L,
    NS_VK_M,
    NS_VK_N,
    NS_VK_O,
    NS_VK_P,
    NS_VK_Q,
    NS_VK_R,
    NS_VK_S,
    NS_VK_T,
    NS_VK_U,
    NS_VK_V,
    NS_VK_W,
    NS_VK_X,
    NS_VK_Y,
    NS_VK_Z,
    NS_VK_COMMA,
    NS_VK_PERIOD,
    0,
    0,
    0,
    0,
    NS_VK_TAB,
    NS_VK_SPACE,
    NS_VK_META, 
    0, 
    0, 
    NS_VK_RETURN, 
    NS_VK_BACK,
    NS_VK_BACK_QUOTE, 
    NS_VK_HYPHEN_MINUS,
    NS_VK_EQUALS,
    NS_VK_OPEN_BRACKET,
    NS_VK_CLOSE_BRACKET,
    NS_VK_BACK_SLASH,
    NS_VK_SEMICOLON,
    NS_VK_QUOTE,
    NS_VK_SLASH,
    NS_VK_AT,
    0, 
    NS_VK_F1, 
    0, 
    NS_VK_PLUS,
    NS_VK_CONTEXT_MENU,
    0, 
    NS_VK_F5, 
    0, 
    0, 
    0, 
    0, 
    0, 
    0, 
    0, 
    0, 
    0, 
    0, 
    0, 
    0, 
    0, 
    0, 
    0, 
    0, 
    0, 
    0, 
    0, 
    0, 
    0, 
    0, 
    0, 
    0, 
    0, 
    0, 
    0, 
    NS_VK_DELETE,
    0, 
    0, 
    NS_VK_CAPS_LOCK,
    NS_VK_SCROLL_LOCK,
    0, 
    0, 
    0, 
    0, 
    0, 
    NS_VK_HOME, 
    NS_VK_END,
    NS_VK_INSERT,
    0, 
    0, 
    0, 
    0, 
    0, 
    0, 
    NS_VK_F1,
    NS_VK_F2,
    NS_VK_F3,
    NS_VK_F4,
    NS_VK_F5,
    NS_VK_F6,
    NS_VK_F7,
    NS_VK_F8,
    NS_VK_F9,
    NS_VK_F10,
    NS_VK_F11,
    NS_VK_F12,
    NS_VK_NUM_LOCK,
    NS_VK_NUMPAD0,
    NS_VK_NUMPAD1,
    NS_VK_NUMPAD2,
    NS_VK_NUMPAD3,
    NS_VK_NUMPAD4,
    NS_VK_NUMPAD5,
    NS_VK_NUMPAD6,
    NS_VK_NUMPAD7,
    NS_VK_NUMPAD8,
    NS_VK_NUMPAD9,
    NS_VK_DIVIDE,
    NS_VK_MULTIPLY,
    NS_VK_SUBTRACT,
    NS_VK_ADD,
    NS_VK_PERIOD,
    NS_VK_COMMA,
    NS_VK_RETURN,
    NS_VK_EQUALS,
    
};

static KeyNameIndex GetKeyNameIndex(int aKeyCode)
{
    switch (aKeyCode) {
#define NS_NATIVE_KEY_TO_DOM_KEY_NAME_INDEX(aNativeKey, aKeyNameIndex) \
    case aNativeKey: return aKeyNameIndex;

#include "NativeKeyToDOMKeyName.h"

#undef NS_NATIVE_KEY_TO_DOM_KEY_NAME_INDEX

    case AKEYCODE_0:
    case AKEYCODE_1:
    case AKEYCODE_2:
    case AKEYCODE_3:
    case AKEYCODE_4:
    case AKEYCODE_5:
    case AKEYCODE_6:
    case AKEYCODE_7:
    case AKEYCODE_8:
    case AKEYCODE_9:
    case AKEYCODE_STAR:
    case AKEYCODE_POUND:
    case AKEYCODE_A:
    case AKEYCODE_B:
    case AKEYCODE_C:
    case AKEYCODE_D:
    case AKEYCODE_E:
    case AKEYCODE_F:
    case AKEYCODE_G:
    case AKEYCODE_H:
    case AKEYCODE_I:
    case AKEYCODE_J:
    case AKEYCODE_K:
    case AKEYCODE_L:
    case AKEYCODE_M:
    case AKEYCODE_N:
    case AKEYCODE_O:
    case AKEYCODE_P:
    case AKEYCODE_Q:
    case AKEYCODE_R:
    case AKEYCODE_S:
    case AKEYCODE_T:
    case AKEYCODE_U:
    case AKEYCODE_V:
    case AKEYCODE_W:
    case AKEYCODE_X:
    case AKEYCODE_Y:
    case AKEYCODE_Z:
    case AKEYCODE_COMMA:
    case AKEYCODE_PERIOD:
    case AKEYCODE_SPACE:
    case AKEYCODE_GRAVE:
    case AKEYCODE_MINUS:
    case AKEYCODE_EQUALS:
    case AKEYCODE_LEFT_BRACKET:
    case AKEYCODE_RIGHT_BRACKET:
    case AKEYCODE_BACKSLASH:
    case AKEYCODE_SEMICOLON:
    case AKEYCODE_APOSTROPHE:
    case AKEYCODE_SLASH:
    case AKEYCODE_AT:
    case AKEYCODE_PLUS:
    case AKEYCODE_NUMPAD_0:
    case AKEYCODE_NUMPAD_1:
    case AKEYCODE_NUMPAD_2:
    case AKEYCODE_NUMPAD_3:
    case AKEYCODE_NUMPAD_4:
    case AKEYCODE_NUMPAD_5:
    case AKEYCODE_NUMPAD_6:
    case AKEYCODE_NUMPAD_7:
    case AKEYCODE_NUMPAD_8:
    case AKEYCODE_NUMPAD_9:
    case AKEYCODE_NUMPAD_DIVIDE:
    case AKEYCODE_NUMPAD_MULTIPLY:
    case AKEYCODE_NUMPAD_SUBTRACT:
    case AKEYCODE_NUMPAD_ADD:
    case AKEYCODE_NUMPAD_DOT:
    case AKEYCODE_NUMPAD_COMMA:
    case AKEYCODE_NUMPAD_EQUALS:
    case AKEYCODE_NUMPAD_LEFT_PAREN:
    case AKEYCODE_NUMPAD_RIGHT_PAREN:
        return KEY_NAME_INDEX_USE_STRING;

    default:
        return KEY_NAME_INDEX_Unidentified;
    }
}

static CodeNameIndex GetCodeNameIndex(int aScanCode)
{
    switch (aScanCode) {
#define NS_NATIVE_KEY_TO_DOM_CODE_NAME_INDEX(aNativeKey, aCodeNameIndex) \
    case aNativeKey: return aCodeNameIndex;

#include "NativeKeyToDOMCodeName.h"

#undef NS_NATIVE_KEY_TO_DOM_CODE_NAME_INDEX

    default:
        return CODE_NAME_INDEX_UNKNOWN;
    }
}

} 
} 

#endif 
