




#ifndef _MOZILLA_GFX_FILTERNODESOFTWARE_H_
#define _MOZILLA_GFX_FILTERNODESOFTWARE_H_

#include "Filters.h"
#include <vector>

namespace mozilla {
namespace gfx {

class DataSourceSurface;
class DrawTarget;
struct DrawOptions;
class FilterNodeSoftware;







class FilterInvalidationListener
{
public:
  virtual void FilterInvalidated(FilterNodeSoftware* aFilter) = 0;
};






class FilterNodeSoftware : public FilterNode,
                           public FilterInvalidationListener
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(FilterNodeSoftware, override)
  virtual ~FilterNodeSoftware();

  
  static already_AddRefed<FilterNode> Create(FilterType aType);

  
  void Draw(DrawTarget* aDrawTarget, const Rect &aSourceRect,
            const Point &aDestPoint, const DrawOptions &aOptions);

  virtual FilterBackend GetBackendType() override { return FILTER_BACKEND_SOFTWARE; }
  virtual void SetInput(uint32_t aIndex, SourceSurface *aSurface) override;
  virtual void SetInput(uint32_t aIndex, FilterNode *aFilter) override;

  virtual const char* GetName() { return "Unknown"; }

  virtual void AddInvalidationListener(FilterInvalidationListener* aListener);
  virtual void RemoveInvalidationListener(FilterInvalidationListener* aListener);

  
  virtual void FilterInvalidated(FilterNodeSoftware* aFilter) override;

protected:

  

  





  virtual int32_t InputIndex(uint32_t aInputEnumIndex) { return -1; }

  






  virtual IntRect GetOutputRectInRect(const IntRect& aInRect) = 0;

  








  virtual already_AddRefed<DataSourceSurface> Render(const IntRect& aRect) = 0;

  




  virtual void RequestFromInputsForRect(const IntRect &aRect) {}

  




  virtual already_AddRefed<DataSourceSurface> GetOutput(const IntRect &aRect);

  

  





  enum FormatHint {
    CAN_HANDLE_A8,
    NEED_COLOR_CHANNELS
  };

  



  SurfaceFormat DesiredFormat(SurfaceFormat aCurrentFormat,
                              FormatHint aFormatHint);

  












  already_AddRefed<DataSourceSurface>
    GetInputDataSourceSurface(uint32_t aInputEnumIndex, const IntRect& aRect,
                              FormatHint aFormatHint = CAN_HANDLE_A8,
                              ConvolveMatrixEdgeMode aEdgeMode = EDGE_MODE_NONE,
                              const IntRect *aTransparencyPaddedSourceRect = nullptr);

  



  IntRect GetInputRectInRect(uint32_t aInputEnumIndex, const IntRect& aInRect);

  


  void RequestInputRect(uint32_t aInputEnumIndex, const IntRect& aRect);

  



  size_t NumberOfSetInputs();

  




  void Invalidate();

  




  void RequestRect(const IntRect &aRect);

  




  void SetInput(uint32_t aIndex, SourceSurface *aSurface,
                FilterNodeSoftware *aFilter);

protected:
  



  std::vector<RefPtr<SourceSurface> > mInputSurfaces;
  std::vector<RefPtr<FilterNodeSoftware> > mInputFilters;

  





  std::vector<FilterInvalidationListener*> mInvalidationListeners;

  



  IntRect mRequestedRect;

  


  IntRect mCachedRect;
  RefPtr<DataSourceSurface> mCachedOutput;
};



class FilterNodeTransformSoftware : public FilterNodeSoftware
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(FilterNodeTransformSoftware, override)
  FilterNodeTransformSoftware();
  virtual const char* GetName() override { return "Transform"; }
  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, uint32_t aGraphicsFilter) override;
  virtual void SetAttribute(uint32_t aIndex, const Matrix &aMatrix) override;

