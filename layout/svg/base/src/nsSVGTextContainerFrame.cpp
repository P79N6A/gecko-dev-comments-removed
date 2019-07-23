



































#include "nsSVGContainerFrame.h"
#include "nsSVGTextFrame.h"
#include "nsSVGUtils.h"
#include "nsSVGMatrix.h"
#include "nsSVGOuterSVGFrame.h"
#include "nsIDOMSVGTextElement.h"
#include "nsIDOMSVGAnimatedLengthList.h"
#include "nsISVGGlyphFragmentLeaf.h"
#include "nsDOMError.h"




NS_INTERFACE_MAP_BEGIN(nsSVGTextContainerFrame)
  NS_INTERFACE_MAP_ENTRY(nsISVGTextContentMetrics)
NS_INTERFACE_MAP_END_INHERITING(nsSVGDisplayContainerFrame)

void
nsSVGTextContainerFrame::UpdateGraphic()
{
  nsSVGTextFrame *textFrame = GetTextFrame();
  if (textFrame)
    textFrame->NotifyGlyphMetricsChange();
}

NS_IMETHODIMP_(already_AddRefed<nsIDOMSVGLengthList>)
nsSVGTextContainerFrame::GetX()
{
  nsCOMPtr<nsIDOMSVGTextPositioningElement> tpElement =
    do_QueryInterface(mContent);

  if (!tpElement)
    return nsnull;

  if (!mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::x))
    return nsnull;

  nsCOMPtr<nsIDOMSVGAnimatedLengthList> animLengthList;
  tpElement->GetX(getter_AddRefs(animLengthList));
  nsIDOMSVGLengthList *retval;
  animLengthList->GetAnimVal(&retval);
  return retval;
}

NS_IMETHODIMP_(already_AddRefed<nsIDOMSVGLengthList>)
nsSVGTextContainerFrame::GetY()
{
  nsCOMPtr<nsIDOMSVGTextPositioningElement> tpElement =
    do_QueryInterface(mContent);

  if (!tpElement)
    return nsnull;

  if (!mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::y))
    return nsnull;

  nsCOMPtr<nsIDOMSVGAnimatedLengthList> animLengthList;
  tpElement->GetY(getter_AddRefs(animLengthList));
  nsIDOMSVGLengthList *retval;
  animLengthList->GetAnimVal(&retval);
  return retval;
}

NS_IMETHODIMP_(already_AddRefed<nsIDOMSVGLengthList>)
nsSVGTextContainerFrame::GetDx()
{
  nsCOMPtr<nsIDOMSVGTextPositioningElement> tpElement =
    do_QueryInterface(mContent);

  if (!tpElement)
    return nsnull;

  nsCOMPtr<nsIDOMSVGAnimatedLengthList> animLengthList;
  tpElement->GetDx(getter_AddRefs(animLengthList));
  nsIDOMSVGLengthList *retval;
  animLengthList->GetAnimVal(&retval);
  return retval;
}

NS_IMETHODIMP_(already_AddRefed<nsIDOMSVGLengthList>)
nsSVGTextContainerFrame::GetDy()
{
  nsCOMPtr<nsIDOMSVGTextPositioningElement> tpElement =
    do_QueryInterface(mContent);

  if (!tpElement)
    return nsnull;

  nsCOMPtr<nsIDOMSVGAnimatedLengthList> animLengthList;
  tpElement->GetDy(getter_AddRefs(animLengthList));
  nsIDOMSVGLengthList *retval;
  animLengthList->GetAnimVal(&retval);
  return retval;
}




NS_IMETHODIMP
nsSVGTextContainerFrame::RemoveFrame(nsIAtom *aListName, nsIFrame *aOldFrame)
{
  nsSVGTextFrame *textFrame = GetTextFrame();

  nsresult rv = nsSVGDisplayContainerFrame::RemoveFrame(aListName, aOldFrame);

  if (textFrame)
    textFrame->NotifyGlyphMetricsChange();

  return rv;
}




NS_IMETHODIMP
nsSVGTextContainerFrame::GetNumberOfChars(PRInt32 *_retval)
{
  *_retval = GetNumberOfChars();

  return NS_OK;
}

NS_IMETHODIMP
nsSVGTextContainerFrame::GetComputedTextLength(float *_retval)
{
  *_retval = GetComputedTextLength();

  return NS_OK;
}

