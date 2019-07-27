




#ifndef MOZILLA_GFX_2D_HELPERS_H_
#define MOZILLA_GFX_2D_HELPERS_H_

#include "2D.h"

namespace mozilla {
namespace gfx {

class AutoSaveTransform
{
 public:
  explicit AutoSaveTransform(DrawTarget *aTarget)
   : mDrawTarget(aTarget),
     mOldTransform(aTarget->GetTransform())
  {
  }
  ~AutoSaveTransform()
  {
    mDrawTarget->SetTransform(mOldTransform);
  }

 private:
  RefPtr<DrawTarget> mDrawTarget;
  Matrix mOldTransform;
};

} 
} 

#endif 
