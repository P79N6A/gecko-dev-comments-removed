





#include "ImageMetadata.h"

#include "RasterImage.h"
#include "nsComponentManagerUtils.h"
#include "nsSupportsPrimitives.h"

using namespace mozilla::image;

void
ImageMetadata::SetOnImage(RasterImage* image)
{
  if (mHotspotX != -1 && mHotspotY != -1) {
    nsCOMPtr<nsISupportsPRUint32> intwrapx = do_CreateInstance("@mozilla.org/supports-PRUint32;1");
    nsCOMPtr<nsISupportsPRUint32> intwrapy = do_CreateInstance("@mozilla.org/supports-PRUint32;1");
    intwrapx->SetData(mHotspotX);
    intwrapy->SetData(mHotspotY);
    image->Set("hotspotX", intwrapx);
    image->Set("hotspotY", intwrapy);
  }

  image->SetLoopCount(mLoopCount);

  for (uint32_t i = 0; i < image->GetNumFrames(); i++) {
    image->SetFrameAsNonPremult(i, mIsNonPremultiplied);
  }
}
