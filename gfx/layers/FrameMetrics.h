




#ifndef GFX_FRAMEMETRICS_H
#define GFX_FRAMEMETRICS_H

#include <stdint.h>                     
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

template<> struct IsPixel<ParentLayerPixel> : TrueType {};

typedef gfx::MarginTyped<ParentLayerPixel> ParentLayerMargin;
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
                                        
  static const FrameMetrics sNullMetrics;   

  FrameMetrics()
    : mCompositionBounds(0, 0, 0, 0)
    , mDisplayPort(0, 0, 0, 0)
    , mCriticalDisplayPort(0, 0, 0, 0)
    , mScrollableRect(0, 0, 0, 0)
    , mResolution(1)
    , mCumulativeResolution(1)
    , mTransformScale(1)
    , mDevPixelsPerCSSPixel(1)
    , mMayHaveTouchListeners(false)
    , mMayHaveTouchCaret(false)
    , mIsRoot(false)
    , mHasScrollgrab(false)
    , mScrollId(NULL_SCROLL_ID)
    , mScrollParentId(NULL_SCROLL_ID)
    , mScrollOffset(0, 0)
    , mZoom(1)
    , mUpdateScrollOffset(false)
    , mScrollGeneration(0)
    , mRootCompositionSize(0, 0)
    , mDisplayPortMargins(0, 0, 0, 0)
    , mUseDisplayPortMargins(false)
    , mPresShellId(-1)
    , mViewport(0, 0, 0, 0)
  {}

  

  bool operator==(const FrameMetrics& aOther) const
  {
    return mCompositionBounds.IsEqualEdges(aOther.mCompositionBounds) &&
           mRootCompositionSize == aOther.mRootCompositionSize &&
           mDisplayPort.IsEqualEdges(aOther.mDisplayPort) &&
           mDisplayPortMargins == aOther.mDisplayPortMargins &&
           mUseDisplayPortMargins == aOther.mUseDisplayPortMargins &&
           mCriticalDisplayPort.IsEqualEdges(aOther.mCriticalDisplayPort) &&
           mViewport.IsEqualEdges(aOther.mViewport) &&
           mScrollableRect.IsEqualEdges(aOther.mScrollableRect) &&
           mResolution == aOther.mResolution &&
           mCumulativeResolution == aOther.mCumulativeResolution &&
           mDevPixelsPerCSSPixel == aOther.mDevPixelsPerCSSPixel &&
           mMayHaveTouchListeners == aOther.mMayHaveTouchListeners &&
           mMayHaveTouchCaret == aOther.mMayHaveTouchCaret &&
           mPresShellId == aOther.mPresShellId &&
           mIsRoot == aOther.mIsRoot &&
           mScrollId == aOther.mScrollId &&
           mScrollParentId == aOther.mScrollParentId &&
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
    CSSSize compSize = CalculateCompositedSizeInCssPixels();
    if (scrollableRect.width < compSize.width) {
      scrollableRect.x = std::max(0.f,
                                  scrollableRect.x - (compSize.width - scrollableRect.width));
      scrollableRect.width = compSize.width;
    }

    if (scrollableRect.height < compSize.height) {
      scrollableRect.y = std::max(0.f,
                                  scrollableRect.y - (compSize.height - scrollableRect.height));
      scrollableRect.height = compSize.height;
    }

    return scrollableRect;
  }

  
  
  CSSToScreenScale CalculateIntrinsicScale() const
  {
    return CSSToScreenScale(
        std::max(mCompositionBounds.width / mViewport.width,
                 mCompositionBounds.height / mViewport.height));
  }

  
  
  
  
  
  CSSToParentLayerScale GetZoomToParent() const
  {
    return mZoom * mTransformScale;
  }

  CSSSize CalculateCompositedSizeInCssPixels() const
  {
    return mCompositionBounds.Size() / GetZoomToParent();
  }

  CSSRect CalculateCompositedRectInCssPixels() const
  {
    return mCompositionBounds / GetZoomToParent();
  }

  CSSSize CalculateBoundedCompositedSizeInCssPixels() const
  {
    CSSSize size = CalculateCompositedSizeInCssPixels();
    size.width = std::min(size.width, mRootCompositionSize.width);
    size.height = std::min(size.height, mRootCompositionSize.height);
    return size;
  }

  void ScrollBy(const CSSPoint& aPoint)
  {
    mScrollOffset += aPoint;
  }

  void ZoomBy(float aFactor)
  {
    mZoom.scale *= aFactor;
  }

  void CopyScrollInfoFrom(const FrameMetrics& aOther)
  {
    mScrollOffset = aOther.mScrollOffset;
    mScrollGeneration = aOther.mScrollGeneration;
  }

  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  ParentLayerRect mCompositionBounds;

  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  CSSRect mDisplayPort;

  
  
  
  
  
  
  CSSRect mCriticalDisplayPort;

  
  
  
  
  
  
  
  
  
  
  
  CSSRect mScrollableRect;

  
  
  

  
  
  
  ParentLayerToLayerScale mResolution;

  
  
  
  LayoutDeviceToLayerScale mCumulativeResolution;

  
  
  
  
  
  
  
  
  
  ScreenToParentLayerScale mTransformScale;

  
  
  
  
  CSSToLayoutDeviceScale mDevPixelsPerCSSPixel;

  
  bool mMayHaveTouchListeners;

  
  bool mMayHaveTouchCaret;

