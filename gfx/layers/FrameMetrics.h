




#ifndef GFX_FRAMEMETRICS_H
#define GFX_FRAMEMETRICS_H

#include <stdint.h>                     
#include "Units.h"                      
#include "mozilla/gfx/BasePoint.h"      
#include "mozilla/gfx/Rect.h"           
#include "mozilla/gfx/ScaleFactor.h"    

namespace mozilla {
namespace layers {




struct ParentLayerPixel {};

typedef gfx::ScaleFactor<LayoutDevicePixel, ParentLayerPixel> LayoutDeviceToParentLayerScale;
typedef gfx::ScaleFactor<ParentLayerPixel, LayerPixel> ParentLayerToLayerScale;

typedef gfx::ScaleFactor<ParentLayerPixel, ScreenPixel> ParentLayerToScreenScale;







struct FrameMetrics {
public:
  
  typedef uint64_t ViewID;
  static const ViewID NULL_SCROLL_ID;   
  static const ViewID ROOT_SCROLL_ID;   
  static const ViewID START_SCROLL_ID;  
                                        

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
    , mMayHaveTouchListeners(false)
    , mPresShellId(-1)
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
           mPresShellId == aOther.mPresShellId;
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
    return mScrollId == ROOT_SCROLL_ID;
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

  
  bool mMayHaveTouchListeners;

  uint32_t mPresShellId;
};

}
}

#endif 
