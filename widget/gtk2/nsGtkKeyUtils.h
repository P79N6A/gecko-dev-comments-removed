





































#ifndef __nsGdkKeyUtils_h__
#define __nsGdkKeyUtils_h__

#include "nsEvent.h"
#include "nsTArray.h"

#include <gdk/gdk.h>

namespace mozilla {
namespace widget {












class KeymapWrapper
{
public:
    


    static PRUint32 ComputeDOMKeyCode(guint aGDKKeyval);

    




    static guint GuessGDKKeyval(PRUint32 aDOMKeyCode);

    


    enum Modifier {
        NOT_MODIFIER       = 0x0000,
        CAPS_LOCK          = 0x0001,
        NUM_LOCK           = 0x0002,
        SCROLL_LOCK        = 0x0004,
        SHIFT              = 0x0008,
        CTRL               = 0x0010,
        ALT                = 0x0020,
        SUPER              = 0x0040,
        HYPER              = 0x0080,
        META               = 0x0100,
        ALTGR              = 0x0200
    };

    



    typedef PRUint32 Modifiers;

    







    static guint GetCurrentModifierState();

    








    static bool AreModifiersCurrentlyActive(Modifiers aModifiers);

    


    static void InitInputEvent(nsInputEvent& aInputEvent,
                               guint aModifierState);

    







    static void InitKeyEvent(nsKeyEvent& aKeyEvent, GdkEventKey* aGdkKeyEvent);

    



    static bool IsKeyPressEventNecessary(GdkEventKey* aGdkKeyEvent);

protected:

    




    static KeymapWrapper* GetInstance();

    KeymapWrapper();
    ~KeymapWrapper();

    bool mInitialized;

    


    void Init();
    void InitBySystemSettings();

    


    struct ModifierKey {
        guint mHardwareKeycode;
        guint mMask;

        ModifierKey(guint aHardwareKeycode) :
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
        INDEX_SUPER,
        INDEX_HYPER,
        INDEX_META,
        INDEX_ALTGR,
        COUNT_OF_MODIFIER_INDEX
    };
    guint mModifierMasks[COUNT_OF_MODIFIER_INDEX];

    guint GetModifierMask(Modifier aModifier) const;

    






    static Modifier GetModifierForGDKKeyval(guint aGdkKeyval);

#ifdef PR_LOGGING
    static const char* GetModifierName(Modifier aModifier);
#endif 

    










    bool AreModifiersActive(Modifiers aModifiers,
                            guint aModifierState) const;

    


    GdkKeymap* mGdkKeymap;

    


    static KeymapWrapper* sInstance;

    


    static void OnKeysChanged(GdkKeymap* aKeymap, KeymapWrapper* aKeymapWrapper);
    static void OnDestroyKeymap(KeymapWrapper* aKeymapWrapper,
                                GdkKeymap *aGdkKeymap);

    










    static PRUint32 GetCharCodeFor(const GdkEventKey *aGdkKeyEvent);
    PRUint32 GetCharCodeFor(const GdkEventKey *aGdkKeyEvent,
                            guint aModifierState,
                            gint aGroup);

    






    gint GetKeyLevel(GdkEventKey *aGdkKeyEvent);

    







    static PRBool IsBasicLatinLetterOrNumeral(PRUint32 aCharCode);

    









    void InitKeypressEvent(nsKeyEvent& aKeyEvent, GdkEventKey* aGdkKeyEvent);
};

} 
} 

#endif
