




#ifndef GFX_FRAMEMETRICS_H
#define GFX_FRAMEMETRICS_H

#include <stdint.h>                     
#include "Units.h"                      
#include "mozilla/Maybe.h"
#include "mozilla/gfx/BasePoint.h"      
#include "mozilla/gfx/Rect.h"           
#include "mozilla/gfx/ScaleFactor.h"    
#include "mozilla/gfx/Logging.h"        
#include "gfxColor.h"
#include "gfxPrefs.h"                   
#include "nsString.h"

namespace IPC {
template <typename T> struct ParamTraits;
} 

namespace mozilla {
namespace layers {







struct FrameMetrics {
  friend struct IPC::ParamTraits<mozilla::layers::FrameMetrics>;
public:
  
  typedef uint64_t ViewID;
  static const ViewID NULL_SCROLL_ID;   
  static const ViewID START_SCROLL_ID = 2;  
                                        
  static const FrameMetrics sNullMetrics;   

  FrameMetrics()
    : mPresShellResolution(1)
    , mCompositionBounds(0, 0, 0, 0)
    , mDisplayPort(0, 0, 0, 0)
    , mCriticalDisplayPort(0, 0, 0, 0)
    , mScrollableRect(0, 0, 0, 0)
    , mCumulativeResolution()
    , mDevPixelsPerCSSPixel(1)
    , mIsRootContent(false)
    , mHasScrollgrab(false)
    , mScrollId(NULL_SCROLL_ID)
    , mScrollParentId(NULL_SCROLL_ID)
    , mScrollOffset(0, 0)
    , mZoom()
    , mUpdateScrollOffset(false)
    , mScrollGeneration(0)
    , mDoSmoothScroll(false)
    , mSmoothScrollOffset(0, 0)
    , mRootCompositionSize(0, 0)
    , mDisplayPortMargins(0, 0, 0, 0)
    , mUseDisplayPortMargins(false)
    , mPresShellId(-1)
    , mViewport(0, 0, 0, 0)
    , mExtraResolution()
    , mBackgroundColor(0, 0, 0, 0)
    , mLineScrollAmount(0, 0)
    , mPageScrollAmount(0, 0)
    , mAllowVerticalScrollWithWheel(false)
    , mIsLayersIdRoot(false)
    , mUsesContainerScrolling(false)
  {
  }

  

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
           mPresShellResolution == aOther.mPresShellResolution &&
           mCumulativeResolution == aOther.mCumulativeResolution &&
           mDevPixelsPerCSSPixel == aOther.mDevPixelsPerCSSPixel &&
           mPresShellId == aOther.mPresShellId &&
           mIsRootContent == aOther.mIsRootContent &&
           mScrollId == aOther.mScrollId &&
           mScrollParentId == aOther.mScrollParentId &&
           mScrollOffset == aOther.mScrollOffset &&
           mSmoothScrollOffset == aOther.mSmoothScrollOffset &&
           mHasScrollgrab == aOther.mHasScrollgrab &&
           mUpdateScrollOffset == aOther.mUpdateScrollOffset &&
           mScrollGeneration == aOther.mScrollGeneration &&
           mExtraResolution == aOther.mExtraResolution &&
           mBackgroundColor == aOther.mBackgroundColor &&
           mDoSmoothScroll == aOther.mDoSmoothScroll &&
           mLineScrollAmount == aOther.mLineScrollAmount &&
           mPageScrollAmount == aOther.mPageScrollAmount &&
           mAllowVerticalScrollWithWheel == aOther.mAllowVerticalScrollWithWheel &&
           mClipRect == aOther.mClipRect &&
           mIsLayersIdRoot == aOther.mIsLayersIdRoot &&
		   mUsesContainerScrolling == aOther.mUsesContainerScrolling;
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
    return mIsRootContent;
  }

  bool IsScrollable() const
  {
    return mScrollId != NULL_SCROLL_ID;
  }

  CSSToScreenScale2D DisplayportPixelsPerCSSPixel() const
  {
    
    
    
    
    
    
    
    return mZoom * ParentLayerToLayerScale(1.0f) / mExtraResolution;
  }

