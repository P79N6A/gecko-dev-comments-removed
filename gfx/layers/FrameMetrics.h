




#ifndef GFX_FRAMEMETRICS_H
#define GFX_FRAMEMETRICS_H

#include "gfxPoint.h"
#include "gfxTypes.h"
#include "nsRect.h"
#include "mozilla/gfx/Rect.h"

namespace mozilla {
namespace layers {







struct THEBES_API FrameMetrics {
public:
  
  typedef PRUint64 ViewID;
  static const ViewID NULL_SCROLL_ID;   
  static const ViewID ROOT_SCROLL_ID;   
  static const ViewID START_SCROLL_ID;  
                                        

  FrameMetrics()
    : mViewport(0, 0, 0, 0)
    , mContentRect(0, 0, 0, 0)
    , mViewportScrollOffset(0, 0)
    , mScrollId(NULL_SCROLL_ID)
    , mCSSContentRect(0, 0, 0, 0)
    , mResolution(1, 1)
    , mMayHaveTouchListeners(false)
  {}

  

  bool operator==(const FrameMetrics& aOther) const
  {
    return (mViewport.IsEqualEdges(aOther.mViewport) &&
            mViewportScrollOffset == aOther.mViewportScrollOffset &&
            mDisplayPort.IsEqualEdges(aOther.mDisplayPort) &&
            mScrollId == aOther.mScrollId);
  }
  bool operator!=(const FrameMetrics& aOther) const
  {
    return !operator==(aOther);
  }

  bool IsDefault() const
  {
    return (FrameMetrics() == *this);
  }

  bool IsRootScrollable() const
  {
    return mScrollId == ROOT_SCROLL_ID;
  }

  bool IsScrollable() const
  {
    return mScrollId != NULL_SCROLL_ID;
  }

  
  nsIntRect mViewport;
  nsIntRect mContentRect;
  nsIntPoint mViewportScrollOffset;
  nsIntRect mDisplayPort;
  ViewID mScrollId;

  
  
  gfx::Rect mCSSContentRect;

  
  
  gfxSize mResolution;

  
  bool mMayHaveTouchListeners;
};

}
}

#endif 
