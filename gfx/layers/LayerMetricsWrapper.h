




#ifndef GFX_LAYERMETRICSWRAPPER_H
#define GFX_LAYERMETRICSWRAPPER_H

#include "Layers.h"

namespace mozilla {
namespace layers {












































































































class MOZ_STACK_CLASS LayerMetricsWrapper {
public:
  LayerMetricsWrapper()
    : mLayer(nullptr)
    , mIndex(0)
  {
  }

  explicit LayerMetricsWrapper(Layer* aRoot)
    : mLayer(aRoot)
    , mIndex(0)
  {
    if (!mLayer) {
      return;
    }

    mIndex = mLayer->GetFrameMetricsCount();
    if (mIndex > 0) {
      mIndex--;
    }
  }

  explicit LayerMetricsWrapper(Layer* aLayer, uint32_t aMetricsIndex)
    : mLayer(aLayer)
    , mIndex(aMetricsIndex)
  {
    MOZ_ASSERT(mLayer);
    MOZ_ASSERT(mIndex == 0 || mIndex < mLayer->GetFrameMetricsCount());
  }

  bool IsValid() const
  {
    return mLayer != nullptr;
  }

  MOZ_EXPLICIT_CONVERSION operator bool() const
  {
    return IsValid();
  }

  LayerMetricsWrapper GetLastChild() const
  {
    MOZ_ASSERT(IsValid());

    if (!AtBottomLayer()) {
      return LayerMetricsWrapper(mLayer, mIndex - 1);
    }
    return LayerMetricsWrapper(mLayer->GetLastChild());
  }

  LayerMetricsWrapper GetPrevSibling() const
  {
    MOZ_ASSERT(IsValid());

    if (AtTopLayer()) {
      return LayerMetricsWrapper(mLayer->GetPrevSibling());
    }
    return LayerMetricsWrapper(nullptr);
  }

  const FrameMetrics& Metrics() const
  {
    MOZ_ASSERT(IsValid());

    if (mIndex >= mLayer->GetFrameMetricsCount()) {
      return FrameMetrics::sNullMetrics;
    }
    return mLayer->GetFrameMetrics(mIndex);
  }

  AsyncPanZoomController* GetApzc() const
  {
    MOZ_ASSERT(IsValid());

    if (mIndex >= mLayer->GetFrameMetricsCount()) {
      return nullptr;
    }
    return mLayer->GetAsyncPanZoomController(mIndex);
  }

  void SetApzc(AsyncPanZoomController* aApzc) const
  {
    MOZ_ASSERT(IsValid());

    if (mLayer->GetFrameMetricsCount() == 0) {
      MOZ_ASSERT(mIndex == 0);
      MOZ_ASSERT(aApzc == nullptr);
      return;
    }
    MOZ_ASSERT(mIndex < mLayer->GetFrameMetricsCount());
    mLayer->SetAsyncPanZoomController(mIndex, aApzc);
  }

  const char* Name() const
  {
    MOZ_ASSERT(IsValid());

    if (AtBottomLayer()) {
      return mLayer->Name();
    }
    return "DummyContainerLayer";
  }

  gfx::Matrix4x4 GetTransform() const
  {
    MOZ_ASSERT(IsValid());

    if (AtBottomLayer()) {
      return mLayer->GetTransform();
    }
    return gfx::Matrix4x4();
  }

  RefLayer* AsRefLayer() const
  {
    MOZ_ASSERT(IsValid());

    if (AtBottomLayer()) {
      return mLayer->AsRefLayer();
    }
    return nullptr;
  }

  nsIntRegion GetVisibleRegion() const
  {
    MOZ_ASSERT(IsValid());

    if (AtBottomLayer()) {
      return mLayer->GetVisibleRegion();
    }
    nsIntRegion region = mLayer->GetVisibleRegion();
    region.Transform(gfx::To3DMatrix(mLayer->GetTransform()));
    return region;
  }

  const nsIntRect* GetClipRect() const
  {
    MOZ_ASSERT(IsValid());

    return mLayer->GetClipRect();
  }

  const std::string& GetContentDescription() const
  {
    MOZ_ASSERT(IsValid());

    return mLayer->GetContentDescription();
  }

  
  
  
  const void* GetLayer() const
  {
    MOZ_ASSERT(IsValid());

    return (void*)mLayer;
  }

private:
  bool AtBottomLayer() const
  {
    return mIndex == 0;
  }

  bool AtTopLayer() const
  {
    return mLayer->GetFrameMetricsCount() == 0 || mIndex == mLayer->GetFrameMetricsCount() - 1;
  }

private:
  Layer* mLayer;
  uint32_t mIndex;
};

}
}

#endif 
