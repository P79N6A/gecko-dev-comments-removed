




#ifndef WindowGfx_h__
#define WindowGfx_h__





#include "nsWindow.h"
#include <imgIContainer.h>




#include "cairo-features.h"

class nsWindowGfx {
public:
  static nsIntRect ToIntRect(const RECT& aRect)
  {
    return nsIntRect(aRect.left, aRect.top,
                     aRect.right - aRect.left, aRect.bottom - aRect.top);
  }

  static nsIntRegion ConvertHRGNToRegion(HRGN aRgn);

  enum IconSizeType {
    kSmallIcon,
    kRegularIcon
  };
  static gfxIntSize GetIconMetrics(IconSizeType aSizeType);
  static nsresult CreateIcon(imgIContainer *aContainer, bool aIsCursor, PRUint32 aHotspotX, PRUint32 aHotspotY, gfxIntSize aScaledSize, HICON *aIcon);

private:
  


  static PRUint8*         Data32BitTo1Bit(PRUint8* aImageData, PRUint32 aWidth, PRUint32 aHeight);
  static HBITMAP          DataToBitmap(PRUint8* aImageData, PRUint32 aWidth, PRUint32 aHeight, PRUint32 aDepth);
};

#endif 
