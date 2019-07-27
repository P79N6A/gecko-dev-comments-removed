




#ifndef MOZILLA_GFX_FILTERNODED2D1_H_
#define MOZILLA_GFX_FILTERNODED2D1_H_

#include "2D.h"
#include "Filters.h"
#include <vector>
#include <d2d1_1.h>
#include <cguid.h>

namespace mozilla {
namespace gfx {

class FilterNodeD2D1 : public FilterNode
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(FilterNodeD2D1)
  static TemporaryRef<FilterNode> Create(ID2D1DeviceContext *aDC, FilterType aType);

  FilterNodeD2D1(ID2D1Effect *aEffect, FilterType aType)
    : mEffect(aEffect)
    , mType(aType)
  {
    InitUnmappedProperties();
  }

  virtual FilterBackend GetBackendType() { return FILTER_BACKEND_DIRECT2D1_1; }

  virtual void SetInput(uint32_t aIndex, SourceSurface *aSurface);
  virtual void SetInput(uint32_t aIndex, FilterNode *aFilter);

  virtual void SetAttribute(uint32_t aIndex, uint32_t aValue);
  virtual void SetAttribute(uint32_t aIndex, Float aValue);
  virtual void SetAttribute(uint32_t aIndex, const Point &aValue);
  virtual void SetAttribute(uint32_t aIndex, const Matrix5x4 &aValue);
  virtual void SetAttribute(uint32_t aIndex, const Point3D &aValue);
  virtual void SetAttribute(uint32_t aIndex, const Size &aValue);
  virtual void SetAttribute(uint32_t aIndex, const IntSize &aValue);
  virtual void SetAttribute(uint32_t aIndex, const Color &aValue);
  virtual void SetAttribute(uint32_t aIndex, const Rect &aValue);
  virtual void SetAttribute(uint32_t aIndex, const IntRect &aValue);
  virtual void SetAttribute(uint32_t aIndex, bool aValue);
  virtual void SetAttribute(uint32_t aIndex, const Float *aValues, uint32_t aSize);
  virtual void SetAttribute(uint32_t aIndex, const IntPoint &aValue);
  virtual void SetAttribute(uint32_t aIndex, const Matrix &aValue);

  
  
  
  
  virtual void WillDraw(DrawTarget *aDT);

protected:
  friend class DrawTargetD2D1;
  friend class DrawTargetD2D;
  friend class FilterNodeConvolveD2D1;

  virtual ID2D1Effect* InputEffect() { return mEffect.get(); }
  virtual ID2D1Effect* OutputEffect() { return mEffect.get(); }

  void InitUnmappedProperties();

  RefPtr<ID2D1Effect> mEffect;
  std::vector<RefPtr<FilterNodeD2D1>> mInputFilters;
  std::vector<RefPtr<SourceSurface>> mInputSurfaces;
  FilterType mType;
};

class FilterNodeConvolveD2D1 : public FilterNodeD2D1
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(FilterNodeConvolveD2D1)
  FilterNodeConvolveD2D1(ID2D1DeviceContext *aDC);

  virtual void SetInput(uint32_t aIndex, FilterNode *aFilter);

  virtual void SetAttribute(uint32_t aIndex, uint32_t aValue);
  virtual void SetAttribute(uint32_t aIndex, const IntSize &aValue);
  virtual void SetAttribute(uint32_t aIndex, const IntPoint &aValue);
  virtual void SetAttribute(uint32_t aIndex, const IntRect &aValue);

protected:
  virtual ID2D1Effect* InputEffect();

private:
  void UpdateChain();
  void UpdateOffset();
  void UpdateSourceRect();

  RefPtr<ID2D1Effect> mFloodEffect;
  RefPtr<ID2D1Effect> mCompositeEffect;
  RefPtr<ID2D1Effect> mCropEffect;
  RefPtr<ID2D1Effect> mBorderEffect;
  ConvolveMatrixEdgeMode mEdgeMode;
  IntPoint mTarget;
  IntSize mKernelSize;
  IntRect mSourceRect;
};

class FilterNodeComponentTransferD2D1 : public FilterNodeD2D1
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(FilterNodeComponentTransferD2D1)
  FilterNodeComponentTransferD2D1(ID2D1DeviceContext *aDC, ID2D1Effect *aEffect, FilterType aType);

protected:
  virtual ID2D1Effect* InputEffect() MOZ_OVERRIDE { return mPrePremultiplyEffect.get(); }
  virtual ID2D1Effect* OutputEffect() MOZ_OVERRIDE { return mPostUnpremultiplyEffect.get(); }

private:
  RefPtr<ID2D1Effect> mPrePremultiplyEffect;
  RefPtr<ID2D1Effect> mPostUnpremultiplyEffect;
};

}
}

#endif
