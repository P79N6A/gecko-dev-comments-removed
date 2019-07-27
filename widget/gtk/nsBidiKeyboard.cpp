






#include "prlink.h"

#include "nsBidiKeyboard.h"
#include <gtk/gtk.h>

NS_IMPL_ISUPPORTS(nsBidiKeyboard, nsIBidiKeyboard)

nsBidiKeyboard::nsBidiKeyboard()
{
    Reset();
}

NS_IMETHODIMP
nsBidiKeyboard::Reset()
{
    mHaveBidiKeyboards = gdk_keymap_have_bidi_layouts(gdk_keymap_get_default());
    return NS_OK;
}

nsBidiKeyboard::~nsBidiKeyboard()
{
}

NS_IMETHODIMP
nsBidiKeyboard::IsLangRTL(bool *aIsRTL)
{
    if (!mHaveBidiKeyboards)
        return NS_ERROR_FAILURE;

    *aIsRTL = (gdk_keymap_get_direction(gdk_keymap_get_default()) == PANGO_DIRECTION_RTL);

    return NS_OK;
}

NS_IMETHODIMP nsBidiKeyboard::GetHaveBidiKeyboards(bool* aResult)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}
