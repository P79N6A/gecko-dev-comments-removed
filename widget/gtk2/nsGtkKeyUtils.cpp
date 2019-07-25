







































#include "prlog.h"

#include "nsGtkKeyUtils.h"

#include <gdk/gdkkeysyms.h>
#ifndef GDK_Sleep
#define GDK_Sleep 0x1008ff2f
#endif

#include <gdk/gdk.h>
#ifdef MOZ_X11
#include <gdk/gdkx.h>
#endif 
#include "nsGUIEvent.h"
#include "keysym2ucs.h"

#ifdef PR_LOGGING
PRLogModuleInfo* gKeymapWrapperLog = nsnull;
#endif 

#include "mozilla/Util.h"

namespace mozilla {
namespace widget {

struct KeyPair {
    PRUint32 DOMKeyCode;
    guint GDKKeyval;
};





static const KeyPair kKeyPairs[] = {
    { NS_VK_CANCEL,     GDK_Cancel },
    { NS_VK_BACK,       GDK_BackSpace },
    { NS_VK_TAB,        GDK_Tab },
    { NS_VK_TAB,        GDK_ISO_Left_Tab },
    { NS_VK_CLEAR,      GDK_Clear },
    { NS_VK_RETURN,     GDK_Return },
    { NS_VK_SHIFT,      GDK_Shift_L },
    { NS_VK_SHIFT,      GDK_Shift_R },
    { NS_VK_CONTROL,    GDK_Control_L },
    { NS_VK_CONTROL,    GDK_Control_R },
    { NS_VK_ALT,        GDK_Alt_L },
    { NS_VK_ALT,        GDK_Alt_R },
    { NS_VK_META,       GDK_Meta_L },
    { NS_VK_META,       GDK_Meta_R },
    { NS_VK_PAUSE,      GDK_Pause },
    { NS_VK_CAPS_LOCK,  GDK_Caps_Lock },
    { NS_VK_KANA,       GDK_Kana_Lock },
    { NS_VK_KANA,       GDK_Kana_Shift },
    { NS_VK_HANGUL,     GDK_Hangul },
    
    
    { NS_VK_HANJA,      GDK_Hangul_Hanja },
    { NS_VK_KANJI,      GDK_Kanji },
    { NS_VK_ESCAPE,     GDK_Escape },
    { NS_VK_CONVERT,    GDK_Henkan },
    { NS_VK_NONCONVERT, GDK_Muhenkan },
    
    { NS_VK_MODECHANGE, GDK_Mode_switch },
    { NS_VK_SPACE,      GDK_space },
    { NS_VK_PAGE_UP,    GDK_Page_Up },
    { NS_VK_PAGE_DOWN,  GDK_Page_Down },
    { NS_VK_END,        GDK_End },
    { NS_VK_HOME,       GDK_Home },
    { NS_VK_LEFT,       GDK_Left },
    { NS_VK_UP,         GDK_Up },
    { NS_VK_RIGHT,      GDK_Right },
    { NS_VK_DOWN,       GDK_Down },
    { NS_VK_SELECT,     GDK_Select },
    { NS_VK_PRINT,      GDK_Print },
    { NS_VK_EXECUTE,    GDK_Execute },
    { NS_VK_PRINTSCREEN, GDK_Print },
    { NS_VK_INSERT,     GDK_Insert },
    { NS_VK_DELETE,     GDK_Delete },
    { NS_VK_HELP,       GDK_Help },

    
    { NS_VK_LEFT,       GDK_KP_Left },
    { NS_VK_RIGHT,      GDK_KP_Right },
    { NS_VK_UP,         GDK_KP_Up },
    { NS_VK_DOWN,       GDK_KP_Down },
    { NS_VK_PAGE_UP,    GDK_KP_Page_Up },
    
    
    
    
    
    { NS_VK_PAGE_DOWN,  GDK_KP_Page_Down },
    { NS_VK_HOME,       GDK_KP_Home },
    { NS_VK_END,        GDK_KP_End },
    { NS_VK_INSERT,     GDK_KP_Insert },
    { NS_VK_DELETE,     GDK_KP_Delete },

    { NS_VK_MULTIPLY,   GDK_KP_Multiply },
    { NS_VK_ADD,        GDK_KP_Add },
    { NS_VK_SEPARATOR,  GDK_KP_Separator },
    { NS_VK_SUBTRACT,   GDK_KP_Subtract },
    { NS_VK_DECIMAL,    GDK_KP_Decimal },
    { NS_VK_DIVIDE,     GDK_KP_Divide },
    { NS_VK_RETURN,     GDK_KP_Enter },
    { NS_VK_NUM_LOCK,   GDK_Num_Lock },
    { NS_VK_SCROLL_LOCK,GDK_Scroll_Lock },

    { NS_VK_COMMA,      GDK_comma },
    { NS_VK_PERIOD,     GDK_period },
    { NS_VK_SLASH,      GDK_slash },
    { NS_VK_BACK_SLASH, GDK_backslash },
    { NS_VK_BACK_QUOTE, GDK_grave },
    { NS_VK_OPEN_BRACKET, GDK_bracketleft },
    { NS_VK_CLOSE_BRACKET, GDK_bracketright },
    { NS_VK_SEMICOLON, GDK_colon },
    { NS_VK_QUOTE, GDK_apostrophe },

    
    
    { NS_VK_CONTEXT_MENU, GDK_Menu },
    { NS_VK_SLEEP,      GDK_Sleep },

    
    
    { NS_VK_SUBTRACT, GDK_minus },
    { NS_VK_EQUALS, GDK_equal },

    
    
    { NS_VK_QUOTE, GDK_quotedbl },
    { NS_VK_OPEN_BRACKET, GDK_braceleft },
    { NS_VK_CLOSE_BRACKET, GDK_braceright },
    { NS_VK_BACK_SLASH, GDK_bar },
    { NS_VK_SEMICOLON, GDK_semicolon },
    { NS_VK_BACK_QUOTE, GDK_asciitilde },
    { NS_VK_COMMA, GDK_less },
    { NS_VK_PERIOD, GDK_greater },
    { NS_VK_SLASH,      GDK_question },
    { NS_VK_1, GDK_exclam },
    { NS_VK_2, GDK_at },
    { NS_VK_3, GDK_numbersign },
    { NS_VK_4, GDK_dollar },
    { NS_VK_5, GDK_percent },
    { NS_VK_6, GDK_asciicircum },
    { NS_VK_7, GDK_ampersand },
    { NS_VK_8, GDK_asterisk },
    { NS_VK_9, GDK_parenleft },
    { NS_VK_0, GDK_parenright },
    { NS_VK_SUBTRACT, GDK_underscore },
    { NS_VK_EQUALS, GDK_plus }
};


static const KeyPair kSunKeyPairs[] = {
    {NS_VK_F11, 0x1005ff10 }, 
    {NS_VK_F12, 0x1005ff11 }  
};

#define MOZ_MODIFIER_KEYS "MozKeymapWrapper"

KeymapWrapper* KeymapWrapper::sInstance = nsnull;

#ifdef PR_LOGGING

static const char* GetBoolName(bool aBool)
{
    return aBool ? "TRUE" : "FALSE";
}