NS_IMETHODIMP
nsSVGTextContainerFrame::GetSubStringLength(PRUint32 charnum,
                                            PRUint32 nchars,
                                            float *_retval)
{
  if (nchars == 0) {
    *_retval = 0.0f;
    return NS_OK;
  }

  if (charnum + nchars > GetNumberOfChars()) {
    *_retval = 0.0f;
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  *_retval = GetSubStringLengthNoValidation(charnum, nchars);

  return NS_OK;
}

NS_IMETHODIMP
nsSVGTextContainerFrame::GetStartPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval)
{
  *_retval = nsnull;

  if (charnum >= GetNumberOfChars()) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsISVGGlyphFragmentNode *node = GetFirstGlyphFragmentChildNode();
  if (!node) {
    return NS_ERROR_FAILURE;
  }

  PRUint32 offset;
  nsISVGGlyphFragmentLeaf *fragment = GetGlyphFragmentAtCharNum(node, charnum, &offset);
  if (!fragment) {
    return NS_ERROR_FAILURE;
  }

  return fragment->GetStartPositionOfChar(charnum - offset, _retval);
}

NS_IMETHODIMP
nsSVGTextContainerFrame::GetEndPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval)
{
  *_retval = nsnull;

  if (charnum >= GetNumberOfChars()) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsISVGGlyphFragmentNode *node = GetFirstGlyphFragmentChildNode();
  if (!node) {
    return NS_ERROR_FAILURE;
  }

  PRUint32 offset;
  nsISVGGlyphFragmentLeaf *fragment = GetGlyphFragmentAtCharNum(node, charnum, &offset);
  if (!fragment) {
    return NS_ERROR_FAILURE;
  }

  return fragment->GetEndPositionOfChar(charnum - offset, _retval);
}

NS_IMETHODIMP
nsSVGTextContainerFrame::GetExtentOfChar(PRUint32 charnum, nsIDOMSVGRect **_retval)
{
  *_retval = nsnull;

  if (charnum >= GetNumberOfChars()) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsISVGGlyphFragmentNode *node = GetFirstGlyphFragmentChildNode();
  if (!node) {
    return NS_ERROR_FAILURE;
  }

  PRUint32 offset;
  nsISVGGlyphFragmentLeaf *fragment = GetGlyphFragmentAtCharNum(node, charnum, &offset);
  if (!fragment) {
    return NS_ERROR_FAILURE;
  }

  return fragment->GetExtentOfChar(charnum - offset, _retval);
}

NS_IMETHODIMP
nsSVGTextContainerFrame::GetRotationOfChar(PRUint32 charnum, float *_retval)
{
  *_retval = 0.0f;

  if (charnum >= GetNumberOfChars()) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsISVGGlyphFragmentNode *node = GetFirstGlyphFragmentChildNode();
  if (!node) {
    return NS_ERROR_FAILURE;
  }

  PRUint32 offset;
  nsISVGGlyphFragmentLeaf *fragment = GetGlyphFragmentAtCharNum(node, charnum, &offset);
  if (!fragment) {
    return NS_ERROR_FAILURE;
  }

  return fragment->GetRotationOfChar(charnum - offset, _retval);
}

NS_IMETHODIMP
nsSVGTextContainerFrame::GetCharNumAtPosition(nsIDOMSVGPoint *point, PRInt32 *_retval)
{
  *_retval = GetCharNumAtPosition(point);

  return NS_OK;
}





nsISVGGlyphFragmentNode *
nsSVGTextContainerFrame::GetFirstGlyphFragmentChildNode()
{
  nsISVGGlyphFragmentNode *retval = nsnull;
  nsIFrame* kid = mFrames.FirstChild();
  while (kid) {
    CallQueryInterface(kid, &retval);
    if (retval) break;
    kid = kid->GetNextSibling();
  }
  return retval;
}

nsISVGGlyphFragmentNode *
nsSVGTextContainerFrame::GetNextGlyphFragmentChildNode(nsISVGGlyphFragmentNode *node)
{
  nsISVGGlyphFragmentNode *retval = nsnull;
  nsIFrame *frame = nsnull;
  CallQueryInterface(node, &frame);
  NS_ASSERTION(frame, "interface not implemented");
  frame = frame->GetNextSibling();
  while (frame) {
    CallQueryInterface(frame, &retval);
    if (retval) break;
    frame = frame->GetNextSibling();
  }
  return retval;
}

