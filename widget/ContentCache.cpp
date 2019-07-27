






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

} 
