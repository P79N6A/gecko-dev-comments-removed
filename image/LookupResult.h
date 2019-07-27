









#ifndef mozilla_image_LookupResult_h
#define mozilla_image_LookupResult_h

#include "mozilla/Attributes.h"
#include "mozilla/Move.h"
#include "imgFrame.h"

namespace mozilla {
namespace image {





class MOZ_STACK_CLASS LookupResult
{
public:
  LookupResult()
    : mIsExactMatch(false)
  { }

  LookupResult(DrawableFrameRef&& aDrawableRef, bool aIsExactMatch)
    : mDrawableRef(Move(aDrawableRef))
    , mIsExactMatch(aIsExactMatch)
  { }

  DrawableFrameRef& DrawableRef() { return mDrawableRef; }
  const DrawableFrameRef& DrawableRef() const { return mDrawableRef; }

  
  explicit operator bool() const { return bool(mDrawableRef); }

  
  bool IsExactMatch() const { return mIsExactMatch; }

private:
  DrawableFrameRef mDrawableRef;
  bool mIsExactMatch;
};

} 
} 

#endif 
