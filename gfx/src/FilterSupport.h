




#ifndef __FilterSupport_h
#define __FilterSupport_h

#include "mozilla/Attributes.h"
#include "mozilla/RefPtr.h"
#include "mozilla/gfx/Rect.h"
#include "mozilla/gfx/Matrix.h"
#include "mozilla/gfx/2D.h"
#include "nsClassHashtable.h"
#include "nsTArray.h"
#include "nsRegion.h"

namespace mozilla {
namespace gfx {


const unsigned short SVG_OPERATOR_UNKNOWN = 0;
const unsigned short SVG_OPERATOR_ERODE = 1;
const unsigned short SVG_OPERATOR_DILATE = 2;


const unsigned short SVG_FECOLORMATRIX_TYPE_UNKNOWN = 0;
const unsigned short SVG_FECOLORMATRIX_TYPE_MATRIX = 1;
const unsigned short SVG_FECOLORMATRIX_TYPE_SATURATE = 2;
const unsigned short SVG_FECOLORMATRIX_TYPE_HUE_ROTATE = 3;
const unsigned short SVG_FECOLORMATRIX_TYPE_LUMINANCE_TO_ALPHA = 4;

const unsigned short SVG_FECOLORMATRIX_TYPE_SEPIA = 5;


const unsigned short SVG_FECOMPONENTTRANSFER_TYPE_UNKNOWN  = 0;
const unsigned short SVG_FECOMPONENTTRANSFER_TYPE_IDENTITY = 1;
const unsigned short SVG_FECOMPONENTTRANSFER_TYPE_TABLE    = 2;
const unsigned short SVG_FECOMPONENTTRANSFER_TYPE_DISCRETE = 3;
const unsigned short SVG_FECOMPONENTTRANSFER_TYPE_LINEAR   = 4;
const unsigned short SVG_FECOMPONENTTRANSFER_TYPE_GAMMA    = 5;


const unsigned short SVG_FEBLEND_MODE_UNKNOWN = 0;
const unsigned short SVG_FEBLEND_MODE_NORMAL = 1;
const unsigned short SVG_FEBLEND_MODE_MULTIPLY = 2;
const unsigned short SVG_FEBLEND_MODE_SCREEN = 3;
const unsigned short SVG_FEBLEND_MODE_DARKEN = 4;
const unsigned short SVG_FEBLEND_MODE_LIGHTEN = 5;
const unsigned short SVG_FEBLEND_MODE_OVERLAY = 6;
const unsigned short SVG_FEBLEND_MODE_COLOR_DODGE = 7;
const unsigned short SVG_FEBLEND_MODE_COLOR_BURN = 8;
const unsigned short SVG_FEBLEND_MODE_HARD_LIGHT = 9;
const unsigned short SVG_FEBLEND_MODE_SOFT_LIGHT = 10;
const unsigned short SVG_FEBLEND_MODE_DIFFERENCE = 11;
const unsigned short SVG_FEBLEND_MODE_EXCLUSION = 12;
const unsigned short SVG_FEBLEND_MODE_HUE = 13;
const unsigned short SVG_FEBLEND_MODE_SATURATION = 14;
const unsigned short SVG_FEBLEND_MODE_COLOR = 15;
const unsigned short SVG_FEBLEND_MODE_LUMINOSITY = 16;


const unsigned short SVG_EDGEMODE_UNKNOWN = 0;
const unsigned short SVG_EDGEMODE_DUPLICATE = 1;
const unsigned short SVG_EDGEMODE_WRAP = 2;
const unsigned short SVG_EDGEMODE_NONE = 3;


const unsigned short SVG_CHANNEL_UNKNOWN = 0;
const unsigned short SVG_CHANNEL_R = 1;
const unsigned short SVG_CHANNEL_G = 2;
const unsigned short SVG_CHANNEL_B = 3;
const unsigned short SVG_CHANNEL_A = 4;


const unsigned short SVG_TURBULENCE_TYPE_UNKNOWN = 0;
const unsigned short SVG_TURBULENCE_TYPE_FRACTALNOISE = 1;
const unsigned short SVG_TURBULENCE_TYPE_TURBULENCE = 2;


const unsigned short SVG_FECOMPOSITE_OPERATOR_UNKNOWN = 0;
const unsigned short SVG_FECOMPOSITE_OPERATOR_OVER = 1;
const unsigned short SVG_FECOMPOSITE_OPERATOR_IN = 2;
const unsigned short SVG_FECOMPOSITE_OPERATOR_OUT = 3;
const unsigned short SVG_FECOMPOSITE_OPERATOR_ATOP = 4;
const unsigned short SVG_FECOMPOSITE_OPERATOR_XOR = 5;
const unsigned short SVG_FECOMPOSITE_OPERATOR_ARITHMETIC = 6;

enum AttributeName {
  eBlendBlendmode = 0,
  eMorphologyRadii,
  eMorphologyOperator,
  eColorMatrixType,
  eColorMatrixValues,
  eFloodColor,
  eTileSourceRect,
  eComponentTransferFunctionR,
  eComponentTransferFunctionG,
  eComponentTransferFunctionB,
  eComponentTransferFunctionA,
  eComponentTransferFunctionType,
  eComponentTransferFunctionTableValues,
  eComponentTransferFunctionSlope,
  eComponentTransferFunctionIntercept,
  eComponentTransferFunctionAmplitude,
  eComponentTransferFunctionExponent,
  eComponentTransferFunctionOffset,
  eConvolveMatrixKernelSize,
  eConvolveMatrixKernelMatrix,
  eConvolveMatrixDivisor,
  eConvolveMatrixBias,
  eConvolveMatrixTarget,
  eConvolveMatrixEdgeMode,
  eConvolveMatrixKernelUnitLength,
  eConvolveMatrixPreserveAlpha,
  eOffsetOffset,
  eDropShadowStdDeviation,
  eDropShadowOffset,
  eDropShadowColor,
  eDisplacementMapScale,
  eDisplacementMapXChannel,
  eDisplacementMapYChannel,
  eTurbulenceOffset,
  eTurbulenceBaseFrequency,
  eTurbulenceNumOctaves,
  eTurbulenceSeed,
  eTurbulenceStitchable,
  eTurbulenceType,
  eCompositeOperator,
  eCompositeCoefficients,
  eGaussianBlurStdDeviation,
  eLightingLight,
  eLightingSurfaceScale,
  eLightingKernelUnitLength,
  eLightingColor,
  eDiffuseLightingDiffuseConstant,
  eSpecularLightingSpecularConstant,
  eSpecularLightingSpecularExponent,
  eLightType,
  eLightTypeNone,
  eLightTypePoint,
  eLightTypeSpot,
  eLightTypeDistant,
  ePointLightPosition,
  eSpotLightPosition,
  eSpotLightPointsAt,
  eSpotLightFocus,
  eSpotLightLimitingConeAngle,
  eDistantLightAzimuth,
  eDistantLightElevation,
  eImageInputIndex,
  eImageFilter,
  eImageNativeSize,
  eImageSubregion,
  eImageTransform,
  eLastAttributeName
};

class DrawTarget;
class SourceSurface;
struct FilterAttribute;

enum class AttributeType {
  eBool,
  eUint,
  eFloat,
  eSize,
  eIntSize,
  eIntPoint,
  eMatrix,
  eMatrix5x4,
  ePoint3D,
  eColor,
  eAttributeMap,
  eFloats,
  Max
};


const float kMaxStdDeviation = 500;





class AttributeMap final {
public:
  AttributeMap();
  AttributeMap(const AttributeMap& aOther);
  AttributeMap& operator=(const AttributeMap& aOther);
  bool operator==(const AttributeMap& aOther) const;
  bool operator!=(const AttributeMap& aOther) const
  {
    return !(*this == aOther);
  }
  ~AttributeMap();