 const char*
KeymapWrapper::GetModifierName(Modifier aModifier)
{
    switch (aModifier) {
        case CAPS_LOCK:    return "CapsLock";
        case NUM_LOCK:     return "NumLock";
        case SCROLL_LOCK:  return "ScrollLock";
        case SHIFT:        return "Shift";
        case CTRL:         return "Ctrl";
        case ALT:          return "Alt";
        case SUPER:        return "Super";
        case HYPER:        return "Hyper";
        case META:         return "Meta";
        case ALTGR:        return "AltGr";
        case NOT_MODIFIER: return "NotModifier";
        default:           return "InvalidValue";
    }
}

#endif 

 KeymapWrapper::Modifier
KeymapWrapper::GetModifierForGDKKeyval(guint aGdkKeyval)
{
    switch (aGdkKeyval) {
        case GDK_Caps_Lock:        return CAPS_LOCK;
        case GDK_Num_Lock:         return NUM_LOCK;
        case GDK_Scroll_Lock:      return SCROLL_LOCK;
        case GDK_Shift_L:
        case GDK_Shift_R:          return SHIFT;
        case GDK_Control_L:
        case GDK_Control_R:        return CTRL;
        case GDK_Alt_L:
        case GDK_Alt_R:            return ALT;
        case GDK_Super_L:
        case GDK_Super_R:          return SUPER;
        case GDK_Hyper_L:
        case GDK_Hyper_R:          return HYPER;
        case GDK_Meta_L:
        case GDK_Meta_R:           return META;
        case GDK_ISO_Level3_Shift:
        case GDK_Mode_switch:      return ALTGR;
        default:                   return NOT_MODIFIER;
    }
}

guint
KeymapWrapper::GetModifierMask(Modifier aModifier) const
{
    switch (aModifier) {
        case CAPS_LOCK:
            return GDK_LOCK_MASK;
        case NUM_LOCK:
            return mModifierMasks[INDEX_NUM_LOCK];
        case SCROLL_LOCK:
            return mModifierMasks[INDEX_SCROLL_LOCK];
        case SHIFT:
            return GDK_SHIFT_MASK;
        case CTRL:
            return GDK_CONTROL_MASK;
        case ALT:
            return mModifierMasks[INDEX_ALT];
        case SUPER:
            return mModifierMasks[INDEX_SUPER];
        case HYPER:
            return mModifierMasks[INDEX_HYPER];
        case META:
            return mModifierMasks[INDEX_META];
        case ALTGR:
            return mModifierMasks[INDEX_ALTGR];
        default:
            return 0;
    }
}

KeymapWrapper::ModifierKey*
KeymapWrapper::GetModifierKey(guint aHardwareKeycode)
{
    for (PRUint32 i = 0; i < mModifierKeys.Length(); i++) {
        ModifierKey& key = mModifierKeys[i];
        if (key.mHardwareKeycode == aHardwareKeycode) {
            return &key;
        }
    }
    return nsnull;
}

 KeymapWrapper*
KeymapWrapper::GetInstance()
{
    if (sInstance) {
        sInstance->Init();
        return sInstance;
    }

    sInstance = new KeymapWrapper();
    return sInstance;
}

KeymapWrapper::KeymapWrapper() :
    mInitialized(false), mGdkKeymap(gdk_keymap_get_default())
{
#ifdef PR_LOGGING
    if (!gKeymapWrapperLog) {
        gKeymapWrapperLog = PR_NewLogModule("KeymapWrapperWidgets");
    }
    PR_LOG(gKeymapWrapperLog, PR_LOG_ALWAYS,
        ("KeymapWrapper(%p): Constructor, mGdkKeymap=%p",
         this, mGdkKeymap));
#endif 

    g_signal_connect(mGdkKeymap, "keys-changed",
                     (GCallback)OnKeysChanged, this);

    
    g_object_weak_ref(G_OBJECT(mGdkKeymap),
                      (GWeakNotify)OnDestroyKeymap, this);

    Init();
}

void
KeymapWrapper::Init()
{
    if (mInitialized) {
        return;
    }
    mInitialized = true;

    PR_LOG(gKeymapWrapperLog, PR_LOG_ALWAYS,
        ("KeymapWrapper(%p): Init, mGdkKeymap=%p",
         this, mGdkKeymap));

    mModifierKeys.Clear();
    memset(mModifierMasks, 0, sizeof(mModifierMasks));

    InitBySystemSettings();

    PR_LOG(gKeymapWrapperLog, PR_LOG_ALWAYS,
        ("KeymapWrapper(%p): Init, CapsLock=0x%X, NumLock=0x%X, "
         "ScrollLock=0x%X, AltGr=0x%X, Shift=0x%X, Ctrl=0x%X, Alt=0x%X, "
         "Meta=0x%X, Super=0x%X, Hyper=0x%X",
         this,
         GetModifierMask(CAPS_LOCK), GetModifierMask(NUM_LOCK),
         GetModifierMask(SCROLL_LOCK), GetModifierMask(ALTGR),
         GetModifierMask(SHIFT), GetModifierMask(CTRL),
         GetModifierMask(ALT), GetModifierMask(META),
         GetModifierMask(SUPER), GetModifierMask(HYPER)));
}

void
KeymapWrapper::InitBySystemSettings()
{
    PR_LOG(gKeymapWrapperLog, PR_LOG_ALWAYS,
      ("KeymapWrapper(%p): InitBySystemSettings, mGdkKeymap=%p",
       this, mGdkKeymap));

    Display* display =
        gdk_x11_display_get_xdisplay(gdk_display_get_default());

    int min_keycode = 0;
    int max_keycode = 0;
    XDisplayKeycodes(display, &min_keycode, &max_keycode);

    int keysyms_per_keycode = 0;
    KeySym* xkeymap = XGetKeyboardMapping(display, min_keycode,
                                          max_keycode - min_keycode + 1,
                                          &keysyms_per_keycode);
    if (!xkeymap) {
        PR_LOG(gKeymapWrapperLog, PR_LOG_ALWAYS,
            ("KeymapWrapper(%p): InitBySystemSettings, "
             "Failed due to null xkeymap", this));
        return;
    }

    XModifierKeymap* xmodmap = XGetModifierMapping(display);
    if (!xmodmap) {
        PR_LOG(gKeymapWrapperLog, PR_LOG_ALWAYS,
            ("KeymapWrapper(%p): InitBySystemSettings, "
             "Failed due to null xmodmap", this));
        XFree(xkeymap);
        return;
    }
    PR_LOG(gKeymapWrapperLog, PR_LOG_ALWAYS,
        ("KeymapWrapper(%p): InitBySystemSettings, min_keycode=%d, "
         "max_keycode=%d, keysyms_per_keycode=%d, max_keypermod=%d",
         this, min_keycode, max_keycode, keysyms_per_keycode,
         xmodmap->max_keypermod));

    
    
    
    
    

    
    
    

    const PRUint32 map_size = 8 * xmodmap->max_keypermod;
    for (PRUint32 i = 0; i < map_size; i++) {
        KeyCode keycode = xmodmap->modifiermap[i];
        PR_LOG(gKeymapWrapperLog, PR_LOG_ALWAYS,
            ("KeymapWrapper(%p): InitBySystemSettings, "
             "  i=%d, keycode=0x%08X",
             this, i, keycode));
        if (!keycode || keycode < min_keycode || keycode > max_keycode) {
            continue;
        }

        ModifierKey* modifierKey = GetModifierKey(keycode);
        if (!modifierKey) {
            modifierKey = mModifierKeys.AppendElement(ModifierKey(keycode));
        }

        const KeySym* syms =
            xkeymap + (keycode - min_keycode) * keysyms_per_keycode;
        const PRUint32 bit = i / xmodmap->max_keypermod;
        modifierKey->mMask |= 1 << bit;
        for (PRInt32 j = 0; j < keysyms_per_keycode; j++) {
            Modifier modifier = GetModifierForGDKKeyval(syms[j]);
            PR_LOG(gKeymapWrapperLog, PR_LOG_ALWAYS,
                ("KeymapWrapper(%p): InitBySystemSettings, "
                 "    bit=%d, j=%d, syms[j]=%s(0x%X), modifier=%s",
                 this, bit, j, gdk_keyval_name(syms[j]), syms[j],
                 GetModifierName(modifier)));

            ModifierIndex index;
            switch (modifier) {
                case NUM_LOCK:
                    index = INDEX_NUM_LOCK;
                    break;
                case SCROLL_LOCK:
                    index = INDEX_SCROLL_LOCK;
                    break;
                case ALT:
                    index = INDEX_ALT;
                    break;
                case SUPER:
                    index = INDEX_SUPER;
                    break;
                case HYPER:
                    index = INDEX_HYPER;
                    break;
                case META:
                    index = INDEX_META;
                    break;
                case ALTGR:
                    index = INDEX_ALTGR;
                    break;
                default:
                    
                    
                    
                    continue;
            }
            mModifierMasks[index] |= 1 << bit;
        }
    }

    XFreeModifiermap(xmodmap);
    XFree(xkeymap);
}

KeymapWrapper::~KeymapWrapper()
{
    PR_LOG(gKeymapWrapperLog, PR_LOG_ALWAYS,
        ("KeymapWrapper(%p): Destructor", this));
}