public:
  void SetIsRoot(bool aIsRoot)
  {
    mIsRoot = aIsRoot;
  }

  bool GetIsRoot() const
  {
    return mIsRoot;
  }

  void SetHasScrollgrab(bool aHasScrollgrab)
  {
    mHasScrollgrab = aHasScrollgrab;
  }

  bool GetHasScrollgrab() const
  {
    return mHasScrollgrab;
  }

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

  ViewID GetScrollId() const
  {
    return mScrollId;
  }

  void SetScrollId(ViewID scrollId)
  {
    mScrollId = scrollId;
  }

  ViewID GetScrollParentId() const
  {
    return mScrollParentId;
  }

  void SetScrollParentId(ViewID aParentId)
  {
    mScrollParentId = aParentId;
  }

  void SetRootCompositionSize(const CSSSize& aRootCompositionSize)
  {
    mRootCompositionSize = aRootCompositionSize;
  }

  const CSSSize& GetRootCompositionSize() const
  {
    return mRootCompositionSize;
  }

  void SetDisplayPortMargins(const LayerMargin& aDisplayPortMargins)
  {
    mDisplayPortMargins = aDisplayPortMargins;
  }

  const LayerMargin& GetDisplayPortMargins() const
  {
    return mDisplayPortMargins;
  }

  void SetUseDisplayPortMargins()
  {
    mUseDisplayPortMargins = true;
  }

  bool GetUseDisplayPortMargins() const
  {
    return mUseDisplayPortMargins;
  }

  uint32_t GetPresShellId() const
  {
    return mPresShellId;
  }

  void SetPresShellId(uint32_t aPresShellId)
  {
    mPresShellId = aPresShellId;
  }

  void SetViewport(const CSSRect& aViewport)
  {
    mViewport = aViewport;
  }

  const CSSRect& GetViewport() const
  {
    return mViewport;
  }

private:
  
  

  
  bool mIsRoot;

  
  bool mHasScrollgrab;

  
  ViewID mScrollId;

  
  ViewID mScrollParentId;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  CSSPoint mScrollOffset;

  
  
  
  
  CSSToScreenScale mZoom;

  
  
  bool mUpdateScrollOffset;
  
  uint32_t mScrollGeneration;

  
  CSSSize mRootCompositionSize;

  
  
  LayerMargin mDisplayPortMargins;

  
  
  bool mUseDisplayPortMargins;

  uint32_t mPresShellId;

  
  
  
  
  
  
  
  
  
  CSSRect mViewport;
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
    , mPresShellId(aMetrics.GetPresShellId())
    , mScrollId(aMetrics.GetScrollId())
  {
    MOZ_COUNT_CTOR(ScrollableLayerGuid);
  }

  ScrollableLayerGuid(const ScrollableLayerGuid& other)
    : mLayersId(other.mLayersId)
    , mPresShellId(other.mPresShellId)
    , mScrollId(other.mScrollId)
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

  bool operator<(const ScrollableLayerGuid& other) const
  {
    if (mLayersId < other.mLayersId) {
      return true;
    }
    if (mLayersId == other.mLayersId) {
      if (mPresShellId < other.mPresShellId) {
        return true;
      }
      if (mPresShellId == other.mPresShellId) {
        return mScrollId < other.mScrollId;
      }
    }
    return false;
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

  ZoomConstraints(const ZoomConstraints& other)
    : mAllowZoom(other.mAllowZoom)
    , mAllowDoubleTapZoom(other.mAllowDoubleTapZoom)
    , mMinZoom(other.mMinZoom)
    , mMaxZoom(other.mMaxZoom)
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
