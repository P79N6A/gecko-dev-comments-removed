









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
    , mResolution(1.0)
    , mScaleToResolution(false)
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

  void SetResolution(float aSize)
  {
    mResolution = aSize;
  }

  float GetResolution() const
  {
    return mResolution;
  }

  void SetScaleToResolution(bool aScaleToResolution)
  {
    mScaleToResolution = aScaleToResolution;
  }

  bool GetScaleToResolution() const
  {
    return mScaleToResolution;
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
  float mResolution;
  bool mScaleToResolution;
  bool mDisabledSet;
  bool mDisabled;
};

#endif 