 void
KeymapWrapper::OnDestroyKeymap(KeymapWrapper* aKeymapWrapper,
                               GdkKeymap *aGdkKeymap)
{
    PR_LOG(gKeymapWrapperLog, PR_LOG_ALWAYS,
        ("KeymapWrapper: OnDestroyKeymap, aGdkKeymap=%p, aKeymapWrapper=%p",
         aGdkKeymap, aKeymapWrapper));
    MOZ_ASSERT(aKeymapWrapper == sInstance,
               "Desroying unexpected instance");
    delete sInstance;
    sInstance = nsnull;
}

 void
KeymapWrapper::OnKeysChanged(GdkKeymap *aGdkKeymap,
                             KeymapWrapper* aKeymapWrapper)
{
    PR_LOG(gKeymapWrapperLog, PR_LOG_ALWAYS,
        ("KeymapWrapper: OnKeysChanged, aGdkKeymap=%p, aKeymapWrapper=%p",
         aGdkKeymap, aKeymapWrapper));

    MOZ_ASSERT(sInstance == aKeymapWrapper,
               "This instance must be the singleton instance");

    
    
    sInstance->mInitialized = false;
}

 guint
KeymapWrapper::GetCurrentModifierState()
{
    GdkModifierType modifiers;
    gdk_display_get_pointer(gdk_display_get_default(),
                            NULL, NULL, NULL, &modifiers);
    return static_cast<guint>(modifiers);
}

