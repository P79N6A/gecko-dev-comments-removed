





#include "TextRange.h"

#include "HyperTextAccessible.h"

using namespace mozilla::a11y;

TextRange::TextRange(HyperTextAccessible* aRoot,
                     Accessible* aStartContainer, int32_t aStartOffset,
                     Accessible* aEndContainer, int32_t aEndOffset) :
  mRoot(aRoot), mStartContainer(aStartContainer), mEndContainer(aEndContainer),
  mStartOffset(aStartOffset), mEndOffset(aEndOffset)
{
}

void
TextRange::Text(nsAString& aText) const
{

}

void
TextRange::Set(HyperTextAccessible* aRoot,
               Accessible* aStartContainer, int32_t aStartOffset,
               Accessible* aEndContainer, int32_t aEndOffset)
{
  mRoot = aRoot;
  mStartContainer = aStartContainer;
  mEndContainer = aEndContainer;
  mStartOffset = aStartOffset;
  mEndOffset = aEndOffset;
}
