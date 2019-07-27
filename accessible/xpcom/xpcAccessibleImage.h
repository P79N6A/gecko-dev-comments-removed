





#ifndef mozilla_a11y_xpcAccessibleImage_h_
#define mozilla_a11y_xpcAccessibleImage_h_

#include "nsIAccessibleImage.h"

#include "xpcAccessibleGeneric.h"

namespace mozilla {
namespace a11y {

class xpcAccessibleImage : public xpcAccessibleGeneric,
                           public nsIAccessibleImage
{
public:
  explicit xpcAccessibleImage(Accessible* aIntl) :
    xpcAccessibleGeneric(aIntl) { }

  NS_DECL_ISUPPORTS_INHERITED

  NS_IMETHOD GetImagePosition(uint32_t aCoordType,
                              int32_t* aX, int32_t* aY) final override;
  NS_IMETHOD GetImageSize(int32_t* aWidth, int32_t* aHeight) final override;

protected:
  virtual ~xpcAccessibleImage() {}

private:
  ImageAccessible* Intl() { return mIntl->AsImage(); }

  xpcAccessibleImage(const xpcAccessibleImage&) = delete;
  xpcAccessibleImage& operator =(const xpcAccessibleImage&) = delete;
};

} 
} 

#endif
