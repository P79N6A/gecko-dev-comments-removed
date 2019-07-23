






































#include "prlink.h"

#include "nsBidiKeyboard.h"
#include <gtk/gtk.h>


static PRLibrary *gtklib = nsnull;

typedef gboolean (PR_CALLBACK *GdkKeymapHaveBidiLayoutsType)(GdkKeymap *keymap);
static GdkKeymapHaveBidiLayoutsType GdkKeymapHaveBidiLayouts = nsnull;


NS_IMPL_ISUPPORTS1(nsBidiKeyboard, nsIBidiKeyboard)

nsBidiKeyboard::nsBidiKeyboard()
{
    if (!gtklib)
        gtklib = PR_LoadLibrary("libgtk-x11-2.0.so.0");

    if (gtklib && !GdkKeymapHaveBidiLayouts)
            GdkKeymapHaveBidiLayouts = (GdkKeymapHaveBidiLayoutsType) PR_FindFunctionSymbol(gtklib, "gdk_keymap_have_bidi_layouts");

    SetHaveBidiKeyboards();
}

nsBidiKeyboard::~nsBidiKeyboard()
{
    if (gtklib) {
        PR_UnloadLibrary(gtklib);
        gtklib = nsnull;

        GdkKeymapHaveBidiLayouts = nsnull;
    }
}

NS_IMETHODIMP
nsBidiKeyboard::IsLangRTL(PRBool *aIsRTL)
{
    if (!mHaveBidiKeyboards)
        return NS_ERROR_FAILURE;

    *aIsRTL = (gdk_keymap_get_direction(NULL) == PANGO_DIRECTION_RTL);

    return NS_OK;
}

nsresult
nsBidiKeyboard::SetHaveBidiKeyboards()
{
    mHaveBidiKeyboards = PR_FALSE;

    if (!gtklib || !GdkKeymapHaveBidiLayouts)
        return NS_ERROR_FAILURE;

    mHaveBidiKeyboards = (*GdkKeymapHaveBidiLayouts)(NULL);

    return NS_OK;
}

NS_IMETHODIMP
nsBidiKeyboard::SetLangFromBidiLevel(PRUint8 aLevel)
{
    
    return NS_ERROR_NOT_IMPLEMENTED;
}

