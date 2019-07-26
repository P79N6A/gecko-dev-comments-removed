









#ifndef nsPresState_h_
#define nsPresState_h_

#include "nsPoint.h"
#include "gfxPoint.h"
#include "nsAutoPtr.h"

class nsPresState
{
public:
  nsPresState()
    : mContentData(nullptr)
    , mScrollState(0, 0)
    , mResolution(1.0, 1.0)
    , mDisabledSet(false)
    , mDisabled(false)
  {}

  void SetScrollState(const nsPoint& aState)
  {
    mScrollState = aState;
  }

  nsPoint GetScrollState() const
  {
    return mScrollState;
  }

  void SetResolution(const gfxSize& aSize)
  {
    mResolution = aSize;
  }

  gfxSize GetResolution() const
  {
    return mResolution;
  }

  void ClearNonScrollState()
  {
    mContentData = nullptr;
    mDisabledSet = false;
  }

  bool GetDisabled() const
  {
    return mDisabled;
  }

  void SetDisabled(bool aDisabled)
  {
    mDisabled = aDisabled;
    mDisabledSet = true;
  }

  bool IsDisabledSet() const
  {
    return mDisabledSet;
  }

  nsISupports* GetStateProperty() const
  {
    return mContentData;
  }

  void SetStateProperty(nsISupports *aProperty)
  {
    mContentData = aProperty;
  }


protected:
  nsCOMPtr<nsISupports> mContentData;
  nsPoint mScrollState;
  gfxSize mResolution;
  bool mDisabledSet;
  bool mDisabled;
};

#endif 
