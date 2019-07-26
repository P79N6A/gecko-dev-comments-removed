




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
namespace layers {




struct ParentLayerPixel {};

typedef gfx::ScaleFactor<LayoutDevicePixel, ParentLayerPixel> LayoutDeviceToParentLayerScale;
typedef gfx::ScaleFactor<ParentLayerPixel, LayerPixel> ParentLayerToLayerScale;

typedef gfx::ScaleFactor<ParentLayerPixel, ScreenPixel> ParentLayerToScreenScale;







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
    , mScrollOffset(0, 0)
    , mScrollId(NULL_SCROLL_ID)
    , mScrollableRect(0, 0, 0, 0)
    , mResolution(1)
    , mCumulativeResolution(1)
    , mZoom(1)
    , mDevPixelsPerCSSPixel(1)
    , mPresShellId(-1)
    , mMayHaveTouchListeners(false)
    , mIsRoot(false)
    , mHasScrollgrab(false)
    , mDisableScrollingX(false)
    , mDisableScrollingY(false)
    , mUpdateScrollOffset(false)
    , mScrollGeneration(0)
  {}

  

  bool operator==(const FrameMetrics& aOther) const
  {
    
    
    return mCompositionBounds.IsEqualEdges(aOther.mCompositionBounds) &&
           mDisplayPort.IsEqualEdges(aOther.mDisplayPort) &&
           mCriticalDisplayPort.IsEqualEdges(aOther.mCriticalDisplayPort) &&
           mViewport.IsEqualEdges(aOther.mViewport) &&
           mScrollOffset == aOther.mScrollOffset &&
           mScrollId == aOther.mScrollId &&
           mScrollableRect.IsEqualEdges(aOther.mScrollableRect) &&
           mResolution == aOther.mResolution &&
           mCumulativeResolution == aOther.mCumulativeResolution &&
           mDevPixelsPerCSSPixel == aOther.mDevPixelsPerCSSPixel &&
           mMayHaveTouchListeners == aOther.mMayHaveTouchListeners &&
           mPresShellId == aOther.mPresShellId &&
           mIsRoot == aOther.mIsRoot &&
           mHasScrollgrab == aOther.mHasScrollgrab &&
           mDisableScrollingX == aOther.mDisableScrollingX &&
           mDisableScrollingY == aOther.mDisableScrollingY &&
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
    return mScrollOffset * LayersPixelsPerCSSPixel();
  }

  LayoutDeviceToParentLayerScale GetParentResolution() const
  {
    return mCumulativeResolution / mResolution;
  }

  
  
  
  
  
  
  CSSRect GetExpandedScrollableRect() const
  {
    CSSRect scrollableRect = mScrollableRect;
    CSSRect compBounds = CalculateCompositedRectInCssPixels();
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

  CSSRect CalculateCompositedRectInCssPixels() const
  {
    return CSSRect(gfx::RoundedIn(mCompositionBounds / mZoom));
  }

  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  ScreenIntRect mCompositionBounds;

  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  CSSRect mDisplayPort;

  
  
  
  
  
  
  CSSRect mCriticalDisplayPort;

  
  
  
  
  
  
  
  
  
  CSSRect mViewport;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  CSSPoint mScrollOffset;

  
  ViewID mScrollId;

  
  
  
  
  
  
  
  
  
  
  
  CSSRect mScrollableRect;

  
  
  

  
  
  
  ParentLayerToLayerScale mResolution;

  
  
  
  LayoutDeviceToLayerScale mCumulativeResolution;

  
  
  
  
  CSSToScreenScale mZoom;

  
  
  
  
  CSSToLayoutDeviceScale mDevPixelsPerCSSPixel;

  uint32_t mPresShellId;

  
  bool mMayHaveTouchListeners;

  
  bool mIsRoot;

  
  bool mHasScrollgrab;

public:
  bool GetDisableScrollingX() const
  {
    return mDisableScrollingX;
  }

  void SetDisableScrollingX(bool aDisableScrollingX)
  {
    mDisableScrollingX = aDisableScrollingX;
  }

  bool GetDisableScrollingY() const
  {
    return mDisableScrollingY;
  }

  void SetDisableScrollingY(bool aDisableScrollingY)
  {
    mDisableScrollingY = aDisableScrollingY;
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

private:
  
  

  
  
  bool mDisableScrollingX;
  bool mDisableScrollingY;

  
  
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
    , mScrollId(aMetrics.mScrollId)
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
