





#ifndef nsIFrameInlines_h___
#define nsIFrameInlines_h___

#include "nsIFrame.h"
#include "nsStyleStructInlines.h"

bool
nsIFrame::IsFloating() const
{
  return StyleDisplay()->IsFloating(this);
}

bool
nsIFrame::IsPositioned() const
{
  return StyleDisplay()->IsPositioned(this);
}

bool
nsIFrame::IsRelativelyPositioned() const
{
  return StyleDisplay()->IsRelativelyPositioned(this);
}

bool
nsIFrame::IsAbsolutelyPositioned() const
{
  return StyleDisplay()->IsAbsolutelyPositioned(this);
}

bool
nsIFrame::IsBlockInside() const
{
  return StyleDisplay()->IsBlockInside(this);
}

bool
nsIFrame::IsBlockOutside() const
{
  return StyleDisplay()->IsBlockOutside(this);
}

bool
nsIFrame::IsInlineOutside() const
{
  return StyleDisplay()->IsInlineOutside(this);
}

uint8_t
nsIFrame::GetDisplay() const
{
  return StyleDisplay()->GetDisplay(this);
}

#endif
