





#include "ImageMetadata.h"

#include "RasterImage.h"
#include "nsComponentManagerUtils.h"
#include "nsISupportsPrimitives.h"
#include "nsXPCOMCID.h"

namespace mozilla {
namespace image {

void
ImageMetadata::SetOnImage(RasterImage* image)
{
  if (mHotspotX != -1 && mHotspotY != -1) {
    nsCOMPtr<nsISupportsPRUint32> intwrapx =
      do_CreateInstance(NS_SUPPORTS_PRUINT32_CONTRACTID);
    nsCOMPtr<nsISupportsPRUint32> intwrapy =
      do_CreateInstance(NS_SUPPORTS_PRUINT32_CONTRACTID);
    intwrapx->SetData(mHotspotX);
    intwrapy->SetData(mHotspotY);
    image->Set("hotspotX", intwrapx);
    image->Set("hotspotY", intwrapy);
  }

  image->SetLoopCount(mLoopCount);
}

} 
} 