  void Set(AttributeName aName, bool aValue);
  void Set(AttributeName aName, uint32_t aValue);
  void Set(AttributeName aName, float aValue);
  void Set(AttributeName aName, const Size& aValue);
  void Set(AttributeName aName, const IntSize& aValue);
  void Set(AttributeName aName, const IntPoint& aValue);
  void Set(AttributeName aName, const Matrix& aValue);
  void Set(AttributeName aName, const Matrix5x4& aValue);
  void Set(AttributeName aName, const Point3D& aValue);
  void Set(AttributeName aName, const Color& aValue);
  void Set(AttributeName aName, const AttributeMap& aValue);
  void Set(AttributeName aName, const float* aValues, int32_t aLength);

  bool GetBool(AttributeName aName) const;
  uint32_t GetUint(AttributeName aName) const;
  float GetFloat(AttributeName aName) const;
  Size GetSize(AttributeName aName) const;
  IntSize GetIntSize(AttributeName aName) const;
  IntPoint GetIntPoint(AttributeName aName) const;
  Matrix GetMatrix(AttributeName aName) const;
  Matrix5x4 GetMatrix5x4(AttributeName aName) const;
  Point3D GetPoint3D(AttributeName aName) const;
  Color GetColor(AttributeName aName) const;
  AttributeMap GetAttributeMap(AttributeName aName) const;
  const nsTArray<float>& GetFloats(AttributeName aName) const;

