




#ifndef GFX_LAYERMETRICSWRAPPER_H
#define GFX_LAYERMETRICSWRAPPER_H

#include "Layers.h"

namespace mozilla {
namespace layers {












































































































class MOZ_STACK_CLASS LayerMetricsWrapper {
public:
  enum StartAt {
    TOP,
    BOTTOM,
  };

  LayerMetricsWrapper()
    : mLayer(nullptr)
    , mIndex(0)
  {
  }

  explicit LayerMetricsWrapper(Layer* aRoot, StartAt aStart = StartAt::TOP)
    : mLayer(aRoot)
    , mIndex(0)
  {
    if (!mLayer) {
      return;
    }

    switch (aStart) {
      case StartAt::TOP:
        mIndex = mLayer->GetFrameMetricsCount();
        if (mIndex > 0) {
          mIndex--;
        }
        break;
      case StartAt::BOTTOM:
        mIndex = 0;
        break;
      default:
        MOZ_ASSERT_UNREACHABLE("Unknown startAt value");
        break;
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

  bool IsScrollInfoLayer() const
  {
    MOZ_ASSERT(IsValid());

    
    
    
    
    return Metrics().IsScrollable()
        && mLayer->AsContainerLayer()
        && !mLayer->GetFirstChild();
  }

  LayerMetricsWrapper GetParent() const
  {
    MOZ_ASSERT(IsValid());

    if (!AtTopLayer()) {
      return LayerMetricsWrapper(mLayer, mIndex + 1);
    }
    if (mLayer->GetParent()) {
      return LayerMetricsWrapper(mLayer->GetParent(), StartAt::BOTTOM);
    }
    return LayerMetricsWrapper(nullptr);
  }

  LayerMetricsWrapper GetFirstChild() const
  {
    MOZ_ASSERT(IsValid());

    if (!AtBottomLayer()) {
      return LayerMetricsWrapper(mLayer, mIndex - 1);
    }
    return LayerMetricsWrapper(mLayer->GetFirstChild());
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

  LayerMetricsWrapper GetNextSibling() const
  {
    MOZ_ASSERT(IsValid());

    if (AtTopLayer()) {
      return LayerMetricsWrapper(mLayer->GetNextSibling());
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

  LayerManager* Manager() const
  {
    MOZ_ASSERT(IsValid());

    return mLayer->Manager();
  }

  gfx::Matrix4x4 GetTransform() const
  {
    MOZ_ASSERT(IsValid());

    if (AtBottomLayer()) {
      return mLayer->GetTransform();
    }
    return gfx::Matrix4x4();
  }

  EventRegions GetEventRegions() const
  {
    MOZ_ASSERT(IsValid());

    if (AtBottomLayer()) {
      return mLayer->GetEventRegions();
    }
    return EventRegions();
  }

  bool HasTransformAnimation() const
  {
    MOZ_ASSERT(IsValid());

    if (AtBottomLayer()) {
      return mLayer->HasTransformAnimation();
    }
    return false;
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

  const Maybe<ParentLayerIntRect>& GetClipRect() const
  {
    MOZ_ASSERT(IsValid());

    static const Maybe<ParentLayerIntRect> sNoClipRect = Nothing();

    if (AtBottomLayer()) {
      return mLayer->GetClipRect();
    }

    return sNoClipRect;
  }

  EventRegionsOverride GetEventRegionsOverride() const
  {
    MOZ_ASSERT(IsValid());

    if (mLayer->AsContainerLayer()) {
      return mLayer->AsContainerLayer()->GetEventRegionsOverride();
    }
    return EventRegionsOverride::NoOverride;
  }

  
  
  
  const void* GetLayer() const
  {
    MOZ_ASSERT(IsValid());

    return (void*)mLayer;
  }

  bool operator==(const LayerMetricsWrapper& aOther) const
  {
    return mLayer == aOther.mLayer
        && mIndex == aOther.mIndex;
  }

  bool operator!=(const LayerMetricsWrapper& aOther) const
  {
    return !(*this == aOther);
  }

  static const FrameMetrics& TopmostScrollableMetrics(Layer* aLayer)
  {
    for (uint32_t i = aLayer->GetFrameMetricsCount(); i > 0; i--) {
      if (aLayer->GetFrameMetrics(i - 1).IsScrollable()) {
        return aLayer->GetFrameMetrics(i - 1);
      }
    }
    return FrameMetrics::sNullMetrics;
  }

  static const FrameMetrics& BottommostScrollableMetrics(Layer* aLayer)
  {
    for (uint32_t i = 0; i < aLayer->GetFrameMetricsCount(); i++) {
      if (aLayer->GetFrameMetrics(i).IsScrollable()) {
        return aLayer->GetFrameMetrics(i);
      }
    }
    return FrameMetrics::sNullMetrics;
  }

  static const FrameMetrics& BottommostMetrics(Layer* aLayer)
  {
    if (aLayer->GetFrameMetricsCount() > 0) {
      return aLayer->GetFrameMetrics(0);
    }
    return FrameMetrics::sNullMetrics;
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
