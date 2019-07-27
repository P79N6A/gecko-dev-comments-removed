






#include "mozilla/ContentCache.h"

namespace mozilla {

void
ContentCache::Clear()
{
  mText.Truncate();
}

void
ContentCache::SetText(const nsAString& aText)
{
  mText = aText;
}

void
ContentCache::SetSelection(uint32_t aAnchorOffset, uint32_t aFocusOffset)
{
  mSelection.mAnchor = aAnchorOffset;
  mSelection.mFocus = aFocusOffset;
}

} 