  typedef bool (*AttributeHandleCallback)(AttributeName aName, AttributeType aType, void* aUserData);
  void EnumerateRead(AttributeHandleCallback aCallback, void* aUserData) const;
  uint32_t Count() const;

private:
  mutable nsClassHashtable<nsUint32HashKey, FilterAttribute>  mMap;
};

enum class ColorSpace {
  SRGB,
  LinearRGB,
  Max
};

enum class AlphaModel {
  Unpremultiplied,
  Premultiplied
};

class ColorModel {
public:
  static ColorModel PremulSRGB()
  {
    return ColorModel(ColorSpace::SRGB, AlphaModel::Premultiplied);
  }

  ColorModel(ColorSpace aColorSpace, AlphaModel aAlphaModel) :
    mColorSpace(aColorSpace), mAlphaModel(aAlphaModel) {}
  ColorModel() :
    mColorSpace(ColorSpace::SRGB), mAlphaModel(AlphaModel::Premultiplied) {}
  bool operator==(const ColorModel& aOther) const {
    return mColorSpace == aOther.mColorSpace &&
           mAlphaModel == aOther.mAlphaModel;
  }

  
  uint8_t ToIndex() const
  {
    return (uint8_t(mColorSpace) << 1) + uint8_t(mAlphaModel);
  }

  ColorSpace mColorSpace;
  AlphaModel mAlphaModel;
};

enum class PrimitiveType {
  Empty = 0,
  Blend,
  Morphology,
  ColorMatrix,
  Flood,
  Tile,
  ComponentTransfer,
  ConvolveMatrix,
  Offset,
  DisplacementMap,
  Turbulence,
  Composite,
  Merge,
  Image,
  GaussianBlur,
  DropShadow,
  DiffuseLighting,
  SpecularLighting,
  ToAlpha,
  Max
};







class FilterPrimitiveDescription final {
public:
  enum {
    kPrimitiveIndexSourceGraphic = -1,
    kPrimitiveIndexSourceAlpha = -2,
    kPrimitiveIndexFillPaint = -3,
    kPrimitiveIndexStrokePaint = -4
  };

  FilterPrimitiveDescription();
  explicit FilterPrimitiveDescription(PrimitiveType aType);
  FilterPrimitiveDescription(const FilterPrimitiveDescription& aOther);
  FilterPrimitiveDescription& operator=(const FilterPrimitiveDescription& aOther);

  PrimitiveType Type() const { return mType; }
  void SetType(PrimitiveType aType) { mType = aType; }
  const AttributeMap& Attributes() const { return mAttributes; }
  AttributeMap& Attributes() { return mAttributes; }

  IntRect PrimitiveSubregion() const { return mFilterPrimitiveSubregion; }
  IntRect FilterSpaceBounds() const { return mFilterSpaceBounds; }
  bool IsTainted() const { return mIsTainted; }

