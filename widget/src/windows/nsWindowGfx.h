




































#ifndef WindowGfx_h__
#define WindowGfx_h__

#include "nsWindow.h"




#include "cairo-features.h"

#ifdef CAIRO_HAS_DDRAW_SURFACE
#include "gfxDDrawSurface.h"
#endif

class nsWindowGfx {
public:
  static void AddRECTToRegion(const RECT& aRect, nsIRegion* aRegion);
  static already_AddRefed<nsIRegion> ConvertHRGNToRegion(HRGN aRgn);
  static void OnSettingsChangeGfx(WPARAM wParam);

#if defined(CAIRO_HAS_DDRAW_SURFACE)
  static PRBool InitDDraw();
#endif 
};

#endif 