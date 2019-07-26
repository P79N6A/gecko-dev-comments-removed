




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
  static nsresult CreateIcon(imgIContainer *aContainer, bool aIsCursor, uint32_t aHotspotX, uint32_t aHotspotY, gfxIntSize aScaledSize, HICON *aIcon);

private:
  


  static uint8_t*         Data32BitTo1Bit(uint8_t* aImageData, uint32_t aWidth, uint32_t aHeight);
  static HBITMAP          DataToBitmap(uint8_t* aImageData, uint32_t aWidth, uint32_t aHeight, uint32_t aDepth);
};

#endif 
