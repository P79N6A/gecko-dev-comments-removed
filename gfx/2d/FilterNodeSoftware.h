




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
  virtual ~FilterNodeSoftware();

  
  static TemporaryRef<FilterNode> Create(FilterType aType);

  
  void Draw(DrawTarget* aDrawTarget, const Rect &aSourceRect,
            const Point &aDestPoint, const DrawOptions &aOptions);

  virtual FilterBackend GetBackendType() MOZ_OVERRIDE { return FILTER_BACKEND_SOFTWARE; }
  virtual void SetInput(uint32_t aIndex, SourceSurface *aSurface) MOZ_OVERRIDE;
  virtual void SetInput(uint32_t aIndex, FilterNode *aFilter) MOZ_OVERRIDE;

  virtual void AddInvalidationListener(FilterInvalidationListener* aListener);
  virtual void RemoveInvalidationListener(FilterInvalidationListener* aListener);

  
  virtual void FilterInvalidated(FilterNodeSoftware* aFilter);

protected:

  

  





  virtual int32_t InputIndex(uint32_t aInputEnumIndex) { return -1; }

  






  virtual IntRect GetOutputRectInRect(const IntRect& aInRect) = 0;

  








  virtual TemporaryRef<DataSourceSurface> Render(const IntRect& aRect) = 0;

  




  virtual void RequestFromInputsForRect(const IntRect &aRect) {}

  




  virtual TemporaryRef<DataSourceSurface> GetOutput(const IntRect &aRect);

  

  





  enum FormatHint {
    CAN_HANDLE_A8,
    NEED_COLOR_CHANNELS
  };

  



  SurfaceFormat DesiredFormat(SurfaceFormat aCurrentFormat,
                              FormatHint aFormatHint);

  












  TemporaryRef<DataSourceSurface>
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
  FilterNodeTransformSoftware();
  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, uint32_t aGraphicsFilter) MOZ_OVERRIDE;
  virtual void SetAttribute(uint32_t aIndex, const Matrix &aMatrix) MOZ_OVERRIDE;

protected:
  virtual TemporaryRef<DataSourceSurface> Render(const IntRect& aRect) MOZ_OVERRIDE;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) MOZ_OVERRIDE;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) MOZ_OVERRIDE;
  virtual void RequestFromInputsForRect(const IntRect &aRect) MOZ_OVERRIDE;
  IntRect SourceRectForOutputRect(const IntRect &aRect);

private:
  Matrix mMatrix;
  Filter mFilter;
};

class FilterNodeBlendSoftware : public FilterNodeSoftware
{
public:
  FilterNodeBlendSoftware();
  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, uint32_t aBlendMode) MOZ_OVERRIDE;

protected:
  virtual TemporaryRef<DataSourceSurface> Render(const IntRect& aRect) MOZ_OVERRIDE;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) MOZ_OVERRIDE;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) MOZ_OVERRIDE;
  virtual void RequestFromInputsForRect(const IntRect &aRect) MOZ_OVERRIDE;

private:
  BlendMode mBlendMode;
};

class FilterNodeMorphologySoftware : public FilterNodeSoftware
{
public:
  FilterNodeMorphologySoftware();
  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, const IntSize &aRadii) MOZ_OVERRIDE;
  virtual void SetAttribute(uint32_t aIndex, uint32_t aOperator) MOZ_OVERRIDE;

protected:
  virtual TemporaryRef<DataSourceSurface> Render(const IntRect& aRect) MOZ_OVERRIDE;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) MOZ_OVERRIDE;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) MOZ_OVERRIDE;
  virtual void RequestFromInputsForRect(const IntRect &aRect) MOZ_OVERRIDE;

private:
  IntSize mRadii;
  MorphologyOperator mOperator;
};

class FilterNodeColorMatrixSoftware : public FilterNodeSoftware
{
public:
  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, const Matrix5x4 &aMatrix) MOZ_OVERRIDE;
  virtual void SetAttribute(uint32_t aIndex, uint32_t aAlphaMode) MOZ_OVERRIDE;

protected:
  virtual TemporaryRef<DataSourceSurface> Render(const IntRect& aRect) MOZ_OVERRIDE;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) MOZ_OVERRIDE;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) MOZ_OVERRIDE;
  virtual void RequestFromInputsForRect(const IntRect &aRect) MOZ_OVERRIDE;

private:
  Matrix5x4 mMatrix;
  AlphaMode mAlphaMode;
};

class FilterNodeFloodSoftware : public FilterNodeSoftware
{
public:
  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, const Color &aColor) MOZ_OVERRIDE;

protected:
  virtual TemporaryRef<DataSourceSurface> GetOutput(const IntRect &aRect) MOZ_OVERRIDE;
  virtual TemporaryRef<DataSourceSurface> Render(const IntRect& aRect) MOZ_OVERRIDE;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) MOZ_OVERRIDE;

private:
  Color mColor;
};

class FilterNodeTileSoftware : public FilterNodeSoftware
{
public:
  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, const IntRect &aSourceRect) MOZ_OVERRIDE;

