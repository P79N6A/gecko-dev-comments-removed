









#ifndef mozilla_image_LookupResult_h
#define mozilla_image_LookupResult_h

#include "mozilla/Attributes.h"
#include "mozilla/Move.h"
#include "imgFrame.h"

namespace mozilla {
namespace image {

enum class MatchType : uint8_t
{
  NOT_FOUND,  
  PENDING,    
  EXACT,      
  SUBSTITUTE_BECAUSE_NOT_FOUND,  
  SUBSTITUTE_BECAUSE_PENDING     
                                 
};





class MOZ_STACK_CLASS LookupResult
{
public:
  explicit LookupResult(MatchType aMatchType)
    : mMatchType(aMatchType)
  {
    MOZ_ASSERT(mMatchType == MatchType::NOT_FOUND ||
               mMatchType == MatchType::PENDING,
               "Only NOT_FOUND or PENDING make sense with no surface");
  }

  LookupResult(LookupResult&& aOther)
    : mDrawableRef(Move(aOther.mDrawableRef))
    , mMatchType(aOther.mMatchType)
  { }

  LookupResult(DrawableFrameRef&& aDrawableRef, MatchType aMatchType)
    : mDrawableRef(Move(aDrawableRef))
    , mMatchType(aMatchType)
  {
    MOZ_ASSERT(!mDrawableRef || !(mMatchType == MatchType::NOT_FOUND ||
                                  mMatchType == MatchType::PENDING),
               "Only NOT_FOUND or PENDING make sense with no surface");
    MOZ_ASSERT(mDrawableRef || mMatchType == MatchType::NOT_FOUND ||
                               mMatchType == MatchType::PENDING,
               "NOT_FOUND or PENDING do not make sense with a surface");
  }

  LookupResult& operator=(LookupResult&& aOther)
  {
    MOZ_ASSERT(&aOther != this, "Self-move-assignment is not supported");
    mDrawableRef = Move(aOther.mDrawableRef);
    mMatchType = aOther.mMatchType;
    return *this;
  }

  DrawableFrameRef& DrawableRef() { return mDrawableRef; }
  const DrawableFrameRef& DrawableRef() const { return mDrawableRef; }

  
  explicit operator bool() const { return bool(mDrawableRef); }

  
  MatchType Type() const { return mMatchType; }

private:
  LookupResult(const LookupResult&) = delete;

  DrawableFrameRef mDrawableRef;
  MatchType mMatchType;
};

} 
} 

#endif
