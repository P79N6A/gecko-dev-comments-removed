




#ifndef NSDISPLAYLISTINVALIDATION_H_
#define NSDISPLAYLISTINVALIDATION_H_

#include "nsRect.h"

class nsDisplayItem;
class nsDisplayListBuilder;
class nsDisplayBackgroundImage;








class nsDisplayItemGeometry
{
public:
  nsDisplayItemGeometry(nsDisplayItem* aItem, nsDisplayListBuilder* aBuilder);
  virtual ~nsDisplayItemGeometry();
  
  



  const nsRect& ComputeInvalidationRegion() { return mBounds; }
  
  







  virtual void MoveBy(const nsPoint& aOffset) = 0;

  


  nsRect mBounds;
};







class nsDisplayItemGenericGeometry : public nsDisplayItemGeometry
{
public:
  nsDisplayItemGenericGeometry(nsDisplayItem* aItem, nsDisplayListBuilder* aBuilder);

  virtual void MoveBy(const nsPoint& aOffset);

  nsRect mBorderRect;
};

class nsDisplayItemBoundsGeometry : public nsDisplayItemGeometry
{
public:
  nsDisplayItemBoundsGeometry(nsDisplayItem* aItem, nsDisplayListBuilder* aBuilder);

  virtual void MoveBy(const nsPoint& aOffset);

  bool mHasRoundedCorners;
};

class nsDisplayBorderGeometry : public nsDisplayItemGeometry
{
public:
  nsDisplayBorderGeometry(nsDisplayItem* aItem, nsDisplayListBuilder* aBuilder);

  virtual void MoveBy(const nsPoint& aOffset);

  nsRect mContentRect;
};

class nsDisplayBackgroundGeometry : public nsDisplayItemGeometry
{
public:
  nsDisplayBackgroundGeometry(nsDisplayBackgroundImage* aItem, nsDisplayListBuilder* aBuilder);

  virtual void MoveBy(const nsPoint& aOffset);

  nsRect mPositioningArea;
};

class nsDisplayBoxShadowInnerGeometry : public nsDisplayItemGeometry
{
public:
  nsDisplayBoxShadowInnerGeometry(nsDisplayItem* aItem, nsDisplayListBuilder* aBuilder);
  
  virtual void MoveBy(const nsPoint& aOffset);

  nsRect mPaddingRect;
};

#endif 
