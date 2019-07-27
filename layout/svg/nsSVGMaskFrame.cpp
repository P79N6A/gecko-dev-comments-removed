





#include "nsSVGMaskFrame.h"


#include "gfx2DGlue.h"
#include "gfxContext.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/RefPtr.h"
#include "nsRenderingContext.h"
#include "nsSVGEffects.h"
#include "mozilla/dom/SVGMaskElement.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::gfx;




#ifdef IS_BIG_ENDIAN
#define GFX_ARGB32_OFFSET_A 0
#define GFX_ARGB32_OFFSET_R 1
#define GFX_ARGB32_OFFSET_G 2
#define GFX_ARGB32_OFFSET_B 3
#else
#define GFX_ARGB32_OFFSET_A 3
#define GFX_ARGB32_OFFSET_R 2
#define GFX_ARGB32_OFFSET_G 1
#define GFX_ARGB32_OFFSET_B 0
#endif



static const uint8_t gsRGBToLinearRGBMap[256] = {
  0,   0,   0,   0,   0,   0,   0,   1,
  1,   1,   1,   1,   1,   1,   1,   1,
  1,   1,   2,   2,   2,   2,   2,   2,
  2,   2,   3,   3,   3,   3,   3,   3,
  4,   4,   4,   4,   4,   5,   5,   5,
  5,   6,   6,   6,   6,   7,   7,   7,
  8,   8,   8,   8,   9,   9,   9,  10,
 10,  10,  11,  11,  12,  12,  12,  13,
 13,  13,  14,  14,  15,  15,  16,  16,
 17,  17,  17,  18,  18,  19,  19,  20,
 20,  21,  22,  22,  23,  23,  24,  24,
 25,  25,  26,  27,  27,  28,  29,  29,
 30,  30,  31,  32,  32,  33,  34,  35,
 35,  36,  37,  37,  38,  39,  40,  41,
 41,  42,  43,  44,  45,  45,  46,  47,
 48,  49,  50,  51,  51,  52,  53,  54,
 55,  56,  57,  58,  59,  60,  61,  62,
 63,  64,  65,  66,  67,  68,  69,  70,
 71,  72,  73,  74,  76,  77,  78,  79,
 80,  81,  82,  84,  85,  86,  87,  88,
 90,  91,  92,  93,  95,  96,  97,  99,
100, 101, 103, 104, 105, 107, 108, 109,
111, 112, 114, 115, 116, 118, 119, 121,
122, 124, 125, 127, 128, 130, 131, 133,
134, 136, 138, 139, 141, 142, 144, 146,
147, 149, 151, 152, 154, 156, 157, 159,
161, 163, 164, 166, 168, 170, 171, 173,
175, 177, 179, 181, 183, 184, 186, 188,
190, 192, 194, 196, 198, 200, 202, 204,
206, 208, 210, 212, 214, 216, 218, 220,
222, 224, 226, 229, 231, 233, 235, 237,
239, 242, 244, 246, 248, 250, 253, 255
};

static void
ComputesRGBLuminanceMask(uint8_t *aData,
                         int32_t aStride,
                         const IntSize &aSize,
                         float aOpacity)
{
  for (int32_t y = 0; y < aSize.height; y++) {
    for (int32_t x = 0; x < aSize.width; x++) {
      uint8_t *pixel = aData + aStride * y + 4 * x;
      uint8_t a = pixel[GFX_ARGB32_OFFSET_A];

      uint8_t luminance;
      if (a) {
        

        luminance =
          static_cast<uint8_t>
                     ((pixel[GFX_ARGB32_OFFSET_R] * 0.2125 +
                       pixel[GFX_ARGB32_OFFSET_G] * 0.7154 +
                       pixel[GFX_ARGB32_OFFSET_B] * 0.0721) *
                      aOpacity);
      } else {
        luminance = 0;
      }
      memset(pixel, luminance, 4);
    }
  }
}

