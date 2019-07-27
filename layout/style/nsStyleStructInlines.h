









#ifndef nsStyleStructInlines_h_
#define nsStyleStructInlines_h_

#include "nsIFrame.h"
#include "nsStyleStruct.h"
#include "nsIContent.h" 

inline void
nsStyleImage::SetSubImage(uint8_t aIndex, imgIContainer* aSubImage) const
{
  const_cast<nsStyleImage*>(this)->mSubImages.ReplaceObjectAt(aSubImage, aIndex);
}

inline imgIContainer*
nsStyleImage::GetSubImage(uint8_t aIndex) const
{
  imgIContainer* subImage = nullptr;
  if (aIndex < mSubImages.Count())
    subImage = mSubImages[aIndex];
  return subImage;
}

bool
nsStyleText::HasTextShadow() const
{
  return mTextShadow;
}

nsCSSShadowArray*
nsStyleText::GetTextShadow() const
{
  return mTextShadow;
}

bool
nsStyleText::WhiteSpaceCanWrap(const nsIFrame* aContextFrame) const
{
  NS_ASSERTION(aContextFrame->StyleText() == this, "unexpected aContextFrame");
  return WhiteSpaceCanWrapStyle() && !aContextFrame->IsSVGText();
}

bool
nsStyleText::WordCanWrap(const nsIFrame* aContextFrame) const
{
  NS_ASSERTION(aContextFrame->StyleText() == this, "unexpected aContextFrame");
  return WordCanWrapStyle() && !aContextFrame->IsSVGText();
}

bool
nsStyleDisplay::IsBlockInside(const nsIFrame* aContextFrame) const
{
  NS_ASSERTION(aContextFrame->StyleDisplay() == this, "unexpected aContextFrame");
  if (aContextFrame->IsSVGText()) {
    return aContextFrame->GetType() == nsGkAtoms::blockFrame;
  }
  return IsBlockInsideStyle();
}

bool
nsStyleDisplay::IsBlockOutside(const nsIFrame* aContextFrame) const
{
  NS_ASSERTION(aContextFrame->StyleDisplay() == this, "unexpected aContextFrame");
  if (aContextFrame->IsSVGText()) {
    return aContextFrame->GetType() == nsGkAtoms::blockFrame;
  }
  return IsBlockOutsideStyle();
}

bool
nsStyleDisplay::IsInlineOutside(const nsIFrame* aContextFrame) const
{
  NS_ASSERTION(aContextFrame->StyleDisplay() == this, "unexpected aContextFrame");
  if (aContextFrame->IsSVGText()) {
    return aContextFrame->GetType() != nsGkAtoms::blockFrame;
  }
  return IsInlineOutsideStyle();
}

bool
nsStyleDisplay::IsOriginalDisplayInlineOutside(const nsIFrame* aContextFrame) const
{
  NS_ASSERTION(aContextFrame->StyleDisplay() == this, "unexpected aContextFrame");
  if (aContextFrame->IsSVGText()) {
    return aContextFrame->GetType() != nsGkAtoms::blockFrame;
  }
  return IsOriginalDisplayInlineOutsideStyle();
}

uint8_t
nsStyleDisplay::GetDisplay(const nsIFrame* aContextFrame) const
{
  NS_ASSERTION(aContextFrame->StyleDisplay() == this, "unexpected aContextFrame");
  if (aContextFrame->IsSVGText() &&
      mDisplay != NS_STYLE_DISPLAY_NONE) {
    return aContextFrame->GetType() == nsGkAtoms::blockFrame ?
             NS_STYLE_DISPLAY_BLOCK :
             NS_STYLE_DISPLAY_INLINE;
  }
  return mDisplay;
}

bool
nsStyleDisplay::IsFloating(const nsIFrame* aContextFrame) const
{
  NS_ASSERTION(aContextFrame->StyleDisplay() == this, "unexpected aContextFrame");
  return IsFloatingStyle() && !aContextFrame->IsSVGText();
}

bool
nsStyleDisplay::HasTransform(const nsIFrame* aContextFrame) const
{
  NS_ASSERTION(aContextFrame->StyleDisplay() == this, "unexpected aContextFrame");
  return HasTransformStyle() && aContextFrame->IsFrameOfType(nsIFrame::eSupportsCSSTransforms);
}

bool
nsStyleDisplay::IsPositioned(const nsIFrame* aContextFrame) const
{
  NS_ASSERTION(aContextFrame->StyleDisplay() == this,
               "unexpected aContextFrame");
  return (IsAbsolutelyPositionedStyle() ||
          IsRelativelyPositionedStyle() ||
          HasTransform(aContextFrame) ||
          HasPerspectiveStyle()) &&
         !aContextFrame->IsSVGText();
}

bool
nsStyleDisplay::IsRelativelyPositioned(const nsIFrame* aContextFrame) const
{
  NS_ASSERTION(aContextFrame->StyleDisplay() == this, "unexpected aContextFrame");
  return IsRelativelyPositionedStyle() && !aContextFrame->IsSVGText();
}

bool
nsStyleDisplay::IsAbsolutelyPositioned(const nsIFrame* aContextFrame) const
{
  NS_ASSERTION(aContextFrame->StyleDisplay() == this, "unexpected aContextFrame");
  return IsAbsolutelyPositionedStyle() && !aContextFrame->IsSVGText();
}

uint8_t
nsStyleVisibility::GetEffectivePointerEvents(nsIFrame* aFrame) const
{
  if (aFrame->GetContent() && !aFrame->GetContent()->GetParent()) {
    
    
    
    
    nsIFrame* f = aFrame->GetContent()->GetPrimaryFrame();
    if (f) {
      return f->StyleVisibility()->mPointerEvents;
    }
  }
  return mPointerEvents;
}

#endif 