protected:
  virtual already_AddRefed<DataSourceSurface> Render(const IntRect& aRect) override;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) override;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) override;
  virtual void RequestFromInputsForRect(const IntRect &aRect) override;
  IntRect SourceRectForOutputRect(const IntRect &aRect);

private:
  Matrix mMatrix;
  Filter mFilter;
};

class FilterNodeBlendSoftware : public FilterNodeSoftware
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(FilterNodeBlendSoftware, override)
  FilterNodeBlendSoftware();
  virtual const char* GetName() override { return "Blend"; }
  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, uint32_t aBlendMode) override;

protected:
  virtual already_AddRefed<DataSourceSurface> Render(const IntRect& aRect) override;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) override;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) override;
  virtual void RequestFromInputsForRect(const IntRect &aRect) override;

private:
  BlendMode mBlendMode;
};

class FilterNodeMorphologySoftware : public FilterNodeSoftware
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(FilterNodeMorphologySoftware, override)
  FilterNodeMorphologySoftware();
  virtual const char* GetName() override { return "Morphology"; }
  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, const IntSize &aRadii) override;
  virtual void SetAttribute(uint32_t aIndex, uint32_t aOperator) override;

protected:
  virtual already_AddRefed<DataSourceSurface> Render(const IntRect& aRect) override;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) override;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) override;
  virtual void RequestFromInputsForRect(const IntRect &aRect) override;

private:
  IntSize mRadii;
  MorphologyOperator mOperator;
};

class FilterNodeColorMatrixSoftware : public FilterNodeSoftware
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(FilterNodeColorMatrixSoftware, override)
  virtual const char* GetName() override { return "ColorMatrix"; }
  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, const Matrix5x4 &aMatrix) override;
  virtual void SetAttribute(uint32_t aIndex, uint32_t aAlphaMode) override;

protected:
  virtual already_AddRefed<DataSourceSurface> Render(const IntRect& aRect) override;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) override;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) override;
  virtual void RequestFromInputsForRect(const IntRect &aRect) override;

private:
  Matrix5x4 mMatrix;
  AlphaMode mAlphaMode;
};

class FilterNodeFloodSoftware : public FilterNodeSoftware
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(FilterNodeFloodSoftware, override)
  virtual const char* GetName() override { return "Flood"; }
  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, const Color &aColor) override;

protected:
  virtual already_AddRefed<DataSourceSurface> GetOutput(const IntRect &aRect) override;
  virtual already_AddRefed<DataSourceSurface> Render(const IntRect& aRect) override;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) override;

private:
  Color mColor;
};

class FilterNodeTileSoftware : public FilterNodeSoftware
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(FilterNodeTileSoftware, override)
  virtual const char* GetName() override { return "Tile"; }
  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, const IntRect &aSourceRect) override;

protected:
  virtual already_AddRefed<DataSourceSurface> Render(const IntRect& aRect) override;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) override;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) override;
  virtual void RequestFromInputsForRect(const IntRect &aRect) override;

private:
  IntRect mSourceRect;
};




class FilterNodeComponentTransferSoftware : public FilterNodeSoftware
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(FilterNodeComponentTransferSoftware, override)
  FilterNodeComponentTransferSoftware();

  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, bool aDisable) override;

protected:
  virtual already_AddRefed<DataSourceSurface> Render(const IntRect& aRect) override;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) override;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) override;
  virtual void RequestFromInputsForRect(const IntRect &aRect) override;
  virtual void GenerateLookupTable(ptrdiff_t aComponent, uint8_t aTables[4][256],
                                   bool aDisabled);
  virtual void FillLookupTable(ptrdiff_t aComponent, uint8_t aTable[256]) = 0;

  bool mDisableR;
  bool mDisableG;
  bool mDisableB;
  bool mDisableA;
};

class FilterNodeTableTransferSoftware : public FilterNodeComponentTransferSoftware
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(FilterNodeTableTransferSoftware, override)
  virtual const char* GetName() override { return "TableTransfer"; }
  using FilterNodeComponentTransferSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, const Float* aFloat, uint32_t aSize) override;