static void
ComputeLinearRGBLuminanceMask(uint8_t *aData,
                              int32_t aStride,
                              const IntSize &aSize,
                              float aOpacity)
{
  for (int32_t y = 0; y < aSize.height; y++) {
    for (int32_t x = 0; x < aSize.width; x++) {
      uint8_t *pixel = aData + aStride * y + 4 * x;
      uint8_t a = pixel[GFX_ARGB32_OFFSET_A];

      uint8_t luminance;
      
      if (a) {
        if (a != 255) {
          pixel[GFX_ARGB32_OFFSET_B] =
            (255 * pixel[GFX_ARGB32_OFFSET_B]) / a;
          pixel[GFX_ARGB32_OFFSET_G] =
            (255 * pixel[GFX_ARGB32_OFFSET_G]) / a;
          pixel[GFX_ARGB32_OFFSET_R] =
            (255 * pixel[GFX_ARGB32_OFFSET_R]) / a;
        }

        
        luminance =
          static_cast<uint8_t>
                     ((gsRGBToLinearRGBMap[pixel[GFX_ARGB32_OFFSET_R]] *
                       0.2125 +
                       gsRGBToLinearRGBMap[pixel[GFX_ARGB32_OFFSET_G]] *
                       0.7154 +
                       gsRGBToLinearRGBMap[pixel[GFX_ARGB32_OFFSET_B]] *
                       0.0721) * (a / 255.0) * aOpacity);
      } else {
        luminance = 0;
      }
      memset(pixel, luminance, 4);
    }
  }
}

static void
ComputeAlphaMask(uint8_t *aData,
                 int32_t aStride,
                 const IntSize &aSize,
                 float aOpacity)
{
  for (int32_t y = 0; y < aSize.height; y++) {
    for (int32_t x = 0; x < aSize.width; x++) {
      uint8_t *pixel = aData + aStride * y + 4 * x;
      uint8_t luminance = pixel[GFX_ARGB32_OFFSET_A] * aOpacity;
      memset(pixel, luminance, 4);
    }
  }
}




nsIFrame*
NS_NewSVGMaskFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGMaskFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGMaskFrame)