void
nsSVGTextContainerFrame::SetWhitespaceHandling()
{
  
  nsISVGGlyphFragmentNode* node = GetFirstGlyphFragmentChildNode();
  nsISVGGlyphFragmentNode* next;

  PRUint8 whitespaceHandling = COMPRESS_WHITESPACE | TRIM_LEADING_WHITESPACE;

  for (nsIFrame *frame = this; frame != nsnull; frame = frame->GetParent()) {
    nsIContent *content = frame->GetContent();
    static nsIContent::AttrValuesArray strings[] =
      {&nsGkAtoms::preserve, &nsGkAtoms::_default, nsnull};

    PRInt32 index = content->FindAttrValueIn(kNameSpaceID_XML,
                                             nsGkAtoms::space,
                                             strings, eCaseMatters);
    if (index == 0) {
      whitespaceHandling = PRESERVE_WHITESPACE;
      break;
    }
    if (index != nsIContent::ATTR_MISSING ||
        (frame->GetStateBits() & NS_STATE_IS_OUTER_SVG))
      break;
  }

  while (node) {
    next = GetNextGlyphFragmentChildNode(node);
    if (!next && (whitespaceHandling & COMPRESS_WHITESPACE)) {
      whitespaceHandling |= TRIM_TRAILING_WHITESPACE;
    }
    node->SetWhitespaceHandling(whitespaceHandling);
    node = next;
    whitespaceHandling &= ~TRIM_LEADING_WHITESPACE;
  }
}

PRUint32
nsSVGTextContainerFrame::GetNumberOfChars()
{
  PRUint32 nchars = 0;
  nsISVGGlyphFragmentNode* node;
  node = GetFirstGlyphFragmentChildNode();

  while (node) {
    nchars += node->GetNumberOfChars();
    node = GetNextGlyphFragmentChildNode(node);
  }

  return nchars;
}

float
nsSVGTextContainerFrame::GetComputedTextLength()
{
  float length = 0.0f;
  nsISVGGlyphFragmentNode* node = GetFirstGlyphFragmentChildNode();

  while (node) {
    length += node->GetComputedTextLength();
    node = GetNextGlyphFragmentChildNode(node);
  }

  return length;
}

float
nsSVGTextContainerFrame::GetSubStringLengthNoValidation(PRUint32 charnum,
                                                        PRUint32 nchars)
{
  float length = 0.0f;
  nsISVGGlyphFragmentNode *node = GetFirstGlyphFragmentChildNode();

  while (node) {
    PRUint32 count = node->GetNumberOfChars();
    if (count > charnum) {
      PRUint32 fragmentChars = PR_MIN(nchars, count);
      float fragmentLength = node->GetSubStringLength(charnum, fragmentChars);
      length += fragmentLength;
      nchars -= fragmentChars;
      if (nchars == 0) break;
    }
    charnum -= PR_MIN(charnum, count);
    node = GetNextGlyphFragmentChildNode(node);
  }

  return length;
}

PRInt32
nsSVGTextContainerFrame::GetCharNumAtPosition(nsIDOMSVGPoint *point)
{
  PRInt32 index = -1;
  PRInt32 offset = 0;
  nsISVGGlyphFragmentNode *node = GetFirstGlyphFragmentChildNode();

  while (node) {
    PRUint32 count = node->GetNumberOfChars();
    if (count > 0) {
      PRInt32 charnum = node->GetCharNumAtPosition(point);
      if (charnum >= 0) {
        index = charnum + offset;
      }
      offset += count;
      
      
    }
    node = GetNextGlyphFragmentChildNode(node);
  }

  return index;
}





nsISVGGlyphFragmentLeaf *
nsSVGTextContainerFrame::GetGlyphFragmentAtCharNum(nsISVGGlyphFragmentNode* node,
                                                   PRUint32 charnum,
                                                   PRUint32 *offset)
{
  nsISVGGlyphFragmentLeaf *fragment = node->GetFirstGlyphFragment();
  *offset = 0;
  
  while (fragment) {
    PRUint32 count = fragment->GetNumberOfChars();
    if (count > charnum)
      return fragment;
    charnum -= count;
    *offset += count;
    fragment = fragment->GetNextGlyphFragment();
  }

  
  return nsnull;
}

nsSVGTextFrame *
nsSVGTextContainerFrame::GetTextFrame()
{
  for (nsIFrame *frame = this; frame != nsnull; frame = frame->GetParent()) {
    if (frame->GetType() == nsGkAtoms::svgTextFrame) {
      return NS_STATIC_CAST(nsSVGTextFrame*, frame);
    }
  }
  return nsnull;
}
