






#ifndef __nsGdkKeyUtils_h__
#define __nsGdkKeyUtils_h__

#include "nsTArray.h"
#include "mozilla/EventForwards.h"

#include <gdk/gdk.h>
#include <X11/XKBlib.h>

namespace mozilla {
namespace widget {












class KeymapWrapper
{
public:
    


    static uint32_t ComputeDOMKeyCode(const GdkEventKey* aGdkKeyEvent);

    


    KeyNameIndex ComputeDOMKeyNameIndex(const GdkEventKey* aGdkKeyEvent);

    


    static CodeNameIndex ComputeDOMCodeNameIndex(
                           const GdkEventKey* aGdkKeyEvent);

    


    enum Modifier {
        NOT_MODIFIER       = 0x0000,
        CAPS_LOCK          = 0x0001,
        NUM_LOCK           = 0x0002,
        SCROLL_LOCK        = 0x0004,
        SHIFT              = 0x0008,
        CTRL               = 0x0010,
        ALT                = 0x0020,
        META               = 0x0040,
        SUPER              = 0x0080,
        HYPER              = 0x0100,
        LEVEL3             = 0x0200,
        LEVEL5             = 0x0400
    };

    



    typedef uint32_t Modifiers;

    







    static guint GetCurrentModifierState();

    








    static bool AreModifiersCurrentlyActive(Modifiers aModifiers);

    










    static bool AreModifiersActive(Modifiers aModifiers,
                                   guint aModifierState);

    


    static void InitInputEvent(WidgetInputEvent& aInputEvent,
                               guint aModifierState);

    







    static void InitKeyEvent(WidgetKeyboardEvent& aKeyEvent,
                             GdkEventKey* aGdkKeyEvent);

    



    static bool IsKeyPressEventNecessary(GdkEventKey* aGdkKeyEvent);

protected:

    




    static KeymapWrapper* GetInstance();

    KeymapWrapper();
    ~KeymapWrapper();

    bool mInitialized;

    


    void Init();
    void InitXKBExtension();
    void InitBySystemSettings();

    


    struct ModifierKey {
        guint mHardwareKeycode;
        guint mMask;

        explicit ModifierKey(guint aHardwareKeycode) :
          mHardwareKeycode(aHardwareKeycode), mMask(0)
        {
        }
    };
    nsTArray<ModifierKey> mModifierKeys;

    



    ModifierKey* GetModifierKey(guint aHardwareKeycode);

    



    enum ModifierIndex {
        INDEX_NUM_LOCK,
        INDEX_SCROLL_LOCK,
        INDEX_ALT,
        INDEX_META,
        INDEX_SUPER,
        INDEX_HYPER,
        INDEX_LEVEL3,
        INDEX_LEVEL5,
        COUNT_OF_MODIFIER_INDEX
    };
    guint mModifierMasks[COUNT_OF_MODIFIER_INDEX];

    guint GetModifierMask(Modifier aModifier) const;

    






    static Modifier GetModifierForGDKKeyval(guint aGdkKeyval);

#ifdef PR_LOGGING
    static const char* GetModifierName(Modifier aModifier);
#endif 

    


    GdkKeymap* mGdkKeymap;

    


    int mXKBBaseEventCode;

    





    XKeyboardState mKeyboardState;

    


    static KeymapWrapper* sInstance;

    


    static guint sLastRepeatableHardwareKeyCode;
    enum RepeatState
    {
        NOT_PRESSED,
        FIRST_PRESS,
        REPEATING
    };
    static RepeatState sRepeatState;

    



    bool IsAutoRepeatableKey(guint aHardwareKeyCode);

    


    static void OnKeysChanged(GdkKeymap* aKeymap, KeymapWrapper* aKeymapWrapper);
    static void OnDestroyKeymap(KeymapWrapper* aKeymapWrapper,
                                GdkKeymap *aGdkKeymap);

    










    static uint32_t GetCharCodeFor(const GdkEventKey *aGdkKeyEvent);
    uint32_t GetCharCodeFor(const GdkEventKey *aGdkKeyEvent,
                            guint aModifierState,
                            gint aGroup);

    









    uint32_t GetUnmodifiedCharCodeFor(const GdkEventKey* aGdkKeyEvent);

    






    gint GetKeyLevel(GdkEventKey *aGdkKeyEvent);

    





    gint GetFirstLatinGroup();

    







    bool IsLatinGroup(guint8 aGroup);

    







    static bool IsBasicLatinLetterOrNumeral(uint32_t aCharCode);

    




    static guint GetGDKKeyvalWithoutModifier(const GdkEventKey *aGdkKeyEvent);

    



    static uint32_t GetDOMKeyCodeFromKeyPairs(guint aGdkKeyval);

    









    void InitKeypressEvent(WidgetKeyboardEvent& aKeyEvent,
                           GdkEventKey* aGdkKeyEvent);

    




    static GdkFilterReturn FilterEvents(GdkXEvent* aXEvent,
                                        GdkEvent* aGdkEvent,
                                        gpointer aData);
};

} 
} 

#endif
