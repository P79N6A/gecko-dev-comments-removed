




#ifndef NSDISPLAYLISTINVALIDATION_H_
#define NSDISPLAYLISTINVALIDATION_H_








class nsDisplayItemGeometry
{
public:
  nsDisplayItemGeometry()
  {
    MOZ_COUNT_CTOR(nsDisplayItemGeometry);
  }
  virtual ~nsDisplayItemGeometry()
  {
    MOZ_COUNT_DTOR(nsDisplayItemGeometry);
  }
  



  nsRegion ComputeInvalidationRegion() { return mBounds; }
  
  







  virtual void MoveBy(const nsPoint& aOffset) = 0;

  


  nscoord mAppUnitsPerDevPixel;

  



  nsIntPoint mPaintOffset;

  gfxPoint mActiveScrolledRootPosition;
  
  


  nsRect mBounds;
};







class nsDisplayItemGenericGeometry : public nsDisplayItemGeometry
{
public:
  virtual void MoveBy(const nsPoint& aOffset)
  {
    mBounds.MoveBy(aOffset);
    mBorderRect.MoveBy(aOffset);
  }

  nsRect mBorderRect;
};

class nsDisplayBorderGeometry : public nsDisplayItemGeometry
{
public:
  virtual void MoveBy(const nsPoint& aOffset)
  {
    mBounds.MoveBy(aOffset);
    mPaddingRect.MoveBy(aOffset);
  }

  nsRect mPaddingRect;
};

class nsDisplayBackgroundGeometry : public nsDisplayItemGeometry
{
public:
  virtual void MoveBy(const nsPoint& aOffset)
  {
    mBounds.MoveBy(aOffset);
    mPaddingRect.MoveBy(aOffset);
    mContentRect.MoveBy(aOffset);
  }

  nsRect mPaddingRect;
  nsRect mContentRect;
};

class nsDisplayBoxShadowInnerGeometry : public nsDisplayItemGeometry
{
public:
  virtual void MoveBy(const nsPoint& aOffset)
  {
    mBounds.MoveBy(aOffset);
    mPaddingRect.MoveBy(aOffset);
  }

  nsRect mPaddingRect;
};

#endif 
