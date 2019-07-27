





#ifndef mozilla_FontRange_h_
#define mozilla_FontRange_h_

namespace mozilla {

struct FontRange
{
  FontRange()
    : mStartOffset(0)
    , mFontSize(0)
  {
  }

  int32_t mStartOffset;
  nsString mFontName;
  gfxFloat mFontSize; 
};

} 

#endif 
