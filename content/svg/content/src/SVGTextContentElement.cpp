




#include "mozilla/dom/SVGTextContentElement.h"
#include "nsISVGPoint.h"
#include "nsSVGTextContainerFrame.h"
#include "nsSVGTextFrame2.h"
#include "nsIDOMSVGAnimatedLength.h"
#include "nsIDOMSVGRect.h"
#include "nsIDOMSVGAnimatedEnum.h"

namespace mozilla {
namespace dom {

nsSVGTextContainerFrame*
SVGTextContentElement::GetTextContainerFrame()
{
  return do_QueryFrame(GetPrimaryFrame(Flush_Layout));
}

nsSVGTextFrame2*
SVGTextContentElement::GetSVGTextFrame()
{
  nsIFrame* frame = GetPrimaryFrame(Flush_Layout);
  while (frame) {
    nsSVGTextFrame2* textFrame = do_QueryFrame(frame);
    if (textFrame) {
      return textFrame;
    }
    frame = frame->GetParent();
  }
  return nullptr;
}



int32_t
SVGTextContentElement::GetNumberOfChars()
{
  if (NS_SVGTextCSSFramesEnabled()) {
    nsSVGTextFrame2* textFrame = GetSVGTextFrame();
    return textFrame ? textFrame->GetNumberOfChars(this) : 0;
  } else {
    nsSVGTextContainerFrame* metrics = GetTextContainerFrame();
    return metrics ? metrics->GetNumberOfChars() : 0;
  }
}

float
SVGTextContentElement::GetComputedTextLength()
{
  if (NS_SVGTextCSSFramesEnabled()) {
    nsSVGTextFrame2* textFrame = GetSVGTextFrame();
    return textFrame ? textFrame->GetComputedTextLength(this) : 0.0f;
  } else {
    nsSVGTextContainerFrame* metrics = GetTextContainerFrame();
    return metrics ? metrics->GetComputedTextLength() : 0.0f;
  }
}

float
SVGTextContentElement::GetSubStringLength(uint32_t charnum, uint32_t nchars, ErrorResult& rv)
{
  if (NS_SVGTextCSSFramesEnabled()) {
    nsSVGTextFrame2* textFrame = GetSVGTextFrame();
    if (!textFrame)
      return 0.0f;

    uint32_t charcount = textFrame->GetNumberOfChars(this);
    if (charcount <= charnum || nchars > charcount - charnum) {
      rv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
      return 0.0f;
    }

    if (nchars == 0)
      return 0.0f;

    return textFrame->GetSubStringLength(this, charnum, nchars);
  } else {
    nsSVGTextContainerFrame* metrics = GetTextContainerFrame();
    if (!metrics)
      return 0.0f;

    uint32_t charcount = metrics->GetNumberOfChars();
    if (charcount <= charnum || nchars > charcount - charnum) {
      rv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
      return 0.0f;
    }

    if (nchars == 0)
      return 0.0f;

    return metrics->GetSubStringLength(charnum, nchars);
  }
}

already_AddRefed<nsISVGPoint>
SVGTextContentElement::GetStartPositionOfChar(uint32_t charnum, ErrorResult& rv)
{
  nsCOMPtr<nsISVGPoint> point;
  if (NS_SVGTextCSSFramesEnabled()) {
    nsSVGTextFrame2* textFrame = GetSVGTextFrame();
    if (!textFrame) {
      rv.Throw(NS_ERROR_FAILURE);
      return nullptr;
    }

    rv = textFrame->GetStartPositionOfChar(this, charnum, getter_AddRefs(point));
  } else {
    nsSVGTextContainerFrame* metrics = GetTextContainerFrame();

    if (!metrics) {
      rv.Throw(NS_ERROR_FAILURE);
      return nullptr;
    }

    rv = metrics->GetStartPositionOfChar(charnum, getter_AddRefs(point));
  }
  return point.forget();
}

already_AddRefed<nsISVGPoint>
SVGTextContentElement::GetEndPositionOfChar(uint32_t charnum, ErrorResult& rv)
{
  nsCOMPtr<nsISVGPoint> point;
  if (NS_SVGTextCSSFramesEnabled()) {
    nsSVGTextFrame2* textFrame = GetSVGTextFrame();
    if (!textFrame) {
      rv.Throw(NS_ERROR_FAILURE);
      return nullptr;
    }

    rv = textFrame->GetEndPositionOfChar(this, charnum, getter_AddRefs(point));
  } else {
    nsSVGTextContainerFrame* metrics = GetTextContainerFrame();

    if (!metrics) {
      rv.Throw(NS_ERROR_FAILURE);
      return nullptr;
    }

    rv = metrics->GetEndPositionOfChar(charnum, getter_AddRefs(point));
  }
  return point.forget();
}

already_AddRefed<nsIDOMSVGRect>
SVGTextContentElement::GetExtentOfChar(uint32_t charnum, ErrorResult& rv)
{
  nsCOMPtr<nsIDOMSVGRect> rect;
  if (NS_SVGTextCSSFramesEnabled()) {
    nsSVGTextFrame2* textFrame = GetSVGTextFrame();

    if (!textFrame) {
      rv.Throw(NS_ERROR_FAILURE);
      return nullptr;
    }

    rv = textFrame->GetExtentOfChar(this, charnum, getter_AddRefs(rect));
  } else {
    nsSVGTextContainerFrame* metrics = GetTextContainerFrame();

    if (!metrics) {
      rv.Throw(NS_ERROR_FAILURE);
      return nullptr;
    }

    rv = metrics->GetExtentOfChar(charnum, getter_AddRefs(rect));
  }
  return rect.forget();
}

float
SVGTextContentElement::GetRotationOfChar(uint32_t charnum, ErrorResult& rv)
{
  float rotation;
  if (NS_SVGTextCSSFramesEnabled()) {
    nsSVGTextFrame2* textFrame = GetSVGTextFrame();

    if (!textFrame) {
      rv.Throw(NS_ERROR_FAILURE);
      return 0.0f;
    }

    rv = textFrame->GetRotationOfChar(this, charnum, &rotation);
  } else {
    nsSVGTextContainerFrame* metrics = GetTextContainerFrame();

    if (!metrics) {
      rv.Throw(NS_ERROR_FAILURE);
      return 0.0f;
    }

    rv = metrics->GetRotationOfChar(charnum, &rotation);
  }
  return rotation;
}

int32_t
SVGTextContentElement::GetCharNumAtPosition(nsISVGPoint& aPoint)
{
  if (NS_SVGTextCSSFramesEnabled()) {
    nsSVGTextFrame2* textFrame = GetSVGTextFrame();
    return textFrame ? textFrame->GetCharNumAtPosition(this, &aPoint) : -1;
  } else {
    nsSVGTextContainerFrame* metrics = GetTextContainerFrame();
    return metrics ? metrics->GetCharNumAtPosition(&aPoint) : -1;
  }
}

} 
} 
