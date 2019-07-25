




































#include "gfxThebesUtils.h"


nsIntRect
gfxThebesUtils::GfxRectToIntRect(const gfxRect& aIn)
{
  nsIntRect result(PRInt32(aIn.X()), PRInt32(aIn.Y()),
                   PRInt32(aIn.Width()), PRInt32(aIn.Height()));
  NS_ASSERTION(gfxRect(result.x, result.y, result.width, result.height) == aIn,
               "The given gfxRect isn't rounded properly!");
  return result;
}