protected:
  virtual void FillLookupTable(ptrdiff_t aComponent, uint8_t aTable[256]) override;

private:
  void FillLookupTableImpl(std::vector<Float>& aTableValues, uint8_t aTable[256]);

  std::vector<Float> mTableR;
  std::vector<Float> mTableG;
  std::vector<Float> mTableB;
  std::vector<Float> mTableA;
};

class FilterNodeDiscreteTransferSoftware : public FilterNodeComponentTransferSoftware
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(FilterNodeDiscreteTransferSoftware, override)
  virtual const char* GetName() override { return "DiscreteTransfer"; }
  using FilterNodeComponentTransferSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, const Float* aFloat, uint32_t aSize) override;

protected:
  virtual void FillLookupTable(ptrdiff_t aComponent, uint8_t aTable[256]) override;

private:
  void FillLookupTableImpl(std::vector<Float>& aTableValues, uint8_t aTable[256]);

  std::vector<Float> mTableR;
  std::vector<Float> mTableG;
  std::vector<Float> mTableB;
  std::vector<Float> mTableA;
};

class FilterNodeLinearTransferSoftware : public FilterNodeComponentTransferSoftware
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(FilterNodeLinearTransformSoftware, override)
  FilterNodeLinearTransferSoftware();
  virtual const char* GetName() override { return "LinearTransfer"; }
  using FilterNodeComponentTransferSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, Float aValue) override;

protected:
  virtual void FillLookupTable(ptrdiff_t aComponent, uint8_t aTable[256]) override;

private:
  void FillLookupTableImpl(Float aSlope, Float aIntercept, uint8_t aTable[256]);

  Float mSlopeR;
  Float mSlopeG;
  Float mSlopeB;
  Float mSlopeA;
  Float mInterceptR;
  Float mInterceptG;
  Float mInterceptB;
  Float mInterceptA;
};

class FilterNodeGammaTransferSoftware : public FilterNodeComponentTransferSoftware
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(FilterNodeGammaTransferSoftware, override)
  FilterNodeGammaTransferSoftware();
  virtual const char* GetName() override { return "GammaTransfer"; }
  using FilterNodeComponentTransferSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, Float aValue) override;

protected:
  virtual void FillLookupTable(ptrdiff_t aComponent, uint8_t aTable[256]) override;

private:
  void FillLookupTableImpl(Float aAmplitude, Float aExponent, Float aOffset, uint8_t aTable[256]);

  Float mAmplitudeR;
  Float mAmplitudeG;
  Float mAmplitudeB;
  Float mAmplitudeA;
  Float mExponentR;
  Float mExponentG;
  Float mExponentB;
  Float mExponentA;
  Float mOffsetR;
  Float mOffsetG;
  Float mOffsetB;
  Float mOffsetA;
};

class FilterNodeConvolveMatrixSoftware : public FilterNodeSoftware
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(FilterNodeConvolveMatrixSoftware, override)
  FilterNodeConvolveMatrixSoftware();
  virtual const char* GetName() override { return "ConvolveMatrix"; }
  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, const IntSize &aKernelSize) override;
  virtual void SetAttribute(uint32_t aIndex, const Float* aMatrix, uint32_t aSize) override;
  virtual void SetAttribute(uint32_t aIndex, Float aValue) override;
  virtual void SetAttribute(uint32_t aIndex, const Size &aKernelUnitLength) override;
  virtual void SetAttribute(uint32_t aIndex, const IntRect &aSourceRect) override;
  virtual void SetAttribute(uint32_t aIndex, const IntPoint &aTarget) override;
  virtual void SetAttribute(uint32_t aIndex, uint32_t aEdgeMode) override;
  virtual void SetAttribute(uint32_t aIndex, bool aPreserveAlpha) override;

