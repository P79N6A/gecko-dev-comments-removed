





#include "nsSVGTextContainerFrame.h"

#include <algorithm>


#include "nsError.h"
#include "nsISVGGlyphFragmentNode.h"
#include "nsSVGUtils.h"
#include "SVGAnimatedNumberList.h"
#include "SVGLengthList.h"
#include "SVGNumberList.h"

using namespace mozilla;




NS_QUERYFRAME_HEAD(nsSVGTextContainerFrame)
  NS_QUERYFRAME_ENTRY(nsSVGTextContainerFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsSVGDisplayContainerFrame)

NS_IMPL_FRAMEARENA_HELPERS(nsSVGTextContainerFrame)

void
nsSVGTextContainerFrame::GetXY(SVGUserUnitList *aX, SVGUserUnitList *aY)
{
  static_cast<nsSVGElement*>(mContent)->
    GetAnimatedLengthListValues(aX, aY, nullptr);
}

void
nsSVGTextContainerFrame::GetDxDy(SVGUserUnitList *aDx, SVGUserUnitList *aDy)
{
  
  
  SVGUserUnitList xLengthList, yLengthList;
  static_cast<nsSVGElement*>(mContent)->
    GetAnimatedLengthListValues(&xLengthList, &yLengthList, aDx, aDy, nullptr);
}

const SVGNumberList*
nsSVGTextContainerFrame::GetRotate()
{
  SVGAnimatedNumberList *animList =
    static_cast<nsSVGElement*>(mContent)->
      GetAnimatedNumberList(nsGkAtoms::rotate);
  return animList ? &animList->GetAnimValue() : nullptr;
}




NS_IMETHODIMP
nsSVGTextContainerFrame::InsertFrames(ChildListID aListID,
                                      nsIFrame* aPrevFrame,
                                      nsFrameList& aFrameList)
{
  nsresult rv = nsSVGDisplayContainerFrame::InsertFrames(aListID,
                                                         aPrevFrame,
                                                         aFrameList);

  NotifyGlyphMetricsChange();
  return rv;
}

NS_IMETHODIMP
nsSVGTextContainerFrame::GetStartPositionOfChar(uint32_t charnum, nsISupports **_retval)
{
  NS_NOTREACHED("Shouldn't get here now that the old SVG text paths are disabled");
  *_retval = nullptr;
  return NS_ERROR_FAILURE; 
}

NS_IMETHODIMP
nsSVGTextContainerFrame::GetEndPositionOfChar(uint32_t charnum, nsISupports **_retval)
{
  NS_NOTREACHED("Shouldn't get here now that the old SVG text paths are disabled");
  *_retval = nullptr;
  return NS_ERROR_FAILURE; 
}

NS_IMETHODIMP
nsSVGTextContainerFrame::GetExtentOfChar(uint32_t charnum, dom::SVGIRect **_retval)
{
  NS_NOTREACHED("Shouldn't get here now that the old SVG text paths are disabled");
  *_retval = nullptr;
  return NS_ERROR_FAILURE; 
}

NS_IMETHODIMP
nsSVGTextContainerFrame::GetRotationOfChar(uint32_t charnum, float *_retval)
{
  NS_NOTREACHED("Shouldn't get here now that the old SVG text paths are disabled");
  *_retval = 0.0f;
  return NS_ERROR_FAILURE; 
}