protected:
  virtual TemporaryRef<DataSourceSurface> Render(const IntRect& aRect) MOZ_OVERRIDE;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) MOZ_OVERRIDE;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) MOZ_OVERRIDE;
  virtual void RequestFromInputsForRect(const IntRect &aRect) MOZ_OVERRIDE;

private:
  IntRect mSourceRect;
};




class FilterNodeComponentTransferSoftware : public FilterNodeSoftware
{
public:
  FilterNodeComponentTransferSoftware();

  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, bool aDisable) MOZ_OVERRIDE;

protected:
  virtual TemporaryRef<DataSourceSurface> Render(const IntRect& aRect) MOZ_OVERRIDE;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) MOZ_OVERRIDE;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) MOZ_OVERRIDE;
  virtual void RequestFromInputsForRect(const IntRect &aRect) MOZ_OVERRIDE;
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
  using FilterNodeComponentTransferSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, const Float* aFloat, uint32_t aSize) MOZ_OVERRIDE;

protected:
  virtual void FillLookupTable(ptrdiff_t aComponent, uint8_t aTable[256]) MOZ_OVERRIDE;

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
  using FilterNodeComponentTransferSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, const Float* aFloat, uint32_t aSize) MOZ_OVERRIDE;

protected:
  virtual void FillLookupTable(ptrdiff_t aComponent, uint8_t aTable[256]) MOZ_OVERRIDE;

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
  FilterNodeLinearTransferSoftware();
  using FilterNodeComponentTransferSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, Float aValue) MOZ_OVERRIDE;

protected:
  virtual void FillLookupTable(ptrdiff_t aComponent, uint8_t aTable[256]) MOZ_OVERRIDE;

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
  FilterNodeGammaTransferSoftware();
  using FilterNodeComponentTransferSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, Float aValue) MOZ_OVERRIDE;

protected:
  virtual void FillLookupTable(ptrdiff_t aComponent, uint8_t aTable[256]) MOZ_OVERRIDE;

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
  FilterNodeConvolveMatrixSoftware();
  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, const IntSize &aKernelSize) MOZ_OVERRIDE;
  virtual void SetAttribute(uint32_t aIndex, const Float* aMatrix, uint32_t aSize) MOZ_OVERRIDE;
  virtual void SetAttribute(uint32_t aIndex, Float aValue) MOZ_OVERRIDE;
  virtual void SetAttribute(uint32_t aIndex, const Size &aKernelUnitLength) MOZ_OVERRIDE;
  virtual void SetAttribute(uint32_t aIndex, const IntRect &aSourceRect) MOZ_OVERRIDE;
  virtual void SetAttribute(uint32_t aIndex, const IntPoint &aTarget) MOZ_OVERRIDE;
  virtual void SetAttribute(uint32_t aIndex, uint32_t aEdgeMode) MOZ_OVERRIDE;
  virtual void SetAttribute(uint32_t aIndex, bool aPreserveAlpha) MOZ_OVERRIDE;

protected:
  virtual TemporaryRef<DataSourceSurface> Render(const IntRect& aRect) MOZ_OVERRIDE;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) MOZ_OVERRIDE;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) MOZ_OVERRIDE;
  virtual void RequestFromInputsForRect(const IntRect &aRect) MOZ_OVERRIDE;

private:
  template<typename CoordType>
  TemporaryRef<DataSourceSurface> DoRender(const IntRect& aRect,
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
  FilterNodeDisplacementMapSoftware();
  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, Float aScale) MOZ_OVERRIDE;
  virtual void SetAttribute(uint32_t aIndex, uint32_t aValue) MOZ_OVERRIDE;

protected:
  virtual TemporaryRef<DataSourceSurface> Render(const IntRect& aRect) MOZ_OVERRIDE;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) MOZ_OVERRIDE;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) MOZ_OVERRIDE;
  virtual void RequestFromInputsForRect(const IntRect &aRect) MOZ_OVERRIDE;

private:
  IntRect InflatedSourceOrDestRect(const IntRect &aDestOrSourceRect);

  Float mScale;
  ColorChannel mChannelX;
  ColorChannel mChannelY;
};

class FilterNodeTurbulenceSoftware : public FilterNodeSoftware
{
public:
  FilterNodeTurbulenceSoftware();
  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, const Size &aSize) MOZ_OVERRIDE;
  virtual void SetAttribute(uint32_t aIndex, const IntRect &aRenderRect) MOZ_OVERRIDE;
  virtual void SetAttribute(uint32_t aIndex, bool aStitchable) MOZ_OVERRIDE;
  virtual void SetAttribute(uint32_t aIndex, uint32_t aValue) MOZ_OVERRIDE;

protected:
  virtual TemporaryRef<DataSourceSurface> Render(const IntRect& aRect) MOZ_OVERRIDE;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) MOZ_OVERRIDE;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) MOZ_OVERRIDE;

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
  FilterNodeArithmeticCombineSoftware();
  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, const Float* aFloat, uint32_t aSize) MOZ_OVERRIDE;

