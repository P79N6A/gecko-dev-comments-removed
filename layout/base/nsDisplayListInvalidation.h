




#ifndef NSDISPLAYLISTINVALIDATION_H_
#define NSDISPLAYLISTINVALIDATION_H_

#include "mozilla/Attributes.h"
#include "FrameLayerBuilder.h"
#include "imgIContainer.h"
#include "nsRect.h"
#include "nsColor.h"
#include "gfxRect.h"

class nsDisplayBackgroundImage;
class nsCharClipDisplayItem;
class nsDisplayItem;
class nsDisplayListBuilder;
class nsDisplaySVGEffects;
class nsDisplayTableItem;
class nsDisplayThemedBackground;








class nsDisplayItemGeometry
{
public:
  nsDisplayItemGeometry(nsDisplayItem* aItem, nsDisplayListBuilder* aBuilder);
  virtual ~nsDisplayItemGeometry();
  
  



  const nsRect& ComputeInvalidationRegion() { return mBounds; }
  
  







  virtual void MoveBy(const nsPoint& aOffset)
  {
    mBounds.MoveBy(aOffset);
  }

  


  nsRect mBounds;
};







class nsDisplayItemGenericGeometry : public nsDisplayItemGeometry
{
public:
  nsDisplayItemGenericGeometry(nsDisplayItem* aItem, nsDisplayListBuilder* aBuilder);

  virtual void MoveBy(const nsPoint& aOffset) override;

  nsRect mBorderRect;
};

bool ShouldSyncDecodeImages(nsDisplayListBuilder* aBuilder);










template <typename T>
class nsImageGeometryMixin
{
public:
  nsImageGeometryMixin(nsDisplayItem* aItem, nsDisplayListBuilder* aBuilder)
    : mLastDrawResult(mozilla::image::DrawResult::NOT_READY)
    , mWaitingForPaint(false)
  {
    
    auto lastGeometry =
      static_cast<T*>(mozilla::FrameLayerBuilder::GetMostRecentGeometry(aItem));
    if (lastGeometry) {
      mLastDrawResult = lastGeometry->mLastDrawResult;
      mWaitingForPaint = lastGeometry->mWaitingForPaint;
    }

    
    
    
    if (ShouldSyncDecodeImages(aBuilder) &&
        ShouldInvalidateToSyncDecodeImages()) {
      mWaitingForPaint = true;
    }
  }

  static void UpdateDrawResult(nsDisplayItem* aItem,
                               mozilla::image::DrawResult aResult)
  {
    auto lastGeometry =
      static_cast<T*>(mozilla::FrameLayerBuilder::GetMostRecentGeometry(aItem));
    if (lastGeometry) {
      lastGeometry->mLastDrawResult = aResult;
      lastGeometry->mWaitingForPaint = false;
    }
  }

  bool ShouldInvalidateToSyncDecodeImages() const
  {
    if (mWaitingForPaint) {
      
      
      
      
      
      return false;
    }

    if (mLastDrawResult == mozilla::image::DrawResult::SUCCESS ||
        mLastDrawResult == mozilla::image::DrawResult::BAD_IMAGE) {
      return false;
    }

    return true;
  }

private:
  mozilla::image::DrawResult mLastDrawResult;
  bool mWaitingForPaint;
};







class nsDisplayItemGenericImageGeometry
  : public nsDisplayItemGenericGeometry
  , public nsImageGeometryMixin<nsDisplayItemGenericImageGeometry>
{
public:
  nsDisplayItemGenericImageGeometry(nsDisplayItem* aItem,
                                    nsDisplayListBuilder* aBuilder)
    : nsDisplayItemGenericGeometry(aItem, aBuilder)
    , nsImageGeometryMixin(aItem, aBuilder)
  { }
};

class nsDisplayItemBoundsGeometry : public nsDisplayItemGeometry
{
public:
  nsDisplayItemBoundsGeometry(nsDisplayItem* aItem, nsDisplayListBuilder* aBuilder);

  bool mHasRoundedCorners;
};

class nsDisplayBorderGeometry : public nsDisplayItemGeometry
{
public:
  nsDisplayBorderGeometry(nsDisplayItem* aItem, nsDisplayListBuilder* aBuilder);

  virtual void MoveBy(const nsPoint& aOffset) override;

  nsRect mContentRect;
};

class nsDisplayBackgroundGeometry
  : public nsDisplayItemGeometry
  , public nsImageGeometryMixin<nsDisplayBackgroundGeometry>
{
public:
  nsDisplayBackgroundGeometry(nsDisplayBackgroundImage* aItem, nsDisplayListBuilder* aBuilder);

  virtual void MoveBy(const nsPoint& aOffset) override;

  nsRect mPositioningArea;
};

class nsDisplayThemedBackgroundGeometry : public nsDisplayItemGeometry
{
public:
  nsDisplayThemedBackgroundGeometry(nsDisplayThemedBackground* aItem, nsDisplayListBuilder* aBuilder);

  virtual void MoveBy(const nsPoint& aOffset) override;

  nsRect mPositioningArea;
  bool mWindowIsActive;
};

class nsDisplayBoxShadowInnerGeometry : public nsDisplayItemGeometry
{
public:
  nsDisplayBoxShadowInnerGeometry(nsDisplayItem* aItem, nsDisplayListBuilder* aBuilder);
  
  virtual void MoveBy(const nsPoint& aOffset) override;

  nsRect mPaddingRect;
};

class nsDisplayBoxShadowOuterGeometry : public nsDisplayItemGenericGeometry
{
public:
  nsDisplayBoxShadowOuterGeometry(nsDisplayItem* aItem,
                                  nsDisplayListBuilder* aBuilder,
                                  float aOpacity);

  float mOpacity;
};

class nsDisplaySolidColorGeometry : public nsDisplayItemBoundsGeometry
{
public:
  nsDisplaySolidColorGeometry(nsDisplayItem* aItem,
                              nsDisplayListBuilder* aBuilder,
                              nscolor aColor)
    : nsDisplayItemBoundsGeometry(aItem, aBuilder)
    , mColor(aColor)
  { }

  nscolor mColor;
};

class nsDisplaySVGEffectsGeometry : public nsDisplayItemGeometry
{
public:
  nsDisplaySVGEffectsGeometry(nsDisplaySVGEffects* aItem, nsDisplayListBuilder* aBuilder);

  virtual void MoveBy(const nsPoint& aOffset) override;

  gfxRect mBBox;
  gfxPoint mUserSpaceOffset;
  nsPoint mFrameOffsetToReferenceFrame;
};

class nsCharClipGeometry : public nsDisplayItemGenericGeometry
{
public:
  nsCharClipGeometry(nsCharClipDisplayItem* aItem, nsDisplayListBuilder* aBuilder);

  nscoord mVisIStartEdge;
  nscoord mVisIEndEdge;
};

class nsDisplayTableItemGeometry
  : public nsDisplayItemGenericGeometry
  , public nsImageGeometryMixin<nsDisplayTableItemGeometry>
{
public:
  nsDisplayTableItemGeometry(nsDisplayTableItem* aItem,
                             nsDisplayListBuilder* aBuilder,
                             const nsPoint& aFrameOffsetToViewport);

  nsPoint mFrameOffsetToViewport;
};

#endif 
