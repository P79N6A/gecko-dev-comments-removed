




#ifndef GFX_FRAMEMETRICS_H
#define GFX_FRAMEMETRICS_H

#include <stdint.h>                     
#include <string>                       
#include "Units.h"                      
#include "mozilla/gfx/BasePoint.h"      
#include "mozilla/gfx/Rect.h"           
#include "mozilla/gfx/ScaleFactor.h"    
#include "mozilla/gfx/Logging.h"        

namespace IPC {
template <typename T> struct ParamTraits;
} 

namespace mozilla {











struct ParentLayerPixel {};

typedef gfx::PointTyped<ParentLayerPixel> ParentLayerPoint;
typedef gfx::RectTyped<ParentLayerPixel> ParentLayerRect;
typedef gfx::SizeTyped<ParentLayerPixel> ParentLayerSize;

typedef gfx::IntMarginTyped<ParentLayerPixel> ParentLayerIntMargin;
typedef gfx::IntPointTyped<ParentLayerPixel> ParentLayerIntPoint;
typedef gfx::IntRectTyped<ParentLayerPixel> ParentLayerIntRect;
typedef gfx::IntSizeTyped<ParentLayerPixel> ParentLayerIntSize;

typedef gfx::ScaleFactor<CSSPixel, ParentLayerPixel> CSSToParentLayerScale;
typedef gfx::ScaleFactor<LayoutDevicePixel, ParentLayerPixel> LayoutDeviceToParentLayerScale;
typedef gfx::ScaleFactor<ScreenPixel, ParentLayerPixel> ScreenToParentLayerScale;

typedef gfx::ScaleFactor<ParentLayerPixel, LayerPixel> ParentLayerToLayerScale;
typedef gfx::ScaleFactor<ParentLayerPixel, ScreenPixel> ParentLayerToScreenScale;


namespace layers {







struct FrameMetrics {
  friend struct IPC::ParamTraits<mozilla::layers::FrameMetrics>;
public:
  
  typedef uint64_t ViewID;
  static const ViewID NULL_SCROLL_ID;   
  static const ViewID START_SCROLL_ID = 2;  
                                        

  FrameMetrics()
    : mCompositionBounds(0, 0, 0, 0)
    , mDisplayPort(0, 0, 0, 0)
    , mCriticalDisplayPort(0, 0, 0, 0)
    , mViewport(0, 0, 0, 0)
    , mScrollableRect(0, 0, 0, 0)
    , mResolution(1)
    , mCumulativeResolution(1)
    , mTransformScale(1)
    , mDevPixelsPerCSSPixel(1)
    , mPresShellId(-1)
    , mMayHaveTouchListeners(false)
    , mIsRoot(false)
    , mHasScrollgrab(false)
    , mScrollId(NULL_SCROLL_ID)
    , mScrollOffset(0, 0)
    , mZoom(1)
    , mUpdateScrollOffset(false)
    , mScrollGeneration(0)
  {}

  

  bool operator==(const FrameMetrics& aOther) const
  {
    
    
    return mCompositionBounds.IsEqualEdges(aOther.mCompositionBounds) &&
           mDisplayPort.IsEqualEdges(aOther.mDisplayPort) &&
           mCriticalDisplayPort.IsEqualEdges(aOther.mCriticalDisplayPort) &&
           mViewport.IsEqualEdges(aOther.mViewport) &&
           mScrollableRect.IsEqualEdges(aOther.mScrollableRect) &&
           mResolution == aOther.mResolution &&
           mCumulativeResolution == aOther.mCumulativeResolution &&
           mDevPixelsPerCSSPixel == aOther.mDevPixelsPerCSSPixel &&
           mMayHaveTouchListeners == aOther.mMayHaveTouchListeners &&
           mPresShellId == aOther.mPresShellId &&
           mIsRoot == aOther.mIsRoot &&
           mScrollId == aOther.mScrollId &&
           mScrollOffset == aOther.mScrollOffset &&
           mHasScrollgrab == aOther.mHasScrollgrab &&
           mUpdateScrollOffset == aOther.mUpdateScrollOffset;
  }
  bool operator!=(const FrameMetrics& aOther) const
  {
    return !operator==(aOther);
  }

