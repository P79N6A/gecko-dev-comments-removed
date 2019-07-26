






#include "prlink.h"

#include "nsBidiKeyboard.h"
#include <gtk/gtk.h>

#if (MOZ_WIDGET_GTK == 2)
typedef gboolean (*GdkKeymapHaveBidiLayoutsType)(GdkKeymap *keymap);
static GdkKeymapHaveBidiLayoutsType GdkKeymapHaveBidiLayouts = nullptr;
#endif

NS_IMPL_ISUPPORTS1(nsBidiKeyboard, nsIBidiKeyboard)

nsBidiKeyboard::nsBidiKeyboard()
{
    Reset();
}

NS_IMETHODIMP
nsBidiKeyboard::Reset()
{
#if (MOZ_WIDGET_GTK == 2)
    PRLibrary *gtklib = nullptr;
#if defined(MOZ_X11)
    if (!GdkKeymapHaveBidiLayouts) {
        GdkKeymapHaveBidiLayouts = (GdkKeymapHaveBidiLayoutsType) 
            PR_FindFunctionSymbolAndLibrary("gdk_keymap_have_bidi_layouts",
                                            &gtklib);
        if (gtklib)
            PR_UnloadLibrary(gtklib);
    }
#endif

    mHaveBidiKeyboards = false;
    if (GdkKeymapHaveBidiLayouts)
        mHaveBidiKeyboards = (*GdkKeymapHaveBidiLayouts)(nullptr);
#else
    mHaveBidiKeyboards = gdk_keymap_have_bidi_layouts(gdk_keymap_get_default());
#endif
    return NS_OK;
}

nsBidiKeyboard::~nsBidiKeyboard()
{
#if (MOZ_WIDGET_GTK == 2)
    GdkKeymapHaveBidiLayouts = nullptr;
#endif
}

NS_IMETHODIMP
nsBidiKeyboard::IsLangRTL(bool *aIsRTL)
{
    if (!mHaveBidiKeyboards)
        return NS_ERROR_FAILURE;

    *aIsRTL = (gdk_keymap_get_direction(gdk_keymap_get_default()) == PANGO_DIRECTION_RTL);

    return NS_OK;
}

NS_IMETHODIMP
nsBidiKeyboard::SetLangFromBidiLevel(uint8_t aLevel)
{
    
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsBidiKeyboard::GetHaveBidiKeyboards(bool* aResult)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}
