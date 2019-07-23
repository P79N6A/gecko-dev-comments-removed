




































#ifndef WindowGfx_h__
#define WindowGfx_h__





#include "nsWindow.h"
#include <imgIContainer.h>




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

  static nsresult CreateIcon(imgIContainer *aContainer, PRBool aIsCursor, PRUint32 aHotspotX, PRUint32 aHotspotY, HICON *aIcon);

private:
  


  static PRUint8*         Data32BitTo1Bit(PRUint8* aImageData, PRUint32 aWidth, PRUint32 aHeight);
  static PRBool           IsCursorTranslucencySupported();
  static HBITMAP          DataToBitmap(PRUint8* aImageData, PRUint32 aWidth, PRUint32 aHeight, PRUint32 aDepth);
};

#endif 