 bool
KeymapWrapper::AreModifiersCurrentlyActive(Modifiers aModifiers)
{
    KeymapWrapper* keymapWrapper = GetInstance();
    guint modifierState = GetCurrentModifierState();
    return keymapWrapper->AreModifiersActive(aModifiers, modifierState);
}

bool
KeymapWrapper::AreModifiersActive(Modifiers aModifiers,
                                  guint aModifierState) const
{
    NS_ENSURE_TRUE(aModifiers, false);
    for (PRUint32 i = 0; i < sizeof(Modifier) * 8 && aModifiers; i++) {
        Modifier modifier = static_cast<Modifier>(1 << i);
        if (!(aModifiers & modifier)) {
            continue;
        }
        if (!(aModifierState & GetModifierMask(modifier))) {
            return false;
        }
        aModifiers &= ~modifier;
    }
    return true;
}

 void
KeymapWrapper::InitInputEvent(nsInputEvent& aInputEvent,
                              guint aModifierState)
{
    KeymapWrapper* keymapWrapper = GetInstance();

    aInputEvent.isShift =
        keymapWrapper->AreModifiersActive(SHIFT, aModifierState);
    aInputEvent.isControl =
        keymapWrapper->AreModifiersActive(CTRL, aModifierState);
    aInputEvent.isAlt =
        keymapWrapper->AreModifiersActive(ALT, aModifierState);
    
    
    aInputEvent.isMeta = false;

    PR_LOG(gKeymapWrapperLog, PR_LOG_DEBUG,
        ("KeymapWrapper(%p): InitInputEvent, aModifierState=0x%08X "
         "aKeyEvent={ isShift=%s, isControl=%s, isAlt=%s, isMeta=%s }",
         keymapWrapper, aModifierState,
         GetBoolName(aInputEvent.isShift), GetBoolName(aInputEvent.isControl),
         GetBoolName(aInputEvent.isAlt), GetBoolName(aInputEvent.isMeta)));
}

 PRUint32
KeymapWrapper::ComputeDOMKeyCode(guint aGDKKeyval)
{
    
    
    

    
    
    if (aGDKKeyval >= GDK_a && aGDKKeyval <= GDK_z) {
        return aGDKKeyval - GDK_a + NS_VK_A;
    }
    if (aGDKKeyval >= GDK_A && aGDKKeyval <= GDK_Z) {
        return aGDKKeyval - GDK_A + NS_VK_A;
    }

    
    if (aGDKKeyval >= GDK_0 && aGDKKeyval <= GDK_9) {
        return aGDKKeyval - GDK_0 + NS_VK_0;
    }

    
    if (aGDKKeyval >= GDK_KP_0 && aGDKKeyval <= GDK_KP_9) {
        return aGDKKeyval - GDK_KP_0 + NS_VK_NUMPAD0;
    }

    
    for (PRUint32 i = 0; i < ArrayLength(kSunKeyPairs); i++) {
        if (kSunKeyPairs[i].GDKKeyval == aGDKKeyval) {
            return kSunKeyPairs[i].DOMKeyCode;
        }
    }

    
    for (PRUint32 i = 0; i < ArrayLength(kKeyPairs); i++) {
        if (kKeyPairs[i].GDKKeyval == aGDKKeyval) {
            return kKeyPairs[i].DOMKeyCode;
        }
    }

    
    if (aGDKKeyval >= GDK_F1 && aGDKKeyval <= GDK_F24) {
        return aGDKKeyval - GDK_F1 + NS_VK_F1;
    }

    return 0;
}