uint32_t
nsSVGTextContainerFrame::GetNumberOfChars()
{
  uint32_t nchars = 0;
  nsISVGGlyphFragmentNode* node = GetFirstGlyphFragmentChildNode();

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
nsSVGTextContainerFrame::GetSubStringLength(uint32_t charnum, uint32_t nchars)
{
  float length = 0.0f;
  nsISVGGlyphFragmentNode *node = GetFirstGlyphFragmentChildNode();

  while (node) {
    uint32_t count = node->GetNumberOfChars();
    if (count > charnum) {
      uint32_t fragmentChars = std::min(nchars, count - charnum);
      float fragmentLength = node->GetSubStringLength(charnum, fragmentChars);
      length += fragmentLength;
      nchars -= fragmentChars;
      if (nchars == 0) break;
    }
    charnum -= std::min(charnum, count);
    node = GetNextGlyphFragmentChildNode(node);
  }

  return length;
}

int32_t
nsSVGTextContainerFrame::GetCharNumAtPosition(nsISVGPoint *point)
{
  int32_t index = -1;
  int32_t offset = 0;
  nsISVGGlyphFragmentNode *node = GetFirstGlyphFragmentChildNode();

  while (node) {
    uint32_t count = node->GetNumberOfChars();
    if (count > 0) {
      int32_t charnum = node->GetCharNumAtPosition(point);
      if (charnum >= 0) {
        index = charnum + offset;
      }
      offset += count;
      
      
    }
    node = GetNextGlyphFragmentChildNode(node);
  }

  return index;
}





nsISVGGlyphFragmentNode *
nsSVGTextContainerFrame::GetFirstGlyphFragmentChildNode()
{
  nsISVGGlyphFragmentNode *retval = nullptr;
  nsIFrame* kid = mFrames.FirstChild();
  while (kid) {
    retval = do_QueryFrame(kid);
    if (retval) break;
    kid = kid->GetNextSibling();
  }
  return retval;
}

nsISVGGlyphFragmentNode *
nsSVGTextContainerFrame::GetNextGlyphFragmentChildNode(nsISVGGlyphFragmentNode *node)
{
  nsISVGGlyphFragmentNode *retval = nullptr;
  nsIFrame *frame = do_QueryFrame(node);
  NS_ASSERTION(frame, "interface not implemented");
  frame = frame->GetNextSibling();
  while (frame) {
    retval = do_QueryFrame(frame);
    if (retval) break;
    frame = frame->GetNextSibling();
  }
  return retval;
}





void
nsSVGTextContainerFrame::CopyPositionList(nsTArray<float> *parentList,
                                        SVGUserUnitList *selfList,
                                        nsTArray<float> &dstList,
                                        uint32_t aOffset)
{
  dstList.Clear();

  uint32_t strLength = GetNumberOfChars();
  uint32_t parentCount = 0;
  if (parentList && parentList->Length() > aOffset) {
    parentCount = std::min(parentList->Length() - aOffset, strLength);
  }

  uint32_t selfCount = std::min(selfList->Length(), strLength);

  uint32_t count = std::max(parentCount, selfCount);

  if (!dstList.SetLength(count))
    return;

  for (uint32_t i = 0; i < selfCount; i++) {
    dstList[i] = (*selfList)[i];
  }
  for (uint32_t i = selfCount; i < parentCount; i++) {
    dstList[i] = (*parentList)[aOffset + i];
  }

}

void
nsSVGTextContainerFrame::CopyRotateList(nsTArray<float> *parentList,
                                        const SVGNumberList *selfList,
                                        nsTArray<float> &dstList,
                                        uint32_t aOffset)
{
  dstList.Clear();

  uint32_t strLength = GetNumberOfChars();
  uint32_t parentCount = 0;
  if (parentList && parentList->Length() > aOffset) {
    parentCount = std::min(parentList->Length() - aOffset, strLength);
  }

  uint32_t selfCount = std::min(selfList ? selfList->Length() : 0, strLength);
  uint32_t count = std::max(parentCount, selfCount);

  if (count > 0) {
    if (!dstList.SetLength(count))
      return;
    for (uint32_t i = 0; i < selfCount; i++) {
      dstList[i] = (*selfList)[i];
    }
    for (uint32_t i = selfCount; i < parentCount; i++) {
      dstList[i] = (*parentList)[aOffset + i];
    }
  } else if (parentList && !parentList->IsEmpty()) {
    
    dstList.AppendElement((*parentList)[parentList->Length() - 1]);
  }
}

uint32_t
nsSVGTextContainerFrame::BuildPositionList(uint32_t aOffset,
                                           uint32_t aDepth)
{
  NS_NOTREACHED("Shouldn't get here now that the old SVG text paths are disabled");

  nsSVGTextContainerFrame *parent = do_QueryFrame(mParent);
  nsTArray<float> *parentX = nullptr, *parentY = nullptr;
  nsTArray<float> *parentDx = nullptr, *parentDy = nullptr;
  nsTArray<float> *parentRotate = nullptr;
  if (parent) {
    parentX = &(parent->mX);
    parentY = &(parent->mY);
    parentDx = &(parent->mDx);
    parentDy = &(parent->mDy);
    parentRotate = &(parent->mRotate);
  }

  SVGUserUnitList x, y;
  GetXY(&x, &y);
  CopyPositionList(parentX, &x, mX, aOffset);
  CopyPositionList(parentY, &y, mY, aOffset);

  SVGUserUnitList dx, dy;
  GetDxDy(&dx, &dy);
  CopyPositionList(parentDx, &dx, mDx, aOffset);
  CopyPositionList(parentDy, &dy, mDy, aOffset);

  const SVGNumberList *rotate = GetRotate();
  CopyRotateList(parentRotate, rotate, mRotate, aOffset);

  uint32_t startIndex = 0;
  nsIFrame* kid = mFrames.FirstChild();
  while (kid) {
    nsSVGTextContainerFrame *text = do_QueryFrame(kid);
    if (text) {
      startIndex += text->BuildPositionList(startIndex, aDepth + 1);
    }
    kid = kid->GetNextSibling();
  }
  return startIndex;
}

void
nsSVGTextContainerFrame::GetEffectiveXY(nsTArray<float> &aX,
                                        nsTArray<float> &aY)
{
  aX.AppendElements(mX);
  aY.AppendElements(mY);
}

void
nsSVGTextContainerFrame::GetEffectiveDxDy(nsTArray<float> &aDx,
                                          nsTArray<float> &aDy)
{
  aDx.AppendElements(mDx);
  aDy.AppendElements(mDy);
}

void
nsSVGTextContainerFrame::GetEffectiveRotate(nsTArray<float> &aRotate)
{
  aRotate.AppendElements(mRotate);
}

void
nsSVGTextContainerFrame::SetWhitespaceCompression()
{
  bool compressWhitespace = true;

  for (const nsIFrame *frame = this; frame != nullptr; frame = frame->GetParent()) {
    static const nsIContent::AttrValuesArray strings[] =
      {&nsGkAtoms::preserve, &nsGkAtoms::_default, nullptr};

    int32_t index = frame->GetContent()->FindAttrValueIn(
                                           kNameSpaceID_XML,
                                           nsGkAtoms::space,
                                           strings, eCaseMatters);
    if (index == 0) {
      compressWhitespace = false;
      break;
    }
    if (index != nsIContent::ATTR_MISSING ||
        (frame->GetStateBits() & NS_STATE_IS_OUTER_SVG))
      break;
  }

  nsISVGGlyphFragmentNode* node = GetFirstGlyphFragmentChildNode();

  while (node) {
    node->SetWhitespaceCompression(compressWhitespace);
    node = GetNextGlyphFragmentChildNode(node);
  }
}