  size_t NumberOfInputs() const { return mInputPrimitives.Length(); }
  int32_t InputPrimitiveIndex(size_t aInputIndex) const
  {
    return aInputIndex < mInputPrimitives.Length() ?
      mInputPrimitives[aInputIndex] : 0;
  }

  ColorSpace InputColorSpace(size_t aInputIndex) const
  {
    return aInputIndex < mInputColorSpaces.Length() ?
      mInputColorSpaces[aInputIndex] : ColorSpace();
  }

  ColorSpace OutputColorSpace() const { return mOutputColorSpace; }

  void SetPrimitiveSubregion(const IntRect& aRect)
  {
    mFilterPrimitiveSubregion = aRect;
  }

  void SetFilterSpaceBounds(const IntRect& aRect)
  {
    mFilterSpaceBounds = aRect;
  }

  void SetIsTainted(bool aIsTainted)
  {
    mIsTainted = aIsTainted;
  }

  void SetInputPrimitive(size_t aInputIndex, int32_t aInputPrimitiveIndex)
  {
    mInputPrimitives.EnsureLengthAtLeast(aInputIndex + 1);
    mInputPrimitives[aInputIndex] = aInputPrimitiveIndex;
  }

  void SetInputColorSpace(size_t aInputIndex, ColorSpace aColorSpace)
  {
    mInputColorSpaces.EnsureLengthAtLeast(aInputIndex + 1);
    mInputColorSpaces[aInputIndex] = aColorSpace;
  }

  void SetOutputColorSpace(const ColorSpace& aColorSpace)
  {
    mOutputColorSpace = aColorSpace;
  }

  bool operator==(const FilterPrimitiveDescription& aOther) const;
  bool operator!=(const FilterPrimitiveDescription& aOther) const
  {
    return !(*this == aOther);
  }

private:
  PrimitiveType mType;
  AttributeMap mAttributes;
  nsTArray<int32_t> mInputPrimitives;
  IntRect mFilterPrimitiveSubregion;
  IntRect mFilterSpaceBounds;
  nsTArray<ColorSpace> mInputColorSpaces;
  ColorSpace mOutputColorSpace;
  bool mIsTainted;
};






struct FilterDescription final {
  FilterDescription() {}
  explicit FilterDescription(const nsTArray<FilterPrimitiveDescription>& aPrimitives)
   : mPrimitives(aPrimitives)
  {}

  bool operator==(const FilterDescription& aOther) const;
  bool operator!=(const FilterDescription& aOther) const
  {
    return !(*this == aOther);
  }

  nsTArray<FilterPrimitiveDescription> mPrimitives;
};






class FilterSupport {
public:

  








  static void
  RenderFilterDescription(DrawTarget* aDT,
                          const FilterDescription& aFilter,
                          const Rect& aRenderRect,
                          SourceSurface* aSourceGraphic,
                          const IntRect& aSourceGraphicRect,
                          SourceSurface* aFillPaint,
                          const IntRect& aFillPaintRect,
                          SourceSurface* aStrokePaint,
                          const IntRect& aStrokePaintRect,
                          nsTArray<RefPtr<SourceSurface>>& aAdditionalImages,
                          const Point& aDestPoint,
                          const DrawOptions& aOptions = DrawOptions());

  



  static nsIntRegion
  ComputeResultChangeRegion(const FilterDescription& aFilter,
                            const nsIntRegion& aSourceGraphicChange,
                            const nsIntRegion& aFillPaintChange,
                            const nsIntRegion& aStrokePaintChange);

  



  static void
  ComputeSourceNeededRegions(const FilterDescription& aFilter,
                             const nsIntRegion& aResultNeededRegion,
                             nsIntRegion& aSourceGraphicNeededRegion,
                             nsIntRegion& aFillPaintNeededRegion,
                             nsIntRegion& aStrokePaintNeededRegion);

  


  static nsIntRegion
  ComputePostFilterExtents(const FilterDescription& aFilter,
                           const nsIntRegion& aSourceGraphicExtents);

  



  static nsIntRegion
  PostFilterExtentsForPrimitive(const FilterPrimitiveDescription& aDescription,
                                const nsTArray<nsIntRegion>& aInputExtents);
};

}
}

#endif