protected:
  virtual TemporaryRef<DataSourceSurface> Render(const IntRect& aRect) MOZ_OVERRIDE;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) MOZ_OVERRIDE;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) MOZ_OVERRIDE;
  virtual void RequestFromInputsForRect(const IntRect &aRect) MOZ_OVERRIDE;

private:
  Float mK1;
  Float mK2;
  Float mK3;
  Float mK4;
};

class FilterNodeCompositeSoftware : public FilterNodeSoftware
{
public:
  FilterNodeCompositeSoftware();
  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, uint32_t aOperator) MOZ_OVERRIDE;

protected:
  virtual TemporaryRef<DataSourceSurface> Render(const IntRect& aRect) MOZ_OVERRIDE;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) MOZ_OVERRIDE;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) MOZ_OVERRIDE;
  virtual void RequestFromInputsForRect(const IntRect &aRect) MOZ_OVERRIDE;

private:
  CompositeOperator mOperator;
};



class FilterNodeBlurXYSoftware : public FilterNodeSoftware
{
protected:
  virtual TemporaryRef<DataSourceSurface> Render(const IntRect& aRect) MOZ_OVERRIDE;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) MOZ_OVERRIDE;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) MOZ_OVERRIDE;
  IntRect InflatedSourceOrDestRect(const IntRect &aDestRect);
  virtual void RequestFromInputsForRect(const IntRect &aRect) MOZ_OVERRIDE;

  
  virtual Size StdDeviationXY() = 0;
};

class FilterNodeGaussianBlurSoftware : public FilterNodeBlurXYSoftware
{
public:
  FilterNodeGaussianBlurSoftware();
  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, Float aStdDeviation) MOZ_OVERRIDE;

protected:
  virtual Size StdDeviationXY() MOZ_OVERRIDE;

private:
  Float mStdDeviation;
};

class FilterNodeDirectionalBlurSoftware : public FilterNodeBlurXYSoftware
{
public:
  FilterNodeDirectionalBlurSoftware();
  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, Float aStdDeviation) MOZ_OVERRIDE;
  virtual void SetAttribute(uint32_t aIndex, uint32_t aBlurDirection) MOZ_OVERRIDE;

protected:
  virtual Size StdDeviationXY() MOZ_OVERRIDE;

private:
  Float mStdDeviation;
  BlurDirection mBlurDirection;
};

class FilterNodeCropSoftware : public FilterNodeSoftware
{
public:
  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, const Rect &aSourceRect) MOZ_OVERRIDE;

protected:
  virtual TemporaryRef<DataSourceSurface> Render(const IntRect& aRect) MOZ_OVERRIDE;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) MOZ_OVERRIDE;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) MOZ_OVERRIDE;
  virtual void RequestFromInputsForRect(const IntRect &aRect) MOZ_OVERRIDE;

private:
  IntRect mCropRect;
};

class FilterNodePremultiplySoftware : public FilterNodeSoftware
{
protected:
  virtual TemporaryRef<DataSourceSurface> Render(const IntRect& aRect) MOZ_OVERRIDE;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) MOZ_OVERRIDE;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) MOZ_OVERRIDE;
  virtual void RequestFromInputsForRect(const IntRect &aRect) MOZ_OVERRIDE;
};

class FilterNodeUnpremultiplySoftware : public FilterNodeSoftware
{
protected:
  virtual TemporaryRef<DataSourceSurface> Render(const IntRect& aRect) MOZ_OVERRIDE;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) MOZ_OVERRIDE;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) MOZ_OVERRIDE;
  virtual void RequestFromInputsForRect(const IntRect &aRect) MOZ_OVERRIDE;
};

template<typename LightType, typename LightingType>
class FilterNodeLightingSoftware : public FilterNodeSoftware
{
public:
  FilterNodeLightingSoftware();
  using FilterNodeSoftware::SetAttribute;
  virtual void SetAttribute(uint32_t aIndex, Float) MOZ_OVERRIDE;
  virtual void SetAttribute(uint32_t aIndex, const Size &) MOZ_OVERRIDE;
  virtual void SetAttribute(uint32_t aIndex, const Point3D &) MOZ_OVERRIDE;
  virtual void SetAttribute(uint32_t aIndex, const Color &) MOZ_OVERRIDE;

protected:
  virtual TemporaryRef<DataSourceSurface> Render(const IntRect& aRect) MOZ_OVERRIDE;
  virtual IntRect GetOutputRectInRect(const IntRect& aRect) MOZ_OVERRIDE;
  virtual int32_t InputIndex(uint32_t aInputEnumIndex) MOZ_OVERRIDE;
  virtual void RequestFromInputsForRect(const IntRect &aRect) MOZ_OVERRIDE;

private:
  template<typename CoordType>
  TemporaryRef<DataSourceSurface> DoRender(const IntRect& aRect,
                                           CoordType aKernelUnitLengthX,
                                           CoordType aKernelUnitLengthY);

  LightType mLight;
  LightingType mLighting;
  Float mSurfaceScale;
  Size mKernelUnitLength;
  Color mColor;
};

}
}

#endif 
