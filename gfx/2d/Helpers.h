




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

} 
} 

#endif 
