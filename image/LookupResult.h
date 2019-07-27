









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

  LookupResult(LookupResult&& aOther)
    : mDrawableRef(Move(aOther.mDrawableRef))
    , mIsExactMatch(aOther.mIsExactMatch)
  { }

  LookupResult(DrawableFrameRef&& aDrawableRef, bool aIsExactMatch)
    : mDrawableRef(Move(aDrawableRef))
    , mIsExactMatch(aIsExactMatch)
  { }

  LookupResult& operator=(LookupResult&& aOther)
  {
    MOZ_ASSERT(&aOther != this, "Self-move-assignment is not supported");
    mDrawableRef = Move(aOther.mDrawableRef);
    mIsExactMatch = aOther.mIsExactMatch;
    return *this;
  }

  DrawableFrameRef& DrawableRef() { return mDrawableRef; }
  const DrawableFrameRef& DrawableRef() const { return mDrawableRef; }

  
  explicit operator bool() const { return bool(mDrawableRef); }

  
  bool IsExactMatch() const { return mIsExactMatch; }

private:
  LookupResult(const LookupResult&) = delete;

  DrawableFrameRef mDrawableRef;
  bool mIsExactMatch;
};

} 
} 

#endif 
