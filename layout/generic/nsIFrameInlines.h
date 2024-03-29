





#ifndef nsIFrameInlines_h___
#define nsIFrameInlines_h___

#include "nsContainerFrame.h"
#include "nsStyleStructInlines.h"
#include "nsCSSAnonBoxes.h"

bool
nsIFrame::IsFlexItem() const
{
  return GetParent() &&
    GetParent()->GetType() == nsGkAtoms::flexContainerFrame &&
    !(GetStateBits() & NS_FRAME_OUT_OF_FLOW);
}

bool
nsIFrame::IsFlexOrGridItem() const
{
  if (GetParent()) {
    nsIAtom* t = GetParent()->GetType();
    return (t == nsGkAtoms::flexContainerFrame ||
            t == nsGkAtoms::gridContainerFrame) &&
      !(GetStateBits() & NS_FRAME_OUT_OF_FLOW);
  }
  return false;
}

bool
nsIFrame::IsTableCaption() const
{
  return StyleDisplay()->mDisplay == NS_STYLE_DISPLAY_TABLE_CAPTION &&
    GetParent()->StyleContext()->GetPseudo() == nsCSSAnonBoxes::tableOuter;
}

bool
nsIFrame::IsFloating() const
{
  return StyleDisplay()->IsFloating(this);
}

bool
nsIFrame::IsAbsPosContaininingBlock() const
{
  return StyleDisplay()->IsAbsPosContainingBlock(this);
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