 guint
KeymapWrapper::GuessGDKKeyval(PRUint32 aDOMKeyCode)
{
    
    
    

    if (aDOMKeyCode >= NS_VK_A && aDOMKeyCode <= NS_VK_Z) {
        
        return aDOMKeyCode;
    }

    
    if (aDOMKeyCode >= NS_VK_0 && aDOMKeyCode <= NS_VK_9) {
        
        return aDOMKeyCode - NS_VK_0 + GDK_0;
    }

    
    if (aDOMKeyCode >= NS_VK_NUMPAD0 && aDOMKeyCode <= NS_VK_NUMPAD9) {
        return aDOMKeyCode - NS_VK_NUMPAD0 + GDK_KP_0;
    }

    
    for (PRUint32 i = 0; i < ArrayLength(kKeyPairs); ++i) {
        if (kKeyPairs[i].DOMKeyCode == aDOMKeyCode) {
            return kKeyPairs[i].GDKKeyval;
        }
    }

    
    if (aDOMKeyCode >= NS_VK_F1 && aDOMKeyCode <= NS_VK_F9) {
        return aDOMKeyCode - NS_VK_F1 + GDK_F1;
    }

    return 0;
}

 void
KeymapWrapper::InitKeyEvent(nsKeyEvent& aKeyEvent,
                            GdkEventKey* aGdkKeyEvent)
{
    KeymapWrapper* keymapWrapper = GetInstance();

    aKeyEvent.keyCode = ComputeDOMKeyCode(aGdkKeyEvent->keyval);

    
    
    
    
    
    
    
    
    
    
    
    guint modifierState = aGdkKeyEvent->state;
    if (aGdkKeyEvent->is_modifier && aGdkKeyEvent->type == GDK_KEY_PRESS) {
        ModifierKey* modifierKey =
            keymapWrapper->GetModifierKey(aGdkKeyEvent->hardware_keycode);
        if (modifierKey) {
            
            modifierState |= modifierKey->mMask;
        }
    }
    InitInputEvent(aKeyEvent, modifierState);

    PR_LOG(gKeymapWrapperLog, PR_LOG_ALWAYS,
        ("KeymapWrapper(%p): InitKeyEvent, modifierState=0x%08X "
         "aGdkKeyEvent={ type=%s, keyval=%s(0x%X), state=0x%08X, "
         "hardware_keycode=0x%08X, is_modifier=%s } "
         "aKeyEvent={ message=%s, isShift=%s, isControl=%s, "
         "isAlt=%s, isMeta=%s }",
         keymapWrapper, modifierState,
         ((aGdkKeyEvent->type == GDK_KEY_PRESS) ?
               "GDK_KEY_PRESS" : "GDK_KEY_RELEASE"),
         gdk_keyval_name(aGdkKeyEvent->keyval),
         aGdkKeyEvent->keyval, aGdkKeyEvent->state,
         aGdkKeyEvent->hardware_keycode,
         GetBoolName(aGdkKeyEvent->is_modifier),
         ((aKeyEvent.message == NS_KEY_DOWN) ? "NS_KEY_DOWN" :
               (aKeyEvent.message == NS_KEY_PRESS) ? "NS_KEY_PRESS" :
                                                      "NS_KEY_UP"),
         GetBoolName(aKeyEvent.isShift), GetBoolName(aKeyEvent.isControl),
         GetBoolName(aKeyEvent.isAlt), GetBoolName(aKeyEvent.isMeta)));

    if (aKeyEvent.message == NS_KEY_PRESS) {
        keymapWrapper->InitKeypressEvent(aKeyEvent, aGdkKeyEvent);
    }

    
    
    
    
    aKeyEvent.pluginEvent = (void *)aGdkKeyEvent;
    aKeyEvent.time = aGdkKeyEvent->time;
}