  bool IsDefault() const
  {
    FrameMetrics def;

    def.mPresShellId = mPresShellId;
    return (def == *this);
  }

  bool IsRootScrollable() const
  {
    return mIsRoot;
  }

  bool IsScrollable() const
  {
    return mScrollId != NULL_SCROLL_ID;
  }

  CSSToLayerScale LayersPixelsPerCSSPixel() const
  {
    return mCumulativeResolution * mDevPixelsPerCSSPixel;
  }

  LayerPoint GetScrollOffsetInLayerPixels() const
  {
    return GetScrollOffset() * LayersPixelsPerCSSPixel();
  }

  LayoutDeviceToParentLayerScale GetParentResolution() const
  {
    return mCumulativeResolution / mResolution;
  }

  
  
  
  
  
  
  CSSRect GetExpandedScrollableRect() const
  {
    CSSRect scrollableRect = mScrollableRect;
    CSSRect compBounds = CSSRect(CalculateCompositedRectInCssPixels());
    if (scrollableRect.width < compBounds.width) {
      scrollableRect.x = std::max(0.f,
                                  scrollableRect.x - (compBounds.width - scrollableRect.width));
      scrollableRect.width = compBounds.width;
    }

    if (scrollableRect.height < compBounds.height) {
      scrollableRect.y = std::max(0.f,
                                  scrollableRect.y - (compBounds.height - scrollableRect.height));
      scrollableRect.height = compBounds.height;
    }

    return scrollableRect;
  }

  
  
  CSSToScreenScale CalculateIntrinsicScale() const
  {
    return CSSToScreenScale(float(mCompositionBounds.width) / float(mViewport.width));
  }

  
  
  
  
  
  CSSToParentLayerScale GetZoomToParent() const
  {
    return mZoom * mTransformScale;
  }

  CSSIntRect CalculateCompositedRectInCssPixels() const
  {
    return gfx::RoundedIn(mCompositionBounds / GetZoomToParent());
  }

  void ScrollBy(const CSSPoint& aPoint)
  {
    mScrollOffset += aPoint;
  }

  void ZoomBy(float aFactor)
  {
    mZoom.scale *= aFactor;
  }

  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  ParentLayerIntRect mCompositionBounds;

  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  CSSRect mDisplayPort;

  
  
  
  
  
  
  CSSRect mCriticalDisplayPort;

  
  
  
  
  
  
  
  
  
  CSSRect mViewport;

  
  
  
  
  
  
  
  
  
  
  
  CSSRect mScrollableRect;

  
  
  

  
  
  
  ParentLayerToLayerScale mResolution;

  
  
  
  LayoutDeviceToLayerScale mCumulativeResolution;

  
  
  
  
  
  
  
  
  
  ScreenToParentLayerScale mTransformScale;

  
  
  
  
  CSSToLayoutDeviceScale mDevPixelsPerCSSPixel;

  uint32_t mPresShellId;

  
  bool mMayHaveTouchListeners;

  
  bool mIsRoot;

  
  bool mHasScrollgrab;

public:
  void SetScrollOffset(const CSSPoint& aScrollOffset)
  {
    mScrollOffset = aScrollOffset;
  }

  const CSSPoint& GetScrollOffset() const
  {
    return mScrollOffset;
  }

  void SetZoom(const CSSToScreenScale& aZoom)
  {
    mZoom = aZoom;
  }

  CSSToScreenScale GetZoom() const
  {
    return mZoom;
  }

  void SetScrollOffsetUpdated(uint32_t aScrollGeneration)
  {
    mUpdateScrollOffset = true;
    mScrollGeneration = aScrollGeneration;
  }

  bool GetScrollOffsetUpdated() const
  {
    return mUpdateScrollOffset;
  }

