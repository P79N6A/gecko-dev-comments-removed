






#include "prlog.h"

#include "nsGtkKeyUtils.h"

#include <gdk/gdkkeysyms.h>
#include <algorithm>
#ifndef GDK_Sleep
#define GDK_Sleep 0x1008ff2f
#endif

#include <gdk/gdk.h>
#ifdef MOZ_X11
#include <gdk/gdkx.h>
#endif 
#if (MOZ_WIDGET_GTK == 3)
#include <gdk/gdkkeysyms-compat.h>
#endif
#include "nsGUIEvent.h"
#include "WidgetUtils.h"
#include "keysym2ucs.h"

#ifdef PR_LOGGING
PRLogModuleInfo* gKeymapWrapperLog = nullptr;
#endif 

#include "mozilla/Util.h"

namespace mozilla {
namespace widget {

struct KeyPair {
    uint32_t DOMKeyCode;
    guint GDKKeyval;
};

#define IS_ASCII_ALPHABETICAL(key) \
    ((('a' <= key) && (key <= 'z')) || (('A' <= key) && (key <= 'Z')))





static const KeyPair kKeyPairs[] = {
    { NS_VK_CANCEL,     GDK_Cancel },
    { NS_VK_BACK,       GDK_BackSpace },
    { NS_VK_TAB,        GDK_Tab },
    { NS_VK_TAB,        GDK_ISO_Left_Tab },
    { NS_VK_CLEAR,      GDK_Clear },
    { NS_VK_RETURN,     GDK_Return },
    { NS_VK_SHIFT,      GDK_Shift_L },
    { NS_VK_SHIFT,      GDK_Shift_R },
    { NS_VK_SHIFT,      GDK_Shift_Lock },
    { NS_VK_CONTROL,    GDK_Control_L },
    { NS_VK_CONTROL,    GDK_Control_R },
    { NS_VK_ALT,        GDK_Alt_L },
    { NS_VK_ALT,        GDK_Alt_R },
    { NS_VK_META,       GDK_Meta_L },
    { NS_VK_META,       GDK_Meta_R },

    
    { NS_VK_WIN,        GDK_Super_L },
    { NS_VK_WIN,        GDK_Super_R },
    { NS_VK_WIN,        GDK_Hyper_L },
    { NS_VK_WIN,        GDK_Hyper_R },

    
    
    
    
    
    
    
    
    
    { NS_VK_ALTGR,      GDK_ISO_Level3_Shift },
    { NS_VK_ALTGR,      GDK_ISO_Level5_Shift },
    
    { NS_VK_ALTGR,      GDK_Mode_switch },

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
    
    
    
    { NS_VK_CLEAR,      GDK_KP_Begin }, 
    { NS_VK_PAGE_DOWN,  GDK_KP_Page_Down },
    { NS_VK_HOME,       GDK_KP_Home },
    { NS_VK_END,        GDK_KP_End },
    { NS_VK_INSERT,     GDK_KP_Insert },
    { NS_VK_DELETE,     GDK_KP_Delete },
    { NS_VK_RETURN,     GDK_KP_Enter },

    { NS_VK_NUM_LOCK,   GDK_Num_Lock },
    { NS_VK_SCROLL_LOCK,GDK_Scroll_Lock },

    
    { NS_VK_F1,         GDK_F1 },
    { NS_VK_F2,         GDK_F2 },
    { NS_VK_F3,         GDK_F3 },
    { NS_VK_F4,         GDK_F4 },
    { NS_VK_F5,         GDK_F5 },
    { NS_VK_F6,         GDK_F6 },
    { NS_VK_F7,         GDK_F7 },
    { NS_VK_F8,         GDK_F8 },
    { NS_VK_F9,         GDK_F9 },
    { NS_VK_F10,        GDK_F10 },
    { NS_VK_F11,        GDK_F11 },
    { NS_VK_F12,        GDK_F12 },
    { NS_VK_F13,        GDK_F13 },
    { NS_VK_F14,        GDK_F14 },
    { NS_VK_F15,        GDK_F15 },
    { NS_VK_F16,        GDK_F16 },
    { NS_VK_F17,        GDK_F17 },
    { NS_VK_F18,        GDK_F18 },
    { NS_VK_F19,        GDK_F19 },
    { NS_VK_F20,        GDK_F20 },
    { NS_VK_F21,        GDK_F21 },
    { NS_VK_F22,        GDK_F22 },
    { NS_VK_F23,        GDK_F23 },
    { NS_VK_F24,        GDK_F24 },

    
    
    { NS_VK_CONTEXT_MENU, GDK_Menu },
    { NS_VK_SLEEP,      GDK_Sleep },

    { NS_VK_ATTN,       GDK_3270_Attn },
    { NS_VK_CRSEL,      GDK_3270_CursorSelect },
    { NS_VK_EXSEL,      GDK_3270_ExSelect },
    { NS_VK_EREOF,      GDK_3270_EraseEOF },
    { NS_VK_PLAY,       GDK_3270_Play },
    
    { NS_VK_PA1,        GDK_3270_PA1 },
};


static const KeyPair kSunKeyPairs[] = {
    {NS_VK_F11, 0x1005ff10 }, 
    {NS_VK_F12, 0x1005ff11 }  
};

#define MOZ_MODIFIER_KEYS "MozKeymapWrapper"

KeymapWrapper* KeymapWrapper::sInstance = nullptr;

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
        case LEVEL3:       return "Level3";
        case LEVEL5:       return "Level5";
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
        case GDK_Shift_Lock:
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
        case GDK_Mode_switch:      return LEVEL3;
        case GDK_ISO_Level5_Shift: return LEVEL5;
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
        case LEVEL3:
            return mModifierMasks[INDEX_LEVEL3];
        case LEVEL5:
            return mModifierMasks[INDEX_LEVEL5];
        default:
            return 0;
    }
}