 PRUint32
KeymapWrapper::GetCharCodeFor(const GdkEventKey *aGdkKeyEvent)
{
    
    
    if (aGdkKeyEvent->keyval > 0xf000 &&
        (aGdkKeyEvent->keyval & 0xff000000) != 0x01000000) {
        
        
        switch (aGdkKeyEvent->keyval) {
            case GDK_KP_Space:              return ' ';
            case GDK_KP_Equal:              return '=';
            case GDK_KP_Multiply:           return '*';
            case GDK_KP_Add:                return '+';
            case GDK_KP_Separator:          return ',';
            case GDK_KP_Subtract:           return '-';
            case GDK_KP_Decimal:            return '.';
            case GDK_KP_Divide:             return '/';
            case GDK_KP_0:                  return '0';
            case GDK_KP_1:                  return '1';
            case GDK_KP_2:                  return '2';
            case GDK_KP_3:                  return '3';
            case GDK_KP_4:                  return '4';
            case GDK_KP_5:                  return '5';
            case GDK_KP_6:                  return '6';
            case GDK_KP_7:                  return '7';
            case GDK_KP_8:                  return '8';
            case GDK_KP_9:                  return '9';
            default:                        return 0; 
        }
    }

    static const long MAX_UNICODE = 0x10FFFF;

    
    long ucs = keysym2ucs(aGdkKeyEvent->keyval);
    if ((ucs != -1) && (ucs < MAX_UNICODE)) {
         return ucs;
    }

    
    return 0;
}

PRUint32
KeymapWrapper::GetCharCodeFor(const GdkEventKey *aGdkKeyEvent,
                              guint aModifierState,
                              gint aGroup)
{
    guint keyval;
    if (!gdk_keymap_translate_keyboard_state(mGdkKeymap,
             aGdkKeyEvent->hardware_keycode,
             GdkModifierType(aModifierState),
             aGroup, &keyval, NULL, NULL, NULL)) {
        return 0;
    }
    GdkEventKey tmpEvent = *aGdkKeyEvent;
    tmpEvent.state = aModifierState;
    tmpEvent.keyval = keyval;
    tmpEvent.group = aGroup;
    return GetCharCodeFor(&tmpEvent);
}


gint
KeymapWrapper::GetKeyLevel(GdkEventKey *aGdkKeyEvent)
{
    gint level;
    if (!gdk_keymap_translate_keyboard_state(mGdkKeymap,
             aGdkKeyEvent->hardware_keycode,
             GdkModifierType(aGdkKeyEvent->state),
             aGdkKeyEvent->group, NULL, NULL, &level, NULL)) {
        return -1;
    }
    return level;
}