  uint32_t GetScrollGeneration() const
  {
    return mScrollGeneration;
  }

  const std::string& GetContentDescription() const
  {
    return mContentDescription;
  }

  void SetContentDescription(const std::string& aContentDescription)
  {
    mContentDescription = aContentDescription;
  }

  ViewID GetScrollId() const
  {
    return mScrollId;
  }
  
  void SetScrollId(ViewID scrollId) 
  {
    mScrollId = scrollId;
  }
  
private:
  
  

  
  ViewID mScrollId;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  CSSPoint mScrollOffset;

  
  
  
  
  CSSToScreenScale mZoom;

  
  
  bool mUpdateScrollOffset;
  
  uint32_t mScrollGeneration;

  
  
  std::string mContentDescription;
};









struct ScrollableLayerGuid {
  uint64_t mLayersId;
  uint32_t mPresShellId;
  FrameMetrics::ViewID mScrollId;

  ScrollableLayerGuid()
    : mLayersId(0)
    , mPresShellId(0)
    , mScrollId(0)
  {
    MOZ_COUNT_CTOR(ScrollableLayerGuid);
  }

  ScrollableLayerGuid(uint64_t aLayersId, uint32_t aPresShellId,
                      FrameMetrics::ViewID aScrollId)
    : mLayersId(aLayersId)
    , mPresShellId(aPresShellId)
    , mScrollId(aScrollId)
  {
    MOZ_COUNT_CTOR(ScrollableLayerGuid);
  }

  ScrollableLayerGuid(uint64_t aLayersId, const FrameMetrics& aMetrics)
    : mLayersId(aLayersId)
    , mPresShellId(aMetrics.mPresShellId)
    , mScrollId(aMetrics.GetScrollId())
  {
    MOZ_COUNT_CTOR(ScrollableLayerGuid);
  }

  ~ScrollableLayerGuid()
  {
    MOZ_COUNT_DTOR(ScrollableLayerGuid);
  }

  bool operator==(const ScrollableLayerGuid& other) const
  {
    return mLayersId == other.mLayersId
        && mPresShellId == other.mPresShellId
        && mScrollId == other.mScrollId;
  }

  bool operator!=(const ScrollableLayerGuid& other) const
  {
    return !(*this == other);
  }
};

template <int LogLevel>
gfx::Log<LogLevel>& operator<<(gfx::Log<LogLevel>& log, const ScrollableLayerGuid& aGuid) {
  return log << '(' << aGuid.mLayersId << ',' << aGuid.mPresShellId << ',' << aGuid.mScrollId << ')';
}

struct ZoomConstraints {
  bool mAllowZoom;
  bool mAllowDoubleTapZoom;
  CSSToScreenScale mMinZoom;
  CSSToScreenScale mMaxZoom;

  ZoomConstraints()
    : mAllowZoom(true)
    , mAllowDoubleTapZoom(true)
  {
    MOZ_COUNT_CTOR(ZoomConstraints);
  }

  ZoomConstraints(bool aAllowZoom,
                  bool aAllowDoubleTapZoom,
                  const CSSToScreenScale& aMinZoom,
                  const CSSToScreenScale& aMaxZoom)
    : mAllowZoom(aAllowZoom)
    , mAllowDoubleTapZoom(aAllowDoubleTapZoom)
    , mMinZoom(aMinZoom)
    , mMaxZoom(aMaxZoom)
  {
    MOZ_COUNT_CTOR(ZoomConstraints);
  }

  ~ZoomConstraints()
  {
    MOZ_COUNT_DTOR(ZoomConstraints);
  }

  bool operator==(const ZoomConstraints& other) const
  {
    return mAllowZoom == other.mAllowZoom
        && mAllowDoubleTapZoom == other.mAllowDoubleTapZoom
        && mMinZoom == other.mMinZoom
        && mMaxZoom == other.mMaxZoom;
  }

  bool operator!=(const ZoomConstraints& other) const
  {
    return !(*this == other);
  }
};

}
}

#endif 
