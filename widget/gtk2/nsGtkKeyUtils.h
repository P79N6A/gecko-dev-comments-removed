





































#ifndef __nsGdkKeyUtils_h__
#define __nsGdkKeyUtils_h__

#include "nsEvent.h"

#include <gdk/gdk.h>

int      GdkKeyCodeToDOMKeyCode     (int aKeysym);
int      DOMKeyCodeToGdkKeyCode     (int aKeysym);
PRUint32 nsConvertCharCodeToUnicode (GdkEventKey* aEvent);

namespace mozilla {
namespace widget {












class KeymapWrapper
{
protected:

    




    static KeymapWrapper* GetInstance();

    KeymapWrapper();
    ~KeymapWrapper();

    bool mInitialized;

    


    GdkKeymap* mGdkKeymap;

    


    static KeymapWrapper* sInstance;

    


    static void OnKeysChanged(GdkKeymap* aKeymap, KeymapWrapper* aKeymapWrapper);
    static void OnDestroyKeymap(KeymapWrapper* aKeymapWrapper,
                                GdkKeymap *aGdkKeymap);
};

} 
} 

#endif 