KeymapWrapper::ModifierKey*
KeymapWrapper::GetModifierKey(guint aHardwareKeycode)
{
    for (uint32_t i = 0; i < mModifierKeys.Length(); i++) {
        ModifierKey& key = mModifierKeys[i];
        if (key.mHardwareKeycode == aHardwareKeycode) {
            return &key;
        }
    }
    return nullptr;
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
         "ScrollLock=0x%X, Level3=0x%X, Level5=0x%X, "
         "Shift=0x%X, Ctrl=0x%X, Alt=0x%X, Meta=0x%X, Super=0x%X, Hyper=0x%X",
         this,
         GetModifierMask(CAPS_LOCK), GetModifierMask(NUM_LOCK),
         GetModifierMask(SCROLL_LOCK), GetModifierMask(LEVEL3),
         GetModifierMask(LEVEL5),
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

    
    
    
    
    

    
    
    
    

    
    Modifier mod[5];
    int32_t foundLevel[5];
    for (uint32_t i = 0; i < ArrayLength(mod); i++) {
        mod[i] = NOT_MODIFIER;
        foundLevel[i] = INT32_MAX;
    }
    const uint32_t map_size = 8 * xmodmap->max_keypermod;
    for (uint32_t i = 0; i < map_size; i++) {
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
        const uint32_t bit = i / xmodmap->max_keypermod;
        modifierKey->mMask |= 1 << bit;

        
        
        if (bit < 3) {
            continue;
        }

        const int32_t modIndex = bit - 3;
        for (int32_t j = 0; j < keysyms_per_keycode; j++) {
            Modifier modifier = GetModifierForGDKKeyval(syms[j]);
            PR_LOG(gKeymapWrapperLog, PR_LOG_ALWAYS,
                ("KeymapWrapper(%p): InitBySystemSettings, "
                 "    Mod%d, j=%d, syms[j]=%s(0x%X), modifier=%s",
                 this, modIndex + 1, j, gdk_keyval_name(syms[j]), syms[j],
                 GetModifierName(modifier)));

            switch (modifier) {
                case NOT_MODIFIER:
                    
                    
                    break;
                case CAPS_LOCK:
                case SHIFT:
                case CTRL:
                    
                    
                    
                    break;
                default:
                    
                    
                    if (j > foundLevel[modIndex]) {
                        break;
                    }
                    
                    
                    if (j == foundLevel[modIndex]) {
                        mod[modIndex] = std::min(modifier, mod[modIndex]);
                        break;
                    }
                    foundLevel[modIndex] = j;
                    mod[modIndex] = modifier;
                    break;
            }
        }
    }

    for (uint32_t i = 0; i < COUNT_OF_MODIFIER_INDEX; i++) {
        Modifier modifier;
        switch (i) {
            case INDEX_NUM_LOCK:
                modifier = NUM_LOCK;
                break;
            case INDEX_SCROLL_LOCK:
                modifier = SCROLL_LOCK;
                break;
            case INDEX_ALT:
                modifier = ALT;
                break;
            case INDEX_META:
                modifier = META;
                break;
            case INDEX_SUPER:
                modifier = SUPER;
                break;
            case INDEX_HYPER:
                modifier = HYPER;
                break;
            case INDEX_LEVEL3:
                modifier = LEVEL3;
                break;
            case INDEX_LEVEL5:
                modifier = LEVEL5;
                break;
            default:
                MOZ_NOT_REACHED("All indexes must be handled here");
                break;
        }
        for (uint32_t j = 0; j < ArrayLength(mod); j++) {
            if (modifier == mod[j]) {
                mModifierMasks[i] |= 1 << (j + 3);
            }
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
    sInstance = nullptr;
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
    guint modifierState = GetCurrentModifierState();
    return AreModifiersActive(aModifiers, modifierState);
}

 bool
KeymapWrapper::AreModifiersActive(Modifiers aModifiers,
                                  guint aModifierState)
{
    NS_ENSURE_TRUE(aModifiers, false);

    KeymapWrapper* keymapWrapper = GetInstance();
    for (uint32_t i = 0; i < sizeof(Modifier) * 8 && aModifiers; i++) {
        Modifier modifier = static_cast<Modifier>(1 << i);
        if (!(aModifiers & modifier)) {
            continue;
        }
        if (!(aModifierState & keymapWrapper->GetModifierMask(modifier))) {
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

    aInputEvent.modifiers = 0;
    
    
    if (keymapWrapper->AreModifiersActive(SHIFT, aModifierState)) {
        aInputEvent.modifiers |= MODIFIER_SHIFT;
    }
    if (keymapWrapper->AreModifiersActive(CTRL, aModifierState)) {
        aInputEvent.modifiers |= MODIFIER_CONTROL;
    }
    if (keymapWrapper->AreModifiersActive(ALT, aModifierState)) {
        aInputEvent.modifiers |= MODIFIER_ALT;
    }
    if (keymapWrapper->AreModifiersActive(META, aModifierState)) {
        aInputEvent.modifiers |= MODIFIER_META;
    }
    if (keymapWrapper->AreModifiersActive(SUPER, aModifierState) ||
        keymapWrapper->AreModifiersActive(HYPER, aModifierState)) {
        aInputEvent.modifiers |= MODIFIER_OS;
    }
    if (keymapWrapper->AreModifiersActive(LEVEL3, aModifierState) ||
        keymapWrapper->AreModifiersActive(LEVEL5, aModifierState)) {
        aInputEvent.modifiers |= MODIFIER_ALTGRAPH;
    }
    if (keymapWrapper->AreModifiersActive(CAPS_LOCK, aModifierState)) {
        aInputEvent.modifiers |= MODIFIER_CAPSLOCK;
    }
    if (keymapWrapper->AreModifiersActive(NUM_LOCK, aModifierState)) {
        aInputEvent.modifiers |= MODIFIER_NUMLOCK;
    }
    if (keymapWrapper->AreModifiersActive(SCROLL_LOCK, aModifierState)) {
        aInputEvent.modifiers |= MODIFIER_SCROLLLOCK;
    }

    PR_LOG(gKeymapWrapperLog, PR_LOG_DEBUG,
        ("KeymapWrapper(%p): InitInputEvent, aModifierState=0x%08X, "
         "aInputEvent.modifiers=0x%04X (Shift: %s, Control: %s, Alt: %s, "
         "Meta: %s, OS: %s, AltGr: %s, "
         "CapsLock: %s, NumLock: %s, ScrollLock: %s)",
         keymapWrapper, aModifierState, aInputEvent.modifiers,
         GetBoolName(aInputEvent.modifiers & MODIFIER_SHIFT),
         GetBoolName(aInputEvent.modifiers & MODIFIER_CONTROL),
         GetBoolName(aInputEvent.modifiers & MODIFIER_ALT),
         GetBoolName(aInputEvent.modifiers & MODIFIER_META),
         GetBoolName(aInputEvent.modifiers & MODIFIER_OS),
         GetBoolName(aInputEvent.modifiers & MODIFIER_ALTGRAPH),
         GetBoolName(aInputEvent.modifiers & MODIFIER_CAPSLOCK),
         GetBoolName(aInputEvent.modifiers & MODIFIER_NUMLOCK),
         GetBoolName(aInputEvent.modifiers & MODIFIER_SCROLLLOCK)));

    switch(aInputEvent.eventStructType) {
        case NS_MOUSE_EVENT:
        case NS_MOUSE_SCROLL_EVENT:
        case NS_WHEEL_EVENT:
        case NS_DRAG_EVENT:
        case NS_SIMPLE_GESTURE_EVENT:
            break;
        default:
            return;
    }

    nsMouseEvent_base& mouseEvent = static_cast<nsMouseEvent_base&>(aInputEvent);
    mouseEvent.buttons = 0;
    if (aModifierState & GDK_BUTTON1_MASK) {
        mouseEvent.buttons |= nsMouseEvent::eLeftButtonFlag;
    }
    if (aModifierState & GDK_BUTTON3_MASK) {
        mouseEvent.buttons |= nsMouseEvent::eRightButtonFlag;
    }
    if (aModifierState & GDK_BUTTON2_MASK) {
        mouseEvent.buttons |= nsMouseEvent::eMiddleButtonFlag;
    }

    PR_LOG(gKeymapWrapperLog, PR_LOG_DEBUG,
        ("KeymapWrapper(%p): InitInputEvent, aInputEvent has buttons, "
         "aInputEvent.buttons=0x%04X (Left: %s, Right: %s, Middle: %s, "
         "4th (BACK): %s, 5th (FORWARD): %s)",
         keymapWrapper, mouseEvent.buttons,
         GetBoolName(mouseEvent.buttons & nsMouseEvent::eLeftButtonFlag),
         GetBoolName(mouseEvent.buttons & nsMouseEvent::eRightButtonFlag),
         GetBoolName(mouseEvent.buttons & nsMouseEvent::eMiddleButtonFlag),
         GetBoolName(mouseEvent.buttons & nsMouseEvent::e4thButtonFlag),
         GetBoolName(mouseEvent.buttons & nsMouseEvent::e5thButtonFlag)));
}

 uint32_t
KeymapWrapper::ComputeDOMKeyCode(const GdkEventKey* aGdkKeyEvent)
{
    
    
    guint keyval = aGdkKeyEvent->keyval;
    if (GetModifierForGDKKeyval(keyval)) {
        
        
        
        
        
        guint keyvalWithoutModifier = GetGDKKeyvalWithoutModifier(aGdkKeyEvent);
        if (GetModifierForGDKKeyval(keyvalWithoutModifier)) {
            keyval = keyvalWithoutModifier;
        }
        
        
        
        
        
        
        
        
        uint32_t DOMKeyCode = GetDOMKeyCodeFromKeyPairs(keyval);
        NS_ASSERTION(DOMKeyCode, "All modifier keys must have a DOM keycode");
        return DOMKeyCode;
    }

    
    uint32_t charCode = GetCharCodeFor(aGdkKeyEvent);
    if (!charCode) {
        
        
        
        guint keyvalWithoutModifier = GetGDKKeyvalWithoutModifier(aGdkKeyEvent);
        uint32_t DOMKeyCode = GetDOMKeyCodeFromKeyPairs(keyvalWithoutModifier);
        if (!DOMKeyCode) {
            
            
            
            DOMKeyCode = GetDOMKeyCodeFromKeyPairs(keyval);
        }
        return DOMKeyCode;
    }

    
    switch (keyval) {
        case GDK_KP_Multiply:  return NS_VK_MULTIPLY;
        case GDK_KP_Add:       return NS_VK_ADD;
        case GDK_KP_Separator: return NS_VK_SEPARATOR;
        case GDK_KP_Subtract:  return NS_VK_SUBTRACT;
        case GDK_KP_Decimal:   return NS_VK_DECIMAL;
        case GDK_KP_Divide:    return NS_VK_DIVIDE;
        case GDK_KP_0:         return NS_VK_NUMPAD0;
        case GDK_KP_1:         return NS_VK_NUMPAD1;
        case GDK_KP_2:         return NS_VK_NUMPAD2;
        case GDK_KP_3:         return NS_VK_NUMPAD3;
        case GDK_KP_4:         return NS_VK_NUMPAD4;
        case GDK_KP_5:         return NS_VK_NUMPAD5;
        case GDK_KP_6:         return NS_VK_NUMPAD6;
        case GDK_KP_7:         return NS_VK_NUMPAD7;
        case GDK_KP_8:         return NS_VK_NUMPAD8;
        case GDK_KP_9:         return NS_VK_NUMPAD9;
    }

    KeymapWrapper* keymapWrapper = GetInstance();

    
    guint baseState =
        (aGdkKeyEvent->state & keymapWrapper->GetModifierMask(NUM_LOCK));

    
    uint32_t unmodifiedChar =
        keymapWrapper->GetCharCodeFor(aGdkKeyEvent, baseState,
                                      aGdkKeyEvent->group);
    if (IsBasicLatinLetterOrNumeral(unmodifiedChar)) {
        
        
        return WidgetUtils::ComputeKeyCodeFromChar(unmodifiedChar);
    }

    
    
    if (unmodifiedChar > 0x7F) {
        unmodifiedChar = 0;
    }

    
    guint shiftState = (baseState | keymapWrapper->GetModifierMask(SHIFT));
    uint32_t shiftedChar =
        keymapWrapper->GetCharCodeFor(aGdkKeyEvent, shiftState,
                                      aGdkKeyEvent->group);
    if (IsBasicLatinLetterOrNumeral(shiftedChar)) {
        
        
        
        
        return WidgetUtils::ComputeKeyCodeFromChar(shiftedChar);
    }

    
    
    if (shiftedChar > 0x7F) {
        shiftedChar = 0;
    }

    
    
    
    
    
    
    
    
    if (!keymapWrapper->IsLatinGroup(aGdkKeyEvent->group)) {
        gint minGroup = keymapWrapper->GetFirstLatinGroup();
        if (minGroup >= 0) {
            uint32_t unmodCharLatin =
                keymapWrapper->GetCharCodeFor(aGdkKeyEvent, baseState,
                                              minGroup);
            if (IsBasicLatinLetterOrNumeral(unmodCharLatin)) {
                
                
                return WidgetUtils::ComputeKeyCodeFromChar(unmodCharLatin);
            }
            uint32_t shiftedCharLatin =
                keymapWrapper->GetCharCodeFor(aGdkKeyEvent, shiftState,
                                              minGroup);
            if (IsBasicLatinLetterOrNumeral(shiftedCharLatin)) {
                
                
                return WidgetUtils::ComputeKeyCodeFromChar(shiftedCharLatin);
            }
        }
    }

    
    
    if (!unmodifiedChar && !shiftedChar) {
        return 0;
    }
    return WidgetUtils::ComputeKeyCodeFromChar(
                unmodifiedChar ? unmodifiedChar : shiftedChar);
}

 guint
KeymapWrapper::GuessGDKKeyval(uint32_t aDOMKeyCode)
{
    
    
    

    if (aDOMKeyCode >= NS_VK_A && aDOMKeyCode <= NS_VK_Z) {
        
        return aDOMKeyCode;
    }

    
    if (aDOMKeyCode >= NS_VK_0 && aDOMKeyCode <= NS_VK_9) {
        
        return aDOMKeyCode - NS_VK_0 + GDK_0;
    }

    switch (aDOMKeyCode) {
        
        case NS_VK_MULTIPLY:  return GDK_KP_Multiply;
        case NS_VK_ADD:       return GDK_KP_Add;
        case NS_VK_SEPARATOR: return GDK_KP_Separator;
        case NS_VK_SUBTRACT:  return GDK_KP_Subtract;
        case NS_VK_DECIMAL:   return GDK_KP_Decimal;
        case NS_VK_DIVIDE:    return GDK_KP_Divide;
        case NS_VK_NUMPAD0:   return GDK_KP_0;
        case NS_VK_NUMPAD1:   return GDK_KP_1;
        case NS_VK_NUMPAD2:   return GDK_KP_2;
        case NS_VK_NUMPAD3:   return GDK_KP_3;
        case NS_VK_NUMPAD4:   return GDK_KP_4;
        case NS_VK_NUMPAD5:   return GDK_KP_5;
        case NS_VK_NUMPAD6:   return GDK_KP_6;
        case NS_VK_NUMPAD7:   return GDK_KP_7;
        case NS_VK_NUMPAD8:   return GDK_KP_8;
        case NS_VK_NUMPAD9:   return GDK_KP_9;
        
        case NS_VK_SPACE:               return GDK_space;
        case NS_VK_COLON:               return GDK_colon;
        case NS_VK_SEMICOLON:           return GDK_semicolon;
        case NS_VK_LESS_THAN:           return GDK_less;
        case NS_VK_EQUALS:              return GDK_equal;
        case NS_VK_GREATER_THAN:        return GDK_greater;
        case NS_VK_QUESTION_MARK:       return GDK_question;
        case NS_VK_AT:                  return GDK_at;
        case NS_VK_CIRCUMFLEX:          return GDK_asciicircum;
        case NS_VK_EXCLAMATION:         return GDK_exclam;
        case NS_VK_DOUBLE_QUOTE:        return GDK_quotedbl;
        case NS_VK_HASH:                return GDK_numbersign;
        case NS_VK_DOLLAR:              return GDK_dollar;
        case NS_VK_PERCENT:             return GDK_percent;
        case NS_VK_AMPERSAND:           return GDK_ampersand;
        case NS_VK_UNDERSCORE:          return GDK_underscore;
        case NS_VK_OPEN_PAREN:          return GDK_parenleft;
        case NS_VK_CLOSE_PAREN:         return GDK_parenright;
        case NS_VK_ASTERISK:            return GDK_asterisk;
        case NS_VK_PLUS:                return GDK_plus;
        case NS_VK_PIPE:                return GDK_bar;
        case NS_VK_HYPHEN_MINUS:        return GDK_minus;
        case NS_VK_OPEN_CURLY_BRACKET:  return GDK_braceleft;
        case NS_VK_CLOSE_CURLY_BRACKET: return GDK_braceright;
        case NS_VK_TILDE:               return GDK_asciitilde;
        case NS_VK_COMMA:               return GDK_comma;
        case NS_VK_PERIOD:              return GDK_period;
        case NS_VK_SLASH:               return GDK_slash;
        case NS_VK_BACK_QUOTE:          return GDK_grave;
        case NS_VK_OPEN_BRACKET:        return GDK_bracketleft;
        case NS_VK_BACK_SLASH:          return GDK_backslash;
        case NS_VK_CLOSE_BRACKET:       return GDK_bracketright;
        case NS_VK_QUOTE:               return GDK_apostrophe;
    }

    
    for (uint32_t i = 0; i < ArrayLength(kKeyPairs); ++i) {
        if (kKeyPairs[i].DOMKeyCode == aDOMKeyCode) {
            return kKeyPairs[i].GDKKeyval;
        }
    }

    return 0;
}

 void
KeymapWrapper::InitKeyEvent(nsKeyEvent& aKeyEvent,
                            GdkEventKey* aGdkKeyEvent)
{
    KeymapWrapper* keymapWrapper = GetInstance();

    aKeyEvent.keyCode = ComputeDOMKeyCode(aGdkKeyEvent);

    
    
    
    
    
    
    
    
    
    
    
    guint modifierState = aGdkKeyEvent->state;
    if (aGdkKeyEvent->is_modifier && aGdkKeyEvent->type == GDK_KEY_PRESS) {
        ModifierKey* modifierKey =
            keymapWrapper->GetModifierKey(aGdkKeyEvent->hardware_keycode);
        if (modifierKey) {
            
            modifierState |= modifierKey->mMask;
        }
    }
    InitInputEvent(aKeyEvent, modifierState);

#ifdef MOZ_PLATFORM_MAEMO
    aKeyEvent.location = nsIDOMKeyEvent::DOM_KEY_LOCATION_MOBILE;
#else 
    switch (aGdkKeyEvent->keyval) {
        case GDK_Shift_L:
        case GDK_Control_L:
        case GDK_Alt_L:
        case GDK_Super_L:
        case GDK_Hyper_L:
        case GDK_Meta_L:
            aKeyEvent.location = nsIDOMKeyEvent::DOM_KEY_LOCATION_LEFT;
            break;

        case GDK_Shift_R:
        case GDK_Control_R:
        case GDK_Alt_R:
        case GDK_Super_R:
        case GDK_Hyper_R:
        case GDK_Meta_R:
            aKeyEvent.location = nsIDOMKeyEvent::DOM_KEY_LOCATION_RIGHT;
            break;

        case GDK_KP_0:
        case GDK_KP_1:
        case GDK_KP_2:
        case GDK_KP_3:
        case GDK_KP_4:
        case GDK_KP_5:
        case GDK_KP_6:
        case GDK_KP_7:
        case GDK_KP_8:
        case GDK_KP_9:
        case GDK_KP_Space:
        case GDK_KP_Tab:
        case GDK_KP_Enter:
        case GDK_KP_F1:
        case GDK_KP_F2:
        case GDK_KP_F3:
        case GDK_KP_F4:
        case GDK_KP_Home:
        case GDK_KP_Left:
        case GDK_KP_Up:
        case GDK_KP_Right:
        case GDK_KP_Down:
        case GDK_KP_Prior: 
        case GDK_KP_Next:  
        case GDK_KP_End:
        case GDK_KP_Begin:
        case GDK_KP_Insert:
        case GDK_KP_Delete:
        case GDK_KP_Equal:
        case GDK_KP_Multiply:
        case GDK_KP_Add:
        case GDK_KP_Separator:
        case GDK_KP_Subtract:
        case GDK_KP_Decimal:
        case GDK_KP_Divide:
            aKeyEvent.location = nsIDOMKeyEvent::DOM_KEY_LOCATION_NUMPAD;
            break;

        default:
            aKeyEvent.location = nsIDOMKeyEvent::DOM_KEY_LOCATION_STANDARD;
            break;
    }
#endif 

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
         GetBoolName(aKeyEvent.IsShift()), GetBoolName(aKeyEvent.IsControl()),
         GetBoolName(aKeyEvent.IsAlt()), GetBoolName(aKeyEvent.IsMeta())));

    if (aKeyEvent.message == NS_KEY_PRESS) {
        keymapWrapper->InitKeypressEvent(aKeyEvent, aGdkKeyEvent);
    }

    
    
    
    
    aKeyEvent.pluginEvent = (void *)aGdkKeyEvent;
    aKeyEvent.time = aGdkKeyEvent->time;
}

 uint32_t
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

uint32_t
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

gint
KeymapWrapper::GetFirstLatinGroup()
{
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
    return minGroup;
}

bool
KeymapWrapper::IsLatinGroup(guint8 aGroup)
{
    GdkKeymapKey *keys;
    gint count;
    bool result = false;
    if (gdk_keymap_get_entries_for_keyval(mGdkKeymap, GDK_a, &keys, &count)) {
        for (gint i = 0; i < count; ++i) {
            if (keys[i].level != 0 && keys[i].level != 1) {
                continue;
            }
            if (keys[i].group == aGroup) {
                result = true;
                break;
            }
        }
        g_free(keys);
    }
    return result;
}

 bool
KeymapWrapper::IsBasicLatinLetterOrNumeral(uint32_t aCharCode)
{
    return (aCharCode >= 'a' && aCharCode <= 'z') ||
           (aCharCode >= 'A' && aCharCode <= 'Z') ||
           (aCharCode >= '0' && aCharCode <= '9');
}

 guint
KeymapWrapper::GetGDKKeyvalWithoutModifier(const GdkEventKey *aGdkKeyEvent)
{
    KeymapWrapper* keymapWrapper = GetInstance();
    guint state =
        (aGdkKeyEvent->state & keymapWrapper->GetModifierMask(NUM_LOCK));
    guint keyval;
    if (!gdk_keymap_translate_keyboard_state(keymapWrapper->mGdkKeymap,
             aGdkKeyEvent->hardware_keycode, GdkModifierType(state),
             aGdkKeyEvent->group, &keyval, NULL, NULL, NULL)) {
        return 0;
    }
    return keyval;
}

 uint32_t
KeymapWrapper::GetDOMKeyCodeFromKeyPairs(guint aGdkKeyval)
{
    
    for (uint32_t i = 0; i < ArrayLength(kSunKeyPairs); i++) {
        if (kSunKeyPairs[i].GDKKeyval == aGdkKeyval) {
            return kSunKeyPairs[i].DOMKeyCode;
        }
    }

    
    for (uint32_t i = 0; i < ArrayLength(kKeyPairs); i++) {
        if (kKeyPairs[i].GDKKeyval == aGdkKeyval) {
            return kKeyPairs[i].DOMKeyCode;
        }
    }

    return 0;
}

void
KeymapWrapper::InitKeypressEvent(nsKeyEvent& aKeyEvent,
                                 GdkEventKey* aGdkKeyEvent)
{
    NS_ENSURE_TRUE_VOID(aKeyEvent.message == NS_KEY_PRESS);

    aKeyEvent.charCode = GetCharCodeFor(aGdkKeyEvent);
    if (!aKeyEvent.charCode) {
        PR_LOG(gKeymapWrapperLog, PR_LOG_ALWAYS,
            ("KeymapWrapper(%p): InitKeypressEvent, "
             "keyCode=0x%02X, charCode=0x%08X",
             this, aKeyEvent.keyCode, aKeyEvent.charCode));
        return;
    }

    
    aKeyEvent.keyCode = 0;

    
    
    
    if (!aKeyEvent.IsControl() && !aKeyEvent.IsAlt() &&
        !aKeyEvent.IsMeta() && !aKeyEvent.IsOS()) {
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
    bool isLatin = (altCharCodes.mUnshiftedCharCode <= 0xFF);
    
    altCharCodes.mShiftedCharCode =
        GetCharCodeFor(aGdkKeyEvent,
                       baseState | GetModifierMask(SHIFT),
                       aGdkKeyEvent->group);
    isLatin = isLatin && (altCharCodes.mShiftedCharCode <= 0xFF);
    if (altCharCodes.mUnshiftedCharCode || altCharCodes.mShiftedCharCode) {
        aKeyEvent.alternativeCharCodes.AppendElement(altCharCodes);
    }

    bool needLatinKeyCodes = !isLatin;
    if (!needLatinKeyCodes) {
        needLatinKeyCodes = 
            (IS_ASCII_ALPHABETICAL(altCharCodes.mUnshiftedCharCode) !=
             IS_ASCII_ALPHABETICAL(altCharCodes.mShiftedCharCode));
    }

    
    
    if (!needLatinKeyCodes) {
        PR_LOG(gKeymapWrapperLog, PR_LOG_ALWAYS,
            ("KeymapWrapper(%p): InitKeypressEvent, keyCode=0x%02X, "
             "charCode=0x%08X, level=%d, altCharCodes={ "
             "mUnshiftedCharCode=0x%08X, mShiftedCharCode=0x%08X }",
             this, aKeyEvent.keyCode, aKeyEvent.charCode, level,
             altCharCodes.mUnshiftedCharCode, altCharCodes.mShiftedCharCode));
        return;
    }

    
    gint minGroup = GetFirstLatinGroup();
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
    uint32_t unmodifiedCh =
        aKeyEvent.IsShift() ? altCharCodes.mShiftedCharCode :
                              altCharCodes.mUnshiftedCharCode;

    
    uint32_t ch = GetCharCodeFor(aGdkKeyEvent, baseState, minGroup);
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
    
    
    
    
    ch = aKeyEvent.IsShift() ? altLatinCharCodes.mShiftedCharCode :
                               altLatinCharCodes.mUnshiftedCharCode;
    if (ch && !(aKeyEvent.IsAlt() || aKeyEvent.IsMeta()) &&
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

 bool
KeymapWrapper::IsKeyPressEventNecessary(GdkEventKey* aGdkKeyEvent)
{
    
    switch (ComputeDOMKeyCode(aGdkKeyEvent)) {
        case NS_VK_SHIFT:
        case NS_VK_CONTROL:
        case NS_VK_ALT:
        case NS_VK_ALTGR:
        case NS_VK_WIN:
        case NS_VK_CAPS_LOCK:
        case NS_VK_NUM_LOCK:
        case NS_VK_SCROLL_LOCK:
            return false;
        default:
            return true;
    }
}

} 
} 
