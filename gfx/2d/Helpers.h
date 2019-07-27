




#ifndef MOZILLA_GFX_2D_HELPERS_H_
#define MOZILLA_GFX_2D_HELPERS_H_

#include "2D.h"

namespace mozilla {
namespace gfx {

class AutoRestoreTransform
{
 public:
  AutoRestoreTransform()
  {
  }

  explicit AutoRestoreTransform(DrawTarget *aTarget)
   : mDrawTarget(aTarget),
     mOldTransform(aTarget->GetTransform())
  {
  }

  void Init(DrawTarget *aTarget)
  {
    MOZ_ASSERT(!mDrawTarget || aTarget == mDrawTarget);
    if (!mDrawTarget) {
      mDrawTarget = aTarget;
      mOldTransform = aTarget->GetTransform();
    }
  }

  ~AutoRestoreTransform()
  {
    if (mDrawTarget) {
      mDrawTarget->SetTransform(mOldTransform);
    }
  }

 private:
  RefPtr<DrawTarget> mDrawTarget;
  Matrix mOldTransform;
};

class AutoPopClips
{
public:
  explicit AutoPopClips(DrawTarget *aTarget)
    : mDrawTarget(aTarget)
    , mPushCount(0)
  {
    MOZ_ASSERT(mDrawTarget);
  }

  ~AutoPopClips()
  {
    PopAll();
  }

  void PushClip(const Path *aPath)
  {
    mDrawTarget->PushClip(aPath);
    ++mPushCount;
  }

  void PushClipRect(const Rect &aRect)
  {
    mDrawTarget->PushClipRect(aRect);
    ++mPushCount;
  }

  void PopClip()
  {
    MOZ_ASSERT(mPushCount > 0);
    mDrawTarget->PopClip();
    --mPushCount;
  }

  void PopAll()
  {
    while (mPushCount-- > 0) {
      mDrawTarget->PopClip();
    }
  }

private:
  RefPtr<DrawTarget> mDrawTarget;
  int32_t mPushCount;
};

} 
} 

#endif 