 PRBool
KeymapWrapper::IsBasicLatinLetterOrNumeral(PRUint32 aCharCode)
{
    return (aCharCode >= 'a' && aCharCode <= 'z') ||
           (aCharCode >= 'A' && aCharCode <= 'Z') ||
           (aCharCode >= '0' && aCharCode <= '9');
}

void
KeymapWrapper::InitKeypressEvent(nsKeyEvent& aKeyEvent,
                                 GdkEventKey* aGdkKeyEvent)
{
    NS_ENSURE_TRUE(aKeyEvent.message == NS_KEY_PRESS, );

    aKeyEvent.charCode = GetCharCodeFor(aGdkKeyEvent);
    if (!aKeyEvent.charCode) {
        PR_LOG(gKeymapWrapperLog, PR_LOG_ALWAYS,
            ("KeymapWrapper(%p): InitKeypressEvent, "
             "keyCode=0x%02X, charCode=0x%08X",
             this, aKeyEvent.keyCode, aKeyEvent.charCode));
        return;
    }

    
    aKeyEvent.keyCode = 0;

    
    
    if (!aKeyEvent.isControl && !aKeyEvent.isAlt && !aKeyEvent.isMeta) {
        PR_LOG(gKeymapWrapperLog, PR_LOG_ALWAYS,
            ("KeymapWrapper(%p): InitKeypressEvent, "
             "keyCode=0x%02X, charCode=0x%08X",
             this, aKeyEvent.keyCode, aKeyEvent.charCode));
        return;
    }

    gint level = GetKeyLevel(aGdkKeyEvent);
    if (level != 0 && level != 1) {
        PR_LOG(gKeymapWrapperLog, PR_LOG_ALWAYS,
            ("KeymapWrapper(%p): InitKeypressEvent, "
             "keyCode=0x%02X, charCode=0x%08X, level=%d",
             this, aKeyEvent.keyCode, aKeyEvent.charCode, level));
        return;
    }

    guint baseState = aGdkKeyEvent->state &
        ~(GetModifierMask(SHIFT) | GetModifierMask(CTRL) |
          GetModifierMask(ALT) | GetModifierMask(META) |
          GetModifierMask(SUPER) | GetModifierMask(HYPER));

    
    
    
    nsAlternativeCharCode altCharCodes(0, 0);
    
    altCharCodes.mUnshiftedCharCode =
        GetCharCodeFor(aGdkKeyEvent, baseState, aGdkKeyEvent->group);
    PRBool isLatin = (altCharCodes.mUnshiftedCharCode <= 0xFF);
    
    altCharCodes.mShiftedCharCode =
        GetCharCodeFor(aGdkKeyEvent,
                       baseState | GetModifierMask(SHIFT),
                       aGdkKeyEvent->group);
    isLatin = isLatin && (altCharCodes.mShiftedCharCode <= 0xFF);
    if (altCharCodes.mUnshiftedCharCode || altCharCodes.mShiftedCharCode) {
        aKeyEvent.alternativeCharCodes.AppendElement(altCharCodes);
    }

    
    
    if (isLatin) {
        PR_LOG(gKeymapWrapperLog, PR_LOG_ALWAYS,
            ("KeymapWrapper(%p): InitKeypressEvent, keyCode=0x%02X, "
             "charCode=0x%08X, level=%d, altCharCodes={ "
             "mUnshiftedCharCode=0x%08X, mShiftedCharCode=0x%08X }",
             this, aKeyEvent.keyCode, aKeyEvent.charCode, level,
             altCharCodes.mUnshiftedCharCode, altCharCodes.mShiftedCharCode));
        return;
    }

    
    GdkKeymapKey *keys;
    gint count;
    gint minGroup = -1;
    if (gdk_keymap_get_entries_for_keyval(mGdkKeymap, GDK_a, &keys, &count)) {
        
        for (gint i = 0; i < count && minGroup != 0; ++i) {
            if (keys[i].level != 0 && keys[i].level != 1) {
                continue;
            }
            if (minGroup >= 0 && keys[i].group > minGroup) {
                continue;
            }
            minGroup = keys[i].group;
        }
        g_free(keys);
    }

    if (minGroup < 0) {
        PR_LOG(gKeymapWrapperLog, PR_LOG_ALWAYS,
            ("KeymapWrapper(%p): InitKeypressEvent, "
             "Latin keyboard layout isn't found: "
             "keyCode=0x%02X, charCode=0x%08X, level=%d, "
             "altCharCodes={ mUnshiftedCharCode=0x%08X, "
             "mShiftedCharCode=0x%08X }",
             this, aKeyEvent.keyCode, aKeyEvent.charCode, level,
             altCharCodes.mUnshiftedCharCode, altCharCodes.mShiftedCharCode));
        return;
    }

    nsAlternativeCharCode altLatinCharCodes(0, 0);
    PRUint32 unmodifiedCh =
        aKeyEvent.isShift ? altCharCodes.mShiftedCharCode :
                            altCharCodes.mUnshiftedCharCode;

    
    PRUint32 ch = GetCharCodeFor(aGdkKeyEvent, baseState, minGroup);
    altLatinCharCodes.mUnshiftedCharCode =
        IsBasicLatinLetterOrNumeral(ch) ? ch : 0;
    
    ch = GetCharCodeFor(aGdkKeyEvent,
                        baseState | GetModifierMask(SHIFT),
                        minGroup);
    altLatinCharCodes.mShiftedCharCode =
        IsBasicLatinLetterOrNumeral(ch) ? ch : 0;
    if (altLatinCharCodes.mUnshiftedCharCode ||
        altLatinCharCodes.mShiftedCharCode) {
        aKeyEvent.alternativeCharCodes.AppendElement(altLatinCharCodes);
    }
    
    
    
    
    ch = aKeyEvent.isShift ? altLatinCharCodes.mShiftedCharCode :
                             altLatinCharCodes.mUnshiftedCharCode;
    if (ch && !(aKeyEvent.isAlt || aKeyEvent.isMeta) &&
        aKeyEvent.charCode == unmodifiedCh) {
        aKeyEvent.charCode = ch;
    }

    PR_LOG(gKeymapWrapperLog, PR_LOG_ALWAYS,
        ("KeymapWrapper(%p): InitKeypressEvent, "
         "keyCode=0x%02X, charCode=0x%08X, level=%d, minGroup=%d, "
         "altCharCodes={ mUnshiftedCharCode=0x%08X, "
         "mShiftedCharCode=0x%08X } "
         "altLatinCharCodes={ mUnshiftedCharCode=0x%08X, "
         "mShiftedCharCode=0x%08X }",
         this, aKeyEvent.keyCode, aKeyEvent.charCode, level, minGroup,
         altCharCodes.mUnshiftedCharCode, altCharCodes.mShiftedCharCode,
         altLatinCharCodes.mUnshiftedCharCode,
         altLatinCharCodes.mShiftedCharCode));
}

} 
} 