protected:
  virtual already_AddRefed<DataSourceSurface> Render(const IntRect& aRect) override;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) override;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) override;
  virtual void RequestFromInputsForRect(const IntRect &aRect) override;

private:
  template<typename CoordType>
  already_AddRefed<DataSourceSurface> DoRender(const IntRect& aRect,
                                           CoordType aKernelUnitLengthX,
                                           CoordType aKernelUnitLengthY);

  IntRect InflatedSourceRect(const IntRect &aDestRect);
  IntRect InflatedDestRect(const IntRect &aSourceRect);

  IntSize mKernelSize;
  std::vector<Float> mKernelMatrix;
  Float mDivisor;
  Float mBias;
  IntPoint mTarget;
  IntRect mSourceRect;
  ConvolveMatrixEdgeMode mEdgeMode;
  Size mKernelUnitLength;
  bool mPreserveAlpha;
};

class FilterNodeDisplacementMapSoftware : public FilterNodeSoftware
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(FilterNodeDisplacementMapSoftware, override)
  FilterNodeDisplacementMapSoftware();
  virtual const char* GetName() override { return "DisplacementMap"; }
  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, Float aScale) override;
  virtual void SetAttribute(uint32_t aIndex, uint32_t aValue) override;

protected:
  virtual already_AddRefed<DataSourceSurface> Render(const IntRect& aRect) override;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) override;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) override;
  virtual void RequestFromInputsForRect(const IntRect &aRect) override;

private:
  IntRect InflatedSourceOrDestRect(const IntRect &aDestOrSourceRect);

  Float mScale;
  ColorChannel mChannelX;
  ColorChannel mChannelY;
};

class FilterNodeTurbulenceSoftware : public FilterNodeSoftware
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(FilterNodeTurbulenceSoftware, override)
  FilterNodeTurbulenceSoftware();
  virtual const char* GetName() override { return "Turbulence"; }
  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, const Size &aSize) override;
  virtual void SetAttribute(uint32_t aIndex, const IntRect &aRenderRect) override;
  virtual void SetAttribute(uint32_t aIndex, bool aStitchable) override;
  virtual void SetAttribute(uint32_t aIndex, uint32_t aValue) override;

protected:
  virtual already_AddRefed<DataSourceSurface> Render(const IntRect& aRect) override;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) override;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) override;

private:
  IntRect mRenderRect;
  Size mBaseFrequency;
  uint32_t mNumOctaves;
  uint32_t mSeed;
  bool mStitchable;
  TurbulenceType mType;
};

class FilterNodeArithmeticCombineSoftware : public FilterNodeSoftware
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(FilterNodeArithmeticCombineSoftware, override)
  FilterNodeArithmeticCombineSoftware();
  virtual const char* GetName() override { return "ArithmeticCombine"; }
  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, const Float* aFloat, uint32_t aSize) override;

protected:
  virtual already_AddRefed<DataSourceSurface> Render(const IntRect& aRect) override;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) override;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) override;
  virtual void RequestFromInputsForRect(const IntRect &aRect) override;

private:
  Float mK1;
  Float mK2;
  Float mK3;
  Float mK4;
};

class FilterNodeCompositeSoftware : public FilterNodeSoftware
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(FilterNodeCompositeSoftware, override)
  FilterNodeCompositeSoftware();
  virtual const char* GetName() override { return "Composite"; }
  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, uint32_t aOperator) override;

protected:
  virtual already_AddRefed<DataSourceSurface> Render(const IntRect& aRect) override;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) override;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) override;
  virtual void RequestFromInputsForRect(const IntRect &aRect) override;

private:
  CompositeOperator mOperator;
};



class FilterNodeBlurXYSoftware : public FilterNodeSoftware
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(FilterNodeBlurXYSoftware, override)
protected:
  virtual already_AddRefed<DataSourceSurface> Render(const IntRect& aRect) override;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) override;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) override;
  IntRect InflatedSourceOrDestRect(const IntRect &aDestRect);
  virtual void RequestFromInputsForRect(const IntRect &aRect) override;

  
  virtual Size StdDeviationXY() = 0;
};

