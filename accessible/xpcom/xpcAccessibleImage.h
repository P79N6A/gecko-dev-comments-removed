





#ifndef mozilla_a11y_xpcAccessibleImage_h_
#define mozilla_a11y_xpcAccessibleImage_h_

#include "nsIAccessibleImage.h"

namespace mozilla {
namespace a11y {

class xpcAccessibleImage : public nsIAccessibleImage
{
public:
  NS_IMETHOD GetImagePosition(uint32_t aCoordType,
                              int32_t* aX, int32_t* aY) MOZ_FINAL;
  NS_IMETHOD GetImageSize(int32_t* aWidth, int32_t* aHeight) MOZ_FINAL;

private:
  friend class ImageAccessible;

  xpcAccessibleImage() { }

  xpcAccessibleImage(const xpcAccessibleImage&) MOZ_DELETE;
  xpcAccessibleImage& operator =(const xpcAccessibleImage&) MOZ_DELETE;
};

} 
} 

#endif