  CSSToLayerScale2D LayersPixelsPerCSSPixel() const
  {
    return mDevPixelsPerCSSPixel * mCumulativeResolution;
  }

  
  LayerToParentLayerScale GetAsyncZoom() const
  {
    
    
    return (mZoom / LayersPixelsPerCSSPixel()).ToScaleFactor();
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

  CSSSize CalculateCompositedSizeInCssPixels() const
  {
    return mCompositionBounds.Size() / GetZoom();
  }

  CSSRect CalculateCompositedRectInCssPixels() const
  {
    return mCompositionBounds / GetZoom();
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

  void ZoomBy(float aScale)
  {
    ZoomBy(gfxSize(aScale, aScale));
  }

  void ZoomBy(const gfxSize& aScale)
  {
    mZoom.xScale *= aScale.width;
    mZoom.yScale *= aScale.height;
  }

  void CopyScrollInfoFrom(const FrameMetrics& aOther)
  {
    mScrollOffset = aOther.mScrollOffset;
    mScrollGeneration = aOther.mScrollGeneration;
  }

  void CopySmoothScrollInfoFrom(const FrameMetrics& aOther)
  {
    mSmoothScrollOffset = aOther.mSmoothScrollOffset;
    mScrollGeneration = aOther.mScrollGeneration;
    mDoSmoothScroll = aOther.mDoSmoothScroll;
  }

  
  
  
  FrameMetrics MakePODObject() const
  {
    FrameMetrics copy = *this;
    copy.mContentDescription.Truncate();
    return copy;
  }

public:
  void SetPresShellResolution(float aPresShellResolution)
  {
    mPresShellResolution = aPresShellResolution;
  }

  float GetPresShellResolution() const
  {
    return mPresShellResolution;
  }

  void SetCompositionBounds(const ParentLayerRect& aCompositionBounds)
  {
    mCompositionBounds = aCompositionBounds;
  }

  const ParentLayerRect& GetCompositionBounds() const
  {
    return mCompositionBounds;
  }

  void SetDisplayPort(const CSSRect& aDisplayPort)
  {
    mDisplayPort = aDisplayPort;
  }

  const CSSRect& GetDisplayPort() const
  {
    return mDisplayPort;
  }

  void SetCriticalDisplayPort(const CSSRect& aCriticalDisplayPort)
  {
    mCriticalDisplayPort = aCriticalDisplayPort;
  }

  const CSSRect& GetCriticalDisplayPort() const
  {
    return mCriticalDisplayPort;
  }

  void SetCumulativeResolution(const LayoutDeviceToLayerScale2D& aCumulativeResolution)
  {
    mCumulativeResolution = aCumulativeResolution;
  }

  const LayoutDeviceToLayerScale2D& GetCumulativeResolution() const
  {
    return mCumulativeResolution;
  }

  void SetDevPixelsPerCSSPixel(const CSSToLayoutDeviceScale& aDevPixelsPerCSSPixel)
  {
    mDevPixelsPerCSSPixel = aDevPixelsPerCSSPixel;
  }

  const CSSToLayoutDeviceScale& GetDevPixelsPerCSSPixel() const
  {
    return mDevPixelsPerCSSPixel;
  }

  void SetIsRootContent(bool aIsRootContent)
  {
    mIsRootContent = aIsRootContent;
  }

  bool IsRootContent() const
  {
    return mIsRootContent;
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

  void SetSmoothScrollOffset(const CSSPoint& aSmoothScrollDestination)
  {
    mSmoothScrollOffset = aSmoothScrollDestination;
  }

  const CSSPoint& GetSmoothScrollOffset() const
  {
    return mSmoothScrollOffset;
  }

  void SetZoom(const CSSToParentLayerScale2D& aZoom)
  {
    mZoom = aZoom;
  }

  const CSSToParentLayerScale2D& GetZoom() const
  {
    return mZoom;
  }

  void SetScrollOffsetUpdated(uint32_t aScrollGeneration)
  {
    mUpdateScrollOffset = true;
    mScrollGeneration = aScrollGeneration;
  }

  void SetSmoothScrollOffsetUpdated(int32_t aScrollGeneration)
  {
    mDoSmoothScroll = true;
    mScrollGeneration = aScrollGeneration;
  }

  bool GetScrollOffsetUpdated() const
  {
    return mUpdateScrollOffset;
  }

  bool GetDoSmoothScroll() const
  {
    return mDoSmoothScroll;
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

  void SetDisplayPortMargins(const ScreenMargin& aDisplayPortMargins)
  {
    mDisplayPortMargins = aDisplayPortMargins;
  }

  const ScreenMargin& GetDisplayPortMargins() const
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

  void SetExtraResolution(const ScreenToLayerScale2D& aExtraResolution)
  {
    mExtraResolution = aExtraResolution;
  }

  const ScreenToLayerScale2D& GetExtraResolution() const
  {
    return mExtraResolution;
  }

  const gfxRGBA& GetBackgroundColor() const
  {
    return mBackgroundColor;
  }

  void SetBackgroundColor(const gfxRGBA& aBackgroundColor)
  {
    mBackgroundColor = aBackgroundColor;
  }

  const nsCString& GetContentDescription() const
  {
    return mContentDescription;
  }

  void SetContentDescription(const nsCString& aContentDescription)
  {
    mContentDescription = aContentDescription;
  }

  const LayoutDeviceIntSize& GetLineScrollAmount() const
  {
    return mLineScrollAmount;
  }

  void SetLineScrollAmount(const LayoutDeviceIntSize& size)
  {
    mLineScrollAmount = size;
  }

  const LayoutDeviceIntSize& GetPageScrollAmount() const
  {
    return mPageScrollAmount;
  }

  void SetPageScrollAmount(const LayoutDeviceIntSize& size)
  {
    mPageScrollAmount = size;
  }

  const CSSRect& GetScrollableRect() const
  {
    return mScrollableRect;
  }

  void SetScrollableRect(const CSSRect& aScrollableRect)
  {
    mScrollableRect = aScrollableRect;
  }

  bool AllowVerticalScrollWithWheel() const
  {
    return mAllowVerticalScrollWithWheel;
  }

  void SetAllowVerticalScrollWithWheel()
  {
    mAllowVerticalScrollWithWheel = true;
  }

  void SetClipRect(const Maybe<ParentLayerIntRect>& aClipRect)
  {
    mClipRect = aClipRect;
  }
  const Maybe<ParentLayerIntRect>& GetClipRect() const
  {
    return mClipRect;
  }
  bool HasClipRect() const {
    return mClipRect.isSome();
  }
  const ParentLayerIntRect& ClipRect() const {
    return mClipRect.ref();
  }

  void SetIsLayersIdRoot(bool aValue) {
    mIsLayersIdRoot = aValue;
  }
  bool IsLayersIdRoot() const {
    return mIsLayersIdRoot;
  }

  void SetUsesContainerScrolling(bool aValue) {
    MOZ_ASSERT_IF(aValue, gfxPrefs::LayoutUseContainersForRootFrames());
    mUsesContainerScrolling = aValue;
  }
  bool UsesContainerScrolling() const {
    return mUsesContainerScrolling;
  }

private:

  
  
  
  
  
  
  
  float mPresShellResolution;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  ParentLayerRect mCompositionBounds;

  
  
  
  
  
  
  
  
  
  
  
  CSSRect mDisplayPort;

  
  
  
  
  
  
  CSSRect mCriticalDisplayPort;

  
  
  
  
  
  
  
  
  
  
  
  CSSRect mScrollableRect;

  
  
  
  
  
  
  
  LayoutDeviceToLayerScale2D mCumulativeResolution;

  
  

  
  
  
  
  CSSToLayoutDeviceScale mDevPixelsPerCSSPixel;

  
  bool mIsRootContent;

  
  bool mHasScrollgrab;

  
  ViewID mScrollId;

  
  ViewID mScrollParentId;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  CSSPoint mScrollOffset;

  
  
  
  
  CSSToParentLayerScale2D mZoom;

  
  
  bool mUpdateScrollOffset;
  
  uint32_t mScrollGeneration;

  
  
  bool mDoSmoothScroll;
  CSSPoint mSmoothScrollOffset;

  
  CSSSize mRootCompositionSize;

  
  
  ScreenMargin mDisplayPortMargins;

  
  
  bool mUseDisplayPortMargins;

  uint32_t mPresShellId;

  
  
  
  
  
  
  
  
  
  CSSRect mViewport;

  
  
  ScreenToLayerScale2D mExtraResolution;

  
  gfxRGBA mBackgroundColor;

  
  
  
  nsCString mContentDescription;

  
  LayoutDeviceIntSize mLineScrollAmount;

  
  LayoutDeviceIntSize mPageScrollAmount;

  
  bool mAllowVerticalScrollWithWheel;

  
  Maybe<ParentLayerIntRect> mClipRect;

  
  
  bool mIsLayersIdRoot;

  
  
  bool mUsesContainerScrolling;

  
  
  
  
  
  
  
  
  
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
  }

  ScrollableLayerGuid(uint64_t aLayersId, uint32_t aPresShellId,
                      FrameMetrics::ViewID aScrollId)
    : mLayersId(aLayersId)
    , mPresShellId(aPresShellId)
    , mScrollId(aScrollId)
  {
  }

  ScrollableLayerGuid(uint64_t aLayersId, const FrameMetrics& aMetrics)
    : mLayersId(aLayersId)
    , mPresShellId(aMetrics.GetPresShellId())
    , mScrollId(aMetrics.GetScrollId())
  {
  }

  ScrollableLayerGuid(const ScrollableLayerGuid& other)
    : mLayersId(other.mLayersId)
    , mPresShellId(other.mPresShellId)
    , mScrollId(other.mScrollId)
  {
  }

  ~ScrollableLayerGuid()
  {
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
  CSSToParentLayerScale mMinZoom;
  CSSToParentLayerScale mMaxZoom;

  ZoomConstraints()
    : mAllowZoom(true)
    , mAllowDoubleTapZoom(true)
  {
    MOZ_COUNT_CTOR(ZoomConstraints);
  }

  ZoomConstraints(bool aAllowZoom,
                  bool aAllowDoubleTapZoom,
                  const CSSToParentLayerScale& aMinZoom,
                  const CSSToParentLayerScale& aMaxZoom)
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
