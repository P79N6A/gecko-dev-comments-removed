



































#ifndef __NS_SVGFILTERINSTANCE_H__
#define __NS_SVGFILTERINSTANCE_H__

#include "nsIDOMSVGLength.h"
#include "nsIDOMSVGRect.h"
#include "nsInterfaceHashtable.h"
#include "nsClassHashtable.h"
#include "nsIDOMSVGFilters.h"
#include "nsRect.h"
#include "nsIContent.h"
#include "nsAutoPtr.h"

#include "gfxImageSurface.h"

class nsSVGLength2;
class nsSVGElement;

class nsSVGFilterInstance
{
public:
  class ColorModel {
  public:
    enum ColorSpace { SRGB, LINEAR_RGB };
    enum AlphaChannel { UNPREMULTIPLIED, PREMULTIPLIED };

    ColorModel(ColorSpace aColorSpace, AlphaChannel aAlphaChannel) :
      mColorSpace(aColorSpace), mAlphaChannel(aAlphaChannel) {}
    PRBool operator==(const ColorModel& aOther) const {
      return mColorSpace == aOther.mColorSpace &&
             mAlphaChannel == aOther.mAlphaChannel;
    }
    ColorSpace   mColorSpace;
    PRPackedBool mAlphaChannel;
  };

  float GetPrimitiveLength(nsSVGLength2 *aLength);

  void GetFilterSubregion(nsIContent *aFilter,
                          nsRect defaultRegion,
                          nsRect *result);

  already_AddRefed<gfxImageSurface> GetImage();
  void LookupImage(const nsAString &aName,
                   gfxImageSurface **aImage,
                   nsRect *aRegion,
                   const ColorModel &aColorModel);
  void DefineImage(const nsAString &aName,
                   gfxImageSurface *aImage,
                   const nsRect &aRegion,
                   const ColorModel &aColorModel);
  void GetFilterBox(float *x, float *y, float *width, float *height) {
    *x = mFilterX;
    *y = mFilterY;
    *width = mFilterWidth;
    *height = mFilterHeight;
  }

  nsSVGFilterInstance(nsSVGElement *aTarget,
                      nsIDOMSVGRect *aTargetBBox,
                      float aFilterX, float aFilterY,
                      float aFilterWidth, float aFilterHeight,
                      PRUint32 aFilterResX, PRUint32 aFilterResY,
                      PRUint16 aPrimitiveUnits) :
    mTarget(aTarget),
    mTargetBBox(aTargetBBox),
    mLastImage(nsnull),
    mFilterX(aFilterX), mFilterY(aFilterY),
    mFilterWidth(aFilterWidth), mFilterHeight(aFilterHeight),
    mFilterResX(aFilterResX), mFilterResY(aFilterResY),
    mPrimitiveUnits(aPrimitiveUnits) {
    mImageDictionary.Init();
  }

private:
  class ImageEntry {
  public:
    ImageEntry(gfxImageSurface *aImage,
               const nsRect &aRegion,
               const ColorModel &aColorModel) :
      mImage(aImage), mRegion(aRegion), mColorModel(aColorModel) {
    }

    nsRefPtr<gfxImageSurface> mImage;
    nsRect mRegion;
    ColorModel mColorModel;
  };

  nsClassHashtable<nsStringHashKey,ImageEntry> mImageDictionary;
  nsRefPtr<nsSVGElement> mTarget;
  nsCOMPtr<nsIDOMSVGRect> mTargetBBox;
  ImageEntry *mLastImage;

  float mFilterX, mFilterY, mFilterWidth, mFilterHeight;
  PRUint32 mFilterResX, mFilterResY;
  PRUint16 mPrimitiveUnits;
};

#endif
