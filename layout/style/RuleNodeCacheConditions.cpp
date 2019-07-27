











#include "RuleNodeCacheConditions.h"

#include "nsStyleContext.h"
#include "WritingModes.h"

using namespace mozilla;

bool
RuleNodeCacheConditions::Matches(nsStyleContext* aStyleContext) const
{
  MOZ_ASSERT(Cacheable());
  if ((mBits & eHaveFontSize) &&
      mFontSize != aStyleContext->StyleFont()->mFont.size) {
    return false;
  }
  if ((mBits & eHaveWritingMode) &&
      (GetWritingMode() != WritingMode(aStyleContext).GetBits())) {
    return false;
  }
  return true;
}

#ifdef DEBUG
void
RuleNodeCacheConditions::List() const
{
  printf("{ ");
  bool first = true;
  if (mBits & eHaveFontSize) {
    printf("FontSize(%d)", mFontSize);
    first = false;
  }
  if (mBits & eHaveWritingMode) {
    if (!first) {
      printf(", ");
    }
    printf("WritingMode(0x%x)", GetWritingMode());
  }
  printf(" }");
}
#endif