class FilterNodeGaussianBlurSoftware : public FilterNodeBlurXYSoftware
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(FilterNodeGaussianBlurSoftware, override)
  FilterNodeGaussianBlurSoftware();
  virtual const char* GetName() override { return "GaussianBlur"; }
  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, Float aStdDeviation) override;

protected:
  virtual Size StdDeviationXY() override;

private:
  Float mStdDeviation;
};

class FilterNodeDirectionalBlurSoftware : public FilterNodeBlurXYSoftware
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(FilterNodeDirectionalBlurSoftware, override)
  FilterNodeDirectionalBlurSoftware();
  virtual const char* GetName() override { return "DirectionalBlur"; }
  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, Float aStdDeviation) override;
  virtual void SetAttribute(uint32_t aIndex, uint32_t aBlurDirection) override;

protected:
  virtual Size StdDeviationXY() override;

private:
  Float mStdDeviation;
  BlurDirection mBlurDirection;
};

class FilterNodeCropSoftware : public FilterNodeSoftware
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(FilterNodeCropSoftware, override)
  virtual const char* GetName() override { return "Crop"; }
  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, const Rect &aSourceRect) override;

protected:
  virtual already_AddRefed<DataSourceSurface> Render(const IntRect& aRect) override;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) override;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) override;
  virtual void RequestFromInputsForRect(const IntRect &aRect) override;

private:
  IntRect mCropRect;
};

class FilterNodePremultiplySoftware : public FilterNodeSoftware
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(FilterNodePremultiplySoftware, override)
  virtual const char* GetName() override { return "Premultiply"; }
protected:
  virtual already_AddRefed<DataSourceSurface> Render(const IntRect& aRect) override;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) override;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) override;
  virtual void RequestFromInputsForRect(const IntRect &aRect) override;
};

class FilterNodeUnpremultiplySoftware : public FilterNodeSoftware
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(FilterNodeUnpremultiplySoftware, override)
  virtual const char* GetName() override { return "Unpremultiply"; }
protected:
  virtual already_AddRefed<DataSourceSurface> Render(const IntRect& aRect) override;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) override;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) override;
  virtual void RequestFromInputsForRect(const IntRect &aRect) override;
};

template<typename LightType, typename LightingType>
class FilterNodeLightingSoftware : public FilterNodeSoftware
{
public:
#if defined(MOZILLA_INTERNAL_API) && (defined(DEBUG) || defined(FORCE_BUILD_REFCNT_LOGGING))
  
  virtual const char* typeName() const override { return mTypeName; }
  virtual size_t typeSize() const override { return sizeof(*this); }
#endif
  explicit FilterNodeLightingSoftware(const char* aTypeName);
  virtual const char* GetName() override { return "Lighting"; }
  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, Float) override;
  virtual void SetAttribute(uint32_t aIndex, const Size &) override;
  virtual void SetAttribute(uint32_t aIndex, const Point3D &) override;
  virtual void SetAttribute(uint32_t aIndex, const Color &) override;

protected:
  virtual already_AddRefed<DataSourceSurface> Render(const IntRect& aRect) override;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) override;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) override;
  virtual void RequestFromInputsForRect(const IntRect &aRect) override;

private:
  template<typename CoordType>
  already_AddRefed<DataSourceSurface> DoRender(const IntRect& aRect,
                                           CoordType aKernelUnitLengthX,
                                           CoordType aKernelUnitLengthY);

  LightType mLight;
  LightingType mLighting;
  Float mSurfaceScale;
  Size mKernelUnitLength;
  Color mColor;
#if defined(MOZILLA_INTERNAL_API) && (defined(DEBUG) || defined(FORCE_BUILD_REFCNT_LOGGING))
  const char* mTypeName;
#endif
};

}
}

#endif 