already_AddRefed<gfxPattern>
nsSVGMaskFrame::GetMaskForMaskedFrame(gfxContext* aContext,
                                      nsIFrame* aMaskedFrame,
                                      const gfxMatrix &aMatrix,
                                      float aOpacity)
{
  
  
  
  if (mInUse) {
    NS_WARNING("Mask loop detected!");
    return nullptr;
  }
  AutoMaskReferencer maskRef(this);

  SVGMaskElement *maskElem = static_cast<SVGMaskElement*>(mContent);

  uint16_t units =
    maskElem->mEnumAttributes[SVGMaskElement::MASKUNITS].GetAnimValue();
  gfxRect bbox;
  if (units == SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
    bbox = nsSVGUtils::GetBBox(aMaskedFrame);
  }

  
  gfxRect maskArea = nsSVGUtils::GetRelativeRect(units,
                       &maskElem->mLengthAttributes[SVGMaskElement::ATTR_X],
                       bbox, aMaskedFrame);

  
  
  
  aContext->Save();
  nsSVGUtils::SetClipRect(aContext, aMatrix, maskArea);
  aContext->IdentityMatrix();
  gfxRect maskSurfaceRect = aContext->GetClipExtents();
  maskSurfaceRect.RoundOut();
  aContext->Restore();

  bool resultOverflows;
  IntSize maskSurfaceSize =
    ToIntSize(nsSVGUtils::ConvertToSurfaceSize(maskSurfaceRect.Size(),
                                               &resultOverflows));

  if (resultOverflows || maskSurfaceSize.IsEmpty()) {
    
    return nullptr;
  }

  RefPtr<DrawTarget> maskDT =
    Factory::CreateDrawTarget(BackendType::CAIRO, maskSurfaceSize,
                              SurfaceFormat::B8G8R8A8);
  if (!maskDT) {
    return nullptr;
  }

  gfxMatrix maskSurfaceMatrix =
    aContext->CurrentMatrix() * gfxMatrix::Translation(-maskSurfaceRect.TopLeft());

  nsRefPtr<nsRenderingContext> tmpCtx = new nsRenderingContext();
  tmpCtx->Init(this->PresContext()->DeviceContext(), maskDT);
  tmpCtx->ThebesContext()->SetMatrix(maskSurfaceMatrix);

  mMatrixForChildren = GetMaskTransform(aMaskedFrame) * aMatrix;

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    
    nsISVGChildFrame* SVGFrame = do_QueryFrame(kid);
    if (SVGFrame) {
      SVGFrame->NotifySVGChanged(nsISVGChildFrame::TRANSFORM_CHANGED);
    }
    gfxMatrix m = mMatrixForChildren;
    if (kid->GetContent()->IsSVG()) {
      m = static_cast<nsSVGElement*>(kid->GetContent())->
            PrependLocalTransformsTo(m);
    }
    nsSVGUtils::PaintFrameWithEffects(kid, tmpCtx, mMatrixForChildren);
  }

  RefPtr<SourceSurface> maskSnapshot = maskDT->Snapshot();
  if (!maskSnapshot) {
    return nullptr;
  }
  RefPtr<DataSourceSurface> maskSurface = maskSnapshot->GetDataSurface();
  DataSourceSurface::MappedSurface map;
  if (!maskSurface->Map(DataSourceSurface::MapType::READ_WRITE, &map)) {
    return nullptr;
  }

  if (StyleSVGReset()->mMaskType == NS_STYLE_MASK_TYPE_LUMINANCE) {
    if (StyleSVG()->mColorInterpolation ==
        NS_STYLE_COLOR_INTERPOLATION_LINEARRGB) {
      ComputeLinearRGBLuminanceMask(map.mData, map.mStride, maskSurfaceSize, aOpacity);
    } else {
      ComputesRGBLuminanceMask(map.mData, map.mStride, maskSurfaceSize, aOpacity);
    }
  } else {
    ComputeAlphaMask(map.mData, map.mStride, maskSurfaceSize, aOpacity);
  }

  maskSurface->Unmap();

  
  if (!maskSurfaceMatrix.Invert()) {
    return nullptr;
  }
  nsRefPtr<gfxPattern> retval =
    new gfxPattern(maskSurface, ToMatrix(maskSurfaceMatrix));
  return retval.forget();
}

nsresult
nsSVGMaskFrame::AttributeChanged(int32_t  aNameSpaceID,
                                 nsIAtom* aAttribute,
                                 int32_t  aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      (aAttribute == nsGkAtoms::x ||
       aAttribute == nsGkAtoms::y ||
       aAttribute == nsGkAtoms::width ||
       aAttribute == nsGkAtoms::height||
       aAttribute == nsGkAtoms::maskUnits ||
       aAttribute == nsGkAtoms::maskContentUnits)) {
    nsSVGEffects::InvalidateDirectRenderingObservers(this);
  }

  return nsSVGMaskFrameBase::AttributeChanged(aNameSpaceID,
                                              aAttribute, aModType);
}

#ifdef DEBUG
void
nsSVGMaskFrame::Init(nsIContent*       aContent,
                     nsContainerFrame* aParent,
                     nsIFrame*         aPrevInFlow)
{
  NS_ASSERTION(aContent->IsSVG(nsGkAtoms::mask),
               "Content is not an SVG mask");

  nsSVGMaskFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

nsIAtom *
nsSVGMaskFrame::GetType() const
{
  return nsGkAtoms::svgMaskFrame;
}

gfxMatrix
nsSVGMaskFrame::GetCanvasTM()
{
  return mMatrixForChildren;
}

gfxMatrix
nsSVGMaskFrame::GetMaskTransform(nsIFrame* aMaskedFrame)
{
  SVGMaskElement *content = static_cast<SVGMaskElement*>(mContent);

  nsSVGEnum* maskContentUnits =
    &content->mEnumAttributes[SVGMaskElement::MASKCONTENTUNITS];

  return nsSVGUtils::AdjustMatrixForUnits(gfxMatrix(), maskContentUnits,
                                          aMaskedFrame);
}
