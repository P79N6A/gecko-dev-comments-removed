





#include "HyperTextAccessible-inl.h"

#include "Accessible-inl.h"
#include "nsAccessibilityService.h"
#include "nsIAccessibleTypes.h"
#include "DocAccessible.h"
#include "Role.h"
#include "States.h"
#include "TextAttrs.h"
#include "TreeWalker.h"

#include "nsCaret.h"
#include "nsContentUtils.h"
#include "nsFocusManager.h"
#include "nsIDOMRange.h"
#include "nsIEditingSession.h"
#include "nsIFrame.h"
#include "nsFrameSelection.h"
#include "nsILineIterator.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIPersistentProperties2.h"
#include "nsIScrollableFrame.h"
#include "nsIServiceManager.h"
#include "nsITextControlElement.h"
#include "nsTextFragment.h"
#include "mozilla/Selection.h"
#include "mozilla/MathAlgorithms.h"
#include "gfxSkipChars.h"
#include <algorithm>

using namespace mozilla;
using namespace mozilla::a11y;





HyperTextAccessible::
  HyperTextAccessible(nsIContent* aNode, DocAccessible* aDoc) :
  AccessibleWrap(aNode, aDoc), xpcAccessibleHyperText()
{
  mGenericTypes |= eHyperText;
}

nsresult
HyperTextAccessible::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  xpcAccessibleHyperText::QueryInterface(aIID, aInstancePtr);
  return *aInstancePtr ? NS_OK : Accessible::QueryInterface(aIID, aInstancePtr);
}
NS_IMPL_ADDREF_INHERITED(HyperTextAccessible, AccessibleWrap)
NS_IMPL_RELEASE_INHERITED(HyperTextAccessible, AccessibleWrap)

role
HyperTextAccessible::NativeRole()
{
  nsIAtom *tag = mContent->Tag();

  if (tag == nsGkAtoms::dd)
    return roles::DEFINITION;

  if (tag == nsGkAtoms::form)
    return roles::FORM;

  if (tag == nsGkAtoms::blockquote || tag == nsGkAtoms::div ||
      tag == nsGkAtoms::section || tag == nsGkAtoms::nav)
    return roles::SECTION;

  if (tag == nsGkAtoms::h1 || tag == nsGkAtoms::h2 ||
      tag == nsGkAtoms::h3 || tag == nsGkAtoms::h4 ||
      tag == nsGkAtoms::h5 || tag == nsGkAtoms::h6)
    return roles::HEADING;

  if (tag == nsGkAtoms::article)
    return roles::DOCUMENT;
        
  
  if (tag == nsGkAtoms::header)
    return roles::HEADER;

  if (tag == nsGkAtoms::footer)
    return roles::FOOTER;

  if (tag == nsGkAtoms::aside)
    return roles::NOTE;

  
  nsIFrame *frame = GetFrame();
  if (frame && frame->GetType() == nsGkAtoms::blockFrame)
    return roles::PARAGRAPH;

  return roles::TEXT_CONTAINER; 
}

uint64_t
HyperTextAccessible::NativeState()
{
  uint64_t states = AccessibleWrap::NativeState();

  nsCOMPtr<nsITextControlElement> textControl = do_QueryInterface(mContent);
  bool editable = !!textControl;
  Accessible* hyperText = this;
  while (!editable && hyperText) {
    if (hyperText->IsHyperText())
      editable = hyperText->GetNode()->IsEditable();
    if (hyperText->IsDoc())
      break;

    hyperText = hyperText->Parent();
  }

  if (editable) {
    states |= states::EDITABLE;

  } else if (mContent->Tag() == nsGkAtoms::article) {
    
    states |= states::READONLY;
  }

  if (HasChildren())
    states |= states::SELECTABLE_TEXT;

  return states;
}

nsIntRect
HyperTextAccessible::GetBoundsInFrame(nsIFrame* aFrame,
                                      uint32_t aStartRenderedOffset,
                                      uint32_t aEndRenderedOffset)
{
  nsPresContext* presContext = mDoc->PresContext();
  if (aFrame->GetType() != nsGkAtoms::textFrame) {
    return aFrame->GetScreenRectInAppUnits().
      ToNearestPixels(presContext->AppUnitsPerDevPixel());
  }

  
  int32_t startContentOffset, endContentOffset;
  nsresult rv = RenderedToContentOffset(aFrame, aStartRenderedOffset, &startContentOffset);
  NS_ENSURE_SUCCESS(rv, nsIntRect());
  rv = RenderedToContentOffset(aFrame, aEndRenderedOffset, &endContentOffset);
  NS_ENSURE_SUCCESS(rv, nsIntRect());

  nsIFrame *frame;
  int32_t startContentOffsetInFrame;
  
  
  rv = aFrame->GetChildFrameContainingOffset(startContentOffset, false,
                                             &startContentOffsetInFrame, &frame);
  NS_ENSURE_SUCCESS(rv, nsIntRect());

  nsRect screenRect;
  while (frame && startContentOffset < endContentOffset) {
    
    
    
    
    nsRect frameScreenRect = frame->GetScreenRectInAppUnits();

    
    int32_t startFrameTextOffset, endFrameTextOffset;
    frame->GetOffsets(startFrameTextOffset, endFrameTextOffset);
    int32_t frameTotalTextLength = endFrameTextOffset - startFrameTextOffset;
    int32_t seekLength = endContentOffset - startContentOffset;
    int32_t frameSubStringLength = std::min(frameTotalTextLength - startContentOffsetInFrame, seekLength);

    
    nsPoint frameTextStartPoint;
    rv = frame->GetPointFromOffset(startContentOffset, &frameTextStartPoint);
    NS_ENSURE_SUCCESS(rv, nsIntRect());

    
    nsPoint frameTextEndPoint;
    rv = frame->GetPointFromOffset(startContentOffset + frameSubStringLength, &frameTextEndPoint);
    NS_ENSURE_SUCCESS(rv, nsIntRect());

    frameScreenRect.x += std::min(frameTextStartPoint.x, frameTextEndPoint.x);
    frameScreenRect.width = mozilla::Abs(frameTextStartPoint.x - frameTextEndPoint.x);

    screenRect.UnionRect(frameScreenRect, screenRect);

    
    startContentOffset += frameSubStringLength;
    startContentOffsetInFrame = 0;
    frame = frame->GetNextContinuation();
  }

  return screenRect.ToNearestPixels(presContext->AppUnitsPerDevPixel());
}

void
HyperTextAccessible::TextSubstring(int32_t aStartOffset, int32_t aEndOffset,
                                   nsAString& aText)
{
  aText.Truncate();

  int32_t startOffset = ConvertMagicOffset(aStartOffset);
  int32_t endOffset = ConvertMagicOffset(aEndOffset);

  int32_t startChildIdx = GetChildIndexAtOffset(startOffset);
  if (startChildIdx == -1)
    return;

  int32_t endChildIdx = GetChildIndexAtOffset(endOffset);
  if (endChildIdx == -1)
    return;

  if (startChildIdx == endChildIdx) {
    int32_t childOffset =  GetChildOffset(startChildIdx);
    if (childOffset == -1)
      return;

    Accessible* child = GetChildAt(startChildIdx);
    child->AppendTextTo(aText, startOffset - childOffset,
                        endOffset - startOffset);
    return;
  }

  int32_t startChildOffset =  GetChildOffset(startChildIdx);
  if (startChildOffset == -1)
    return;

  Accessible* startChild = GetChildAt(startChildIdx);
  startChild->AppendTextTo(aText, startOffset - startChildOffset);

  for (int32_t childIdx = startChildIdx + 1; childIdx < endChildIdx; childIdx++) {
    Accessible* child = GetChildAt(childIdx);
    child->AppendTextTo(aText);
  }

  int32_t endChildOffset =  GetChildOffset(endChildIdx);
  if (endChildOffset == -1)
    return;

  Accessible* endChild = GetChildAt(endChildIdx);
  endChild->AppendTextTo(aText, 0, endOffset - endChildOffset);
}

Accessible*
HyperTextAccessible::DOMPointToHypertextOffset(nsINode* aNode,
                                               int32_t aNodeOffset,
                                               int32_t* aHyperTextOffset,
                                               bool aIsEndOffset) const
{
  if (!aHyperTextOffset)
    return nullptr;
  *aHyperTextOffset = 0;

  if (!aNode)
    return nullptr;

  uint32_t addTextOffset = 0;
  nsINode* findNode = nullptr;

  if (aNodeOffset == -1) {
    findNode = aNode;

  } else if (aNode->IsNodeOfType(nsINode::eTEXT)) {
    
    
    
    nsIFrame *frame = aNode->AsContent()->GetPrimaryFrame();
    NS_ENSURE_TRUE(frame, nullptr);
    nsresult rv = ContentToRenderedOffset(frame, aNodeOffset, &addTextOffset);
    NS_ENSURE_SUCCESS(rv, nullptr);
    
    findNode = aNode;

  } else {
    
    
    
    
    
    
    

    findNode = aNode->GetChildAt(aNodeOffset);
    if (!findNode) {
      if (aNodeOffset == 0) {
        if (aNode == GetNode()) {
          
          
          *aHyperTextOffset = 0;
          return nullptr;
        }

        
        findNode = aNode;
      } else if (aNodeOffset == aNode->GetChildCount()) {
        
        for (nsINode* tmpNode = aNode;
             !findNode && tmpNode && tmpNode != mContent;
             tmpNode = tmpNode->GetParent()) {
          findNode = tmpNode->GetNextSibling();
        }
      }
    }
  }

  
  
  Accessible* descendantAcc = nullptr;
  if (findNode) {
    nsCOMPtr<nsIContent> findContent(do_QueryInterface(findNode));
    if (findContent && findContent->IsHTML() &&
        findContent->NodeInfo()->Equals(nsGkAtoms::br) &&
        findContent->AttrValueIs(kNameSpaceID_None,
                                 nsGkAtoms::mozeditorbogusnode,
                                 nsGkAtoms::_true,
                                 eIgnoreCase)) {
      
      *aHyperTextOffset = 0;
      return nullptr;
    }
    descendantAcc = GetFirstAvailableAccessible(findNode);
  }

  
  Accessible* childAccAtOffset = nullptr;
  while (descendantAcc) {
    Accessible* parentAcc = descendantAcc->Parent();
    if (parentAcc == this) {
      childAccAtOffset = descendantAcc;
      break;
    }

    
    
    
    
    
    if (aIsEndOffset) {
      
      
    
    
    addTextOffset = addTextOffset > 0;
    } else
      addTextOffset = 0;

    descendantAcc = parentAcc;
  }

  
  
  
  
  uint32_t childCount = ChildCount();

  uint32_t childIdx = 0;
  Accessible* childAcc = nullptr;
  for (; childIdx < childCount; childIdx++) {
    childAcc = mChildren[childIdx];
    if (childAcc == childAccAtOffset)
      break;

    *aHyperTextOffset += nsAccUtils::TextLength(childAcc);
  }

  if (childIdx < childCount) {
    *aHyperTextOffset += addTextOffset;
    NS_ASSERTION(childAcc == childAccAtOffset,
                 "These should be equal whenever we exit loop and childAcc != nullptr");

    if (childIdx < childCount - 1 ||
        addTextOffset < nsAccUtils::TextLength(childAccAtOffset)) {
      
      return childAccAtOffset;
    }
  }

  return nullptr;
}

bool
HyperTextAccessible::OffsetsToDOMRange(int32_t aStartOffset, int32_t aEndOffset,
                                       nsRange* aRange)
{
  DOMPoint startPoint = OffsetToDOMPoint(aStartOffset);
  if (!startPoint.node)
    return false;

  aRange->SetStart(startPoint.node, startPoint.idx);
  if (aStartOffset == aEndOffset) {
    aRange->SetEnd(startPoint.node, startPoint.idx);
    return true;
  }

  DOMPoint endPoint = OffsetToDOMPoint(aEndOffset);
  if (!endPoint.node)
    return false;

  aRange->SetEnd(endPoint.node, endPoint.idx);
  return true;
}

DOMPoint
HyperTextAccessible::OffsetToDOMPoint(int32_t aOffset)
{
  
  
  if (aOffset == 0) {
    nsCOMPtr<nsIEditor> editor = GetEditor();
    if (editor) {
      bool isEmpty = false;
      editor->GetDocumentIsEmpty(&isEmpty);
      if (isEmpty) {
        nsCOMPtr<nsIDOMElement> editorRootElm;
        editor->GetRootElement(getter_AddRefs(editorRootElm));

        nsCOMPtr<nsINode> editorRoot(do_QueryInterface(editorRootElm));
        return DOMPoint(editorRoot, 0);
      }
    }
  }

  int32_t childIdx = GetChildIndexAtOffset(aOffset);
  if (childIdx == -1)
    return DOMPoint();

  Accessible* child = GetChildAt(childIdx);
  int32_t innerOffset = aOffset - GetChildOffset(childIdx);

  
  if (child->IsTextLeaf()) {
    nsIContent* content = child->GetContent();
    int32_t idx = 0;
    if (NS_FAILED(RenderedToContentOffset(content->GetPrimaryFrame(),
                                          innerOffset, &idx)))
      return DOMPoint();

    return DOMPoint(content, idx);
  }

  
  NS_ASSERTION(innerOffset == 0 || innerOffset == 1, "A wrong inner offset!");
  nsINode* node = child->GetNode();
  nsINode* parentNode = node->GetParentNode();
  return parentNode ?
    DOMPoint(parentNode, parentNode->IndexOf(node) + innerOffset) :
    DOMPoint();
}

int32_t
HyperTextAccessible::FindOffset(int32_t aOffset, nsDirection aDirection,
                                nsSelectionAmount aAmount,
                                EWordMovementType aWordMovementType)
{
  
  HyperTextAccessible* text = this;
  Accessible* child = nullptr;
  int32_t innerOffset = aOffset;

  do {
    int32_t childIdx = text->GetChildIndexAtOffset(innerOffset);
    NS_ASSERTION(childIdx != -1, "Bad in offset!");
    if (childIdx == -1)
      return -1;

    child = text->GetChildAt(childIdx);
    innerOffset -= text->GetChildOffset(childIdx);

    text = child->AsHyperText();
  } while (text);

  nsIFrame* childFrame = child->GetFrame();
  NS_ENSURE_TRUE(childFrame, -1);

  int32_t innerContentOffset = innerOffset;
  if (child->IsTextLeaf()) {
    NS_ASSERTION(childFrame->GetType() == nsGkAtoms::textFrame, "Wrong frame!");
    RenderedToContentOffset(childFrame, innerOffset, &innerContentOffset);
  }

  nsIFrame* frameAtOffset = childFrame;
  int32_t unusedOffsetInFrame = 0;
  childFrame->GetChildFrameContainingOffset(innerContentOffset, true,
                                            &unusedOffsetInFrame,
                                            &frameAtOffset);

  const bool kIsJumpLinesOk = true; 
  const bool kIsScrollViewAStop = false; 
  const bool kIsKeyboardSelect = true; 
  const bool kIsVisualBidi = false; 
  nsPeekOffsetStruct pos(aAmount, aDirection, innerContentOffset,
                         0, kIsJumpLinesOk, kIsScrollViewAStop,
                         kIsKeyboardSelect, kIsVisualBidi,
                         aWordMovementType);
  nsresult rv = frameAtOffset->PeekOffset(&pos);

  
  if (NS_FAILED(rv) && aAmount == eSelectLine) {
    pos.mAmount = (aDirection == eDirNext) ? eSelectEndLine : eSelectBeginLine;
    frameAtOffset->PeekOffset(&pos);
  }
  if (!pos.mResultContent)
    return -1;

  
  
  
  int32_t hyperTextOffset = 0;
  Accessible* finalAccessible =
    DOMPointToHypertextOffset(pos.mResultContent, pos.mContentOffset,
                              &hyperTextOffset, aDirection == eDirNext);

  
  
  if (!finalAccessible && aDirection == eDirPrevious)
    return 0;

  return hyperTextOffset;
}

int32_t
HyperTextAccessible::FindLineBoundary(int32_t aOffset,
                                      EWhichLineBoundary aWhichLineBoundary)
{
  
  
  
  switch (aWhichLineBoundary) {
    case ePrevLineBegin: {
      
      
      if (IsEmptyLastLineOffset(aOffset))
        return FindOffset(aOffset, eDirPrevious, eSelectBeginLine);

      int32_t tmpOffset = FindOffset(aOffset, eDirPrevious, eSelectLine);
      return FindOffset(tmpOffset, eDirPrevious, eSelectBeginLine);
    }

    case ePrevLineEnd: {
      if (IsEmptyLastLineOffset(aOffset))
        return aOffset - 1;

      
      int32_t tmpOffset = FindOffset(aOffset, eDirPrevious, eSelectBeginLine);
      if (tmpOffset == 0)
        return 0;

      
      
      tmpOffset = FindOffset(aOffset, eDirPrevious, eSelectLine);
      return FindOffset(tmpOffset, eDirNext, eSelectEndLine);
    }

    case eThisLineBegin:
      if (IsEmptyLastLineOffset(aOffset))
        return aOffset;

      
      return FindOffset(aOffset, eDirPrevious, eSelectBeginLine);

    case eThisLineEnd:
      if (IsEmptyLastLineOffset(aOffset))
        return aOffset;

      
      return FindOffset(aOffset, eDirNext, eSelectEndLine);

    case eNextLineBegin: {
      if (IsEmptyLastLineOffset(aOffset))
        return aOffset;

      
      
      int32_t tmpOffset = FindOffset(aOffset, eDirNext, eSelectLine);
      if (tmpOffset == CharacterCount())
        return tmpOffset;

      return FindOffset(tmpOffset, eDirPrevious, eSelectBeginLine);
    }

    case eNextLineEnd: {
      if (IsEmptyLastLineOffset(aOffset))
        return aOffset;

      
      int32_t tmpOffset = FindOffset(aOffset, eDirNext, eSelectLine);
      if (tmpOffset != CharacterCount())
        return FindOffset(tmpOffset, eDirNext, eSelectEndLine);
      return tmpOffset;
    }
  }

  return -1;
}

void
HyperTextAccessible::TextBeforeOffset(int32_t aOffset,
                                      AccessibleTextBoundary aBoundaryType,
                                      int32_t* aStartOffset, int32_t* aEndOffset,
                                      nsAString& aText)
{
  *aStartOffset = *aEndOffset = 0;
  aText.Truncate();

  int32_t convertedOffset = ConvertMagicOffset(aOffset);
  if (convertedOffset < 0) {
    NS_ERROR("Wrong given offset!");
    return;
  }

  int32_t adjustedOffset = convertedOffset;
  if (aOffset == nsIAccessibleText::TEXT_OFFSET_CARET)
    adjustedOffset = AdjustCaretOffset(adjustedOffset);

  switch (aBoundaryType) {
    case BOUNDARY_CHAR:
      if (convertedOffset != 0)
        CharAt(convertedOffset - 1, aText, aStartOffset, aEndOffset);
      break;

    case BOUNDARY_WORD_START: {
      
      
      
      if (adjustedOffset == CharacterCount()) {
        *aEndOffset = FindWordBoundary(adjustedOffset, eDirPrevious, eStartWord);
        *aStartOffset = FindWordBoundary(*aEndOffset, eDirPrevious, eStartWord);
      } else {
        *aStartOffset = FindWordBoundary(adjustedOffset, eDirPrevious, eStartWord);
        *aEndOffset = FindWordBoundary(*aStartOffset, eDirNext, eStartWord);
        if (*aEndOffset != adjustedOffset) {
          *aEndOffset = *aStartOffset;
          *aStartOffset = FindWordBoundary(*aEndOffset, eDirPrevious, eStartWord);
        }
      }
      TextSubstring(*aStartOffset, *aEndOffset, aText);
      break;
    }

    case BOUNDARY_WORD_END: {
      
      *aEndOffset = FindWordBoundary(convertedOffset, eDirPrevious, eEndWord);
      *aStartOffset = FindWordBoundary(*aEndOffset, eDirPrevious, eEndWord);
      TextSubstring(*aStartOffset, *aEndOffset, aText);
      break;
    }

    case BOUNDARY_LINE_START:
      *aStartOffset = FindLineBoundary(adjustedOffset, ePrevLineBegin);
      *aEndOffset = FindLineBoundary(adjustedOffset, eThisLineBegin);
      TextSubstring(*aStartOffset, *aEndOffset, aText);
      break;

    case BOUNDARY_LINE_END: {
      *aEndOffset = FindLineBoundary(adjustedOffset, ePrevLineEnd);
      int32_t tmpOffset = *aEndOffset;
      
      if (*aEndOffset != 0 && !IsLineEndCharAt(*aEndOffset))
        tmpOffset--;

      *aStartOffset = FindLineBoundary(tmpOffset, ePrevLineEnd);
      TextSubstring(*aStartOffset, *aEndOffset, aText);
      break;
    }
  }
}

void
HyperTextAccessible::TextAtOffset(int32_t aOffset,
                                  AccessibleTextBoundary aBoundaryType,
                                  int32_t* aStartOffset, int32_t* aEndOffset,
                                  nsAString& aText)
{
  *aStartOffset = *aEndOffset = 0;
  aText.Truncate();

  int32_t adjustedOffset = ConvertMagicOffset(aOffset);
  if (adjustedOffset < 0) {
    NS_ERROR("Wrong given offset!");
    return;
  }

  switch (aBoundaryType) {
    case BOUNDARY_CHAR:
      
      
      if (aOffset == nsIAccessibleText::TEXT_OFFSET_CARET && IsCaretAtEndOfLine())
        *aStartOffset = *aEndOffset = adjustedOffset;
      else
        CharAt(adjustedOffset, aText, aStartOffset, aEndOffset);
      break;

    case BOUNDARY_WORD_START:
      if (aOffset == nsIAccessibleText::TEXT_OFFSET_CARET)
        adjustedOffset = AdjustCaretOffset(adjustedOffset);

      *aEndOffset = FindWordBoundary(adjustedOffset, eDirNext, eStartWord);
      *aStartOffset = FindWordBoundary(*aEndOffset, eDirPrevious, eStartWord);
      TextSubstring(*aStartOffset, *aEndOffset, aText);
      break;

    case BOUNDARY_WORD_END:
      
      
      
      *aEndOffset = FindWordBoundary(adjustedOffset, eDirNext, eEndWord);
      *aStartOffset = FindWordBoundary(*aEndOffset, eDirPrevious, eEndWord);
      TextSubstring(*aStartOffset, *aEndOffset, aText);
      break;

    case BOUNDARY_LINE_START:
      if (aOffset == nsIAccessibleText::TEXT_OFFSET_CARET)
        adjustedOffset = AdjustCaretOffset(adjustedOffset);

      *aStartOffset = FindLineBoundary(adjustedOffset, eThisLineBegin);
      *aEndOffset = FindLineBoundary(adjustedOffset, eNextLineBegin);
      TextSubstring(*aStartOffset, *aEndOffset, aText);
      break;

    case BOUNDARY_LINE_END:
      if (aOffset == nsIAccessibleText::TEXT_OFFSET_CARET)
        adjustedOffset = AdjustCaretOffset(adjustedOffset);

      
      *aStartOffset = FindLineBoundary(adjustedOffset, ePrevLineEnd);
      *aEndOffset = FindLineBoundary(adjustedOffset, eThisLineEnd);
      TextSubstring(*aStartOffset, *aEndOffset, aText);
      break;
  }
}

void
HyperTextAccessible::TextAfterOffset(int32_t aOffset,
                                     AccessibleTextBoundary aBoundaryType,
                                     int32_t* aStartOffset, int32_t* aEndOffset,
                                     nsAString& aText)
{
  *aStartOffset = *aEndOffset = 0;
  aText.Truncate();

  int32_t convertedOffset = ConvertMagicOffset(aOffset);
  if (convertedOffset < 0) {
    NS_ERROR("Wrong given offset!");
    return;
  }

  int32_t adjustedOffset = convertedOffset;
  if (aOffset == nsIAccessibleText::TEXT_OFFSET_CARET)
    adjustedOffset = AdjustCaretOffset(adjustedOffset);

  switch (aBoundaryType) {
    case BOUNDARY_CHAR:
      
      
      if (adjustedOffset >= CharacterCount())
        *aStartOffset = *aEndOffset = CharacterCount();
      else
        CharAt(adjustedOffset + 1, aText, aStartOffset, aEndOffset);
      break;

    case BOUNDARY_WORD_START:
      
      *aStartOffset = FindWordBoundary(adjustedOffset, eDirNext, eStartWord);
      *aEndOffset = FindWordBoundary(*aStartOffset, eDirNext, eStartWord);
      TextSubstring(*aStartOffset, *aEndOffset, aText);
      break;

    case BOUNDARY_WORD_END:
      
      
      
      if (convertedOffset == 0) {
        *aStartOffset = FindWordBoundary(convertedOffset, eDirNext, eEndWord);
        *aEndOffset = FindWordBoundary(*aStartOffset, eDirNext, eEndWord);
      } else {
        *aEndOffset = FindWordBoundary(convertedOffset, eDirNext, eEndWord);
        *aStartOffset = FindWordBoundary(*aEndOffset, eDirPrevious, eEndWord);
        if (*aStartOffset != convertedOffset) {
          *aStartOffset = *aEndOffset;
          *aEndOffset = FindWordBoundary(*aStartOffset, eDirNext, eEndWord);
        }
      }
      TextSubstring(*aStartOffset, *aEndOffset, aText);
      break;

    case BOUNDARY_LINE_START:
      *aStartOffset = FindLineBoundary(adjustedOffset, eNextLineBegin);
      *aEndOffset = FindLineBoundary(*aStartOffset, eNextLineBegin);
      TextSubstring(*aStartOffset, *aEndOffset, aText);
      break;

    case BOUNDARY_LINE_END:
      *aStartOffset = FindLineBoundary(adjustedOffset, eThisLineEnd);
      *aEndOffset = FindLineBoundary(adjustedOffset, eNextLineEnd);
      TextSubstring(*aStartOffset, *aEndOffset, aText);
      break;
  }
}

already_AddRefed<nsIPersistentProperties>
HyperTextAccessible::TextAttributes(bool aIncludeDefAttrs, int32_t aOffset,
                                       int32_t* aStartOffset,
                                       int32_t* aEndOffset)
{
  
  
  
  

  *aStartOffset = *aEndOffset = 0;
  nsCOMPtr<nsIPersistentProperties> attributes =
    do_CreateInstance(NS_PERSISTENTPROPERTIES_CONTRACTID);

  int32_t offset = ConvertMagicOffset(aOffset);
  Accessible* accAtOffset = GetChildAtOffset(offset);
  if (!accAtOffset) {
    
    
    if (offset == 0) {
      if (aIncludeDefAttrs) {
        TextAttrsMgr textAttrsMgr(this);
        textAttrsMgr.GetAttributes(attributes);
      }
      return attributes.forget();
    }
    return nullptr;
  }

  int32_t accAtOffsetIdx = accAtOffset->IndexInParent();
  int32_t startOffset = GetChildOffset(accAtOffsetIdx);
  int32_t endOffset = GetChildOffset(accAtOffsetIdx + 1);
  int32_t offsetInAcc = offset - startOffset;

  TextAttrsMgr textAttrsMgr(this, aIncludeDefAttrs, accAtOffset,
                            accAtOffsetIdx);
  textAttrsMgr.GetAttributes(attributes, &startOffset, &endOffset);

  
  nsIFrame *offsetFrame = accAtOffset->GetFrame();
  if (offsetFrame && offsetFrame->GetType() == nsGkAtoms::textFrame) {
    int32_t nodeOffset = 0;
    RenderedToContentOffset(offsetFrame, offsetInAcc, &nodeOffset);

    
    GetSpellTextAttribute(accAtOffset->GetNode(), nodeOffset,
                          &startOffset, &endOffset, attributes);
  }

  *aStartOffset = startOffset;
  *aEndOffset = endOffset;
  return attributes.forget();
}

already_AddRefed<nsIPersistentProperties>
HyperTextAccessible::DefaultTextAttributes()
{
  nsCOMPtr<nsIPersistentProperties> attributes =
    do_CreateInstance(NS_PERSISTENTPROPERTIES_CONTRACTID);

  TextAttrsMgr textAttrsMgr(this);
  textAttrsMgr.GetAttributes(attributes);
  return attributes.forget();
}

int32_t
HyperTextAccessible::GetLevelInternal()
{
  nsIAtom *tag = mContent->Tag();
  if (tag == nsGkAtoms::h1)
    return 1;
  if (tag == nsGkAtoms::h2)
    return 2;
  if (tag == nsGkAtoms::h3)
    return 3;
  if (tag == nsGkAtoms::h4)
    return 4;
  if (tag == nsGkAtoms::h5)
    return 5;
  if (tag == nsGkAtoms::h6)
    return 6;

  return AccessibleWrap::GetLevelInternal();
}

already_AddRefed<nsIPersistentProperties>
HyperTextAccessible::NativeAttributes()
{
  nsCOMPtr<nsIPersistentProperties> attributes =
    AccessibleWrap::NativeAttributes();

  
  
  nsIFrame *frame = GetFrame();
  if (frame && frame->GetType() == nsGkAtoms::blockFrame) {
    nsAutoString unused;
    attributes->SetStringProperty(NS_LITERAL_CSTRING("formatting"),
                                  NS_LITERAL_STRING("block"), unused);
  }

  if (FocusMgr()->IsFocused(this)) {
    int32_t lineNumber = CaretLineNumber();
    if (lineNumber >= 1) {
      nsAutoString strLineNumber;
      strLineNumber.AppendInt(lineNumber);
      nsAccUtils::SetAccAttr(attributes, nsGkAtoms::lineNumber, strLineNumber);
    }
  }

  if (!HasOwnContent())
    return attributes.forget();

  
  
  nsIAtom* tag = mContent->Tag();
  if (tag == nsGkAtoms::nav) {
    nsAccUtils::SetAccAttr(attributes, nsGkAtoms::xmlroles,
                           NS_LITERAL_STRING("navigation"));
  } else if (tag == nsGkAtoms::section)  {
    nsAccUtils::SetAccAttr(attributes, nsGkAtoms::xmlroles,
                           NS_LITERAL_STRING("region"));
  } else if (tag == nsGkAtoms::header || tag == nsGkAtoms::footer) {
    
    
    nsIContent* parent = mContent->GetParent();
    while (parent) {
      if (parent->Tag() == nsGkAtoms::article ||
          parent->Tag() == nsGkAtoms::section)
        break;
      parent = parent->GetParent();
    }

    
    if (!parent) {
      if (tag == nsGkAtoms::header) {
        nsAccUtils::SetAccAttr(attributes, nsGkAtoms::xmlroles,
                               NS_LITERAL_STRING("banner"));
      } else if (tag == nsGkAtoms::footer) {
        nsAccUtils::SetAccAttr(attributes, nsGkAtoms::xmlroles,
                               NS_LITERAL_STRING("contentinfo"));
      }
    }
  } else if (tag == nsGkAtoms::aside) {
    nsAccUtils::SetAccAttr(attributes, nsGkAtoms::xmlroles,
                           NS_LITERAL_STRING("complementary"));
  } else if (tag == nsGkAtoms::article) {
    nsAccUtils::SetAccAttr(attributes, nsGkAtoms::xmlroles,
                           NS_LITERAL_STRING("article"));
  } else if (tag == nsGkAtoms::main) {
    nsAccUtils::SetAccAttr(attributes, nsGkAtoms::xmlroles,
                           NS_LITERAL_STRING("main"));
  }

  return attributes.forget();
}

int32_t
HyperTextAccessible::OffsetAtPoint(int32_t aX, int32_t aY, uint32_t aCoordType)
{
  nsIFrame* hyperFrame = GetFrame();
  if (!hyperFrame)
    return -1;

  nsIntPoint coords = nsAccUtils::ConvertToScreenCoords(aX, aY, aCoordType,
                                                        this);

  nsPresContext* presContext = mDoc->PresContext();
  nsPoint coordsInAppUnits =
    coords.ToAppUnits(presContext->AppUnitsPerDevPixel());

  nsRect frameScreenRect = hyperFrame->GetScreenRectInAppUnits();
  if (!frameScreenRect.Contains(coordsInAppUnits.x, coordsInAppUnits.y))
    return -1; 

  nsPoint pointInHyperText(coordsInAppUnits.x - frameScreenRect.x,
                           coordsInAppUnits.y - frameScreenRect.y);

  
  

  
  
  int32_t offset = 0;
  uint32_t childCount = ChildCount();
  for (uint32_t childIdx = 0; childIdx < childCount; childIdx++) {
    Accessible* childAcc = mChildren[childIdx];

    nsIFrame *primaryFrame = childAcc->GetFrame();
    NS_ENSURE_TRUE(primaryFrame, -1);

    nsIFrame *frame = primaryFrame;
    while (frame) {
      nsIContent *content = frame->GetContent();
      NS_ENSURE_TRUE(content, -1);
      nsPoint pointInFrame = pointInHyperText - frame->GetOffsetTo(hyperFrame);
      nsSize frameSize = frame->GetSize();
      if (pointInFrame.x < frameSize.width && pointInFrame.y < frameSize.height) {
        
        if (frame->GetType() == nsGkAtoms::textFrame) {
          nsIFrame::ContentOffsets contentOffsets =
            frame->GetContentOffsetsFromPointExternal(pointInFrame, nsIFrame::IGNORE_SELECTION_STYLE);
          if (contentOffsets.IsNull() || contentOffsets.content != content) {
            return -1; 
          }
          uint32_t addToOffset;
          nsresult rv = ContentToRenderedOffset(primaryFrame,
                                                contentOffsets.offset,
                                                &addToOffset);
          NS_ENSURE_SUCCESS(rv, -1);
          offset += addToOffset;
        }
        return offset;
      }
      frame = frame->GetNextContinuation();
    }

    offset += nsAccUtils::TextLength(childAcc);
  }

  return -1; 
}

nsIntRect
HyperTextAccessible::TextBounds(int32_t aStartOffset, int32_t aEndOffset,
                                uint32_t aCoordType)
{
  int32_t startOffset = ConvertMagicOffset(aStartOffset);
  int32_t endOffset = ConvertMagicOffset(aEndOffset);
  NS_ASSERTION(startOffset < endOffset, "Wrong bad in!");

  int32_t childIdx = GetChildIndexAtOffset(startOffset);
  if (childIdx == -1)
    return nsIntRect();

  nsIntRect bounds;
  int32_t prevOffset = GetChildOffset(childIdx);
  int32_t offset1 = startOffset - prevOffset;

  while (childIdx < ChildCount()) {
    nsIFrame* frame = GetChildAt(childIdx)->GetFrame();
    if (!frame) {
      NS_NOTREACHED("No frame for a child!");
      continue;
    }

    childIdx++;
    int32_t nextOffset = GetChildOffset(childIdx);
    if (nextOffset >= endOffset) {
      bounds.UnionRect(bounds, GetBoundsInFrame(frame, offset1,
                                                endOffset - prevOffset));
      break;
    }

    bounds.UnionRect(bounds, GetBoundsInFrame(frame, offset1,
                                              nextOffset - prevOffset));

    prevOffset = nextOffset;
    offset1 = 0;
  }

  nsAccUtils::ConvertScreenCoordsTo(&bounds.x, &bounds.y, aCoordType, this);
  return bounds;
}

already_AddRefed<nsIEditor>
HyperTextAccessible::GetEditor() const
{
  if (!mContent->HasFlag(NODE_IS_EDITABLE)) {
    
    Accessible* ancestor = Parent();
    while (ancestor) {
      HyperTextAccessible* hyperText = ancestor->AsHyperText();
      if (hyperText) {
        
        
        return hyperText->GetEditor();
      }

      ancestor = ancestor->Parent();
    }

    return nullptr;
  }

  nsCOMPtr<nsIDocShell> docShell = nsCoreUtils::GetDocShellFor(mContent);
  nsCOMPtr<nsIEditingSession> editingSession(do_GetInterface(docShell));
  if (!editingSession)
    return nullptr; 

  nsCOMPtr<nsIEditor> editor;
  nsIDocument* docNode = mDoc->DocumentNode();
  editingSession->GetEditorForWindow(docNode->GetWindow(),
                                     getter_AddRefs(editor));
  return editor.forget();
}





nsresult
HyperTextAccessible::SetSelectionRange(int32_t aStartPos, int32_t aEndPos)
{
  
  
  
  
  
  
  nsCOMPtr<nsIEditor> editor = GetEditor();

  bool isFocusable = InteractiveState() & states::FOCUSABLE;

  
  
  
  
  if (isFocusable)
    TakeFocus();

  
  SetSelectionBoundsAt(0, aStartPos, aEndPos);

  
  
  Selection* domSel = DOMSelection();
  NS_ENSURE_STATE(domSel);

  for (int32_t idx = domSel->GetRangeCount() - 1; idx > 0; idx--)
    domSel->RemoveRange(domSel->GetRangeAt(idx));

  
  
  
  if (isFocusable)
    return NS_OK;

  nsFocusManager* DOMFocusManager = nsFocusManager::GetFocusManager();
  if (DOMFocusManager) {
    NS_ENSURE_TRUE(mDoc, NS_ERROR_FAILURE);
    nsIDocument* docNode = mDoc->DocumentNode();
    NS_ENSURE_TRUE(docNode, NS_ERROR_FAILURE);
    nsCOMPtr<nsPIDOMWindow> window = docNode->GetWindow();
    nsCOMPtr<nsIDOMElement> result;
    DOMFocusManager->MoveFocus(window, nullptr, nsIFocusManager::MOVEFOCUS_CARET,
                               nsIFocusManager::FLAG_BYMOVEFOCUS, getter_AddRefs(result));
  }

  return NS_OK;
}

int32_t
HyperTextAccessible::CaretOffset() const
{
  
  
  if (!IsDoc() && !FocusMgr()->IsFocused(this) &&
      (InteractiveState() & states::FOCUSABLE)) {
    return -1;
  }

  
  
  FocusManager::FocusDisposition focusDisp =
    FocusMgr()->IsInOrContainsFocus(this);
  if (focusDisp == FocusManager::eNone)
    return -1;

  
  
  Selection* domSel = DOMSelection();
  NS_ENSURE_TRUE(domSel, -1);

  nsINode* focusNode = domSel->GetFocusNode();
  uint32_t focusOffset = domSel->FocusOffset();

  
  
  if (focusDisp == FocusManager::eContainedByFocus) {
    nsINode* resultNode =
      nsCoreUtils::GetDOMNodeFromDOMPoint(focusNode, focusOffset);

    nsINode* thisNode = GetNode();
    if (resultNode != thisNode &&
        !nsCoreUtils::IsAncestorOf(thisNode, resultNode))
      return -1;
  }

  int32_t caretOffset = -1;
  DOMPointToHypertextOffset(focusNode, focusOffset, &caretOffset);
  return caretOffset;
}

int32_t
HyperTextAccessible::CaretLineNumber()
{
  
  
  nsRefPtr<nsFrameSelection> frameSelection = FrameSelection();
  if (!frameSelection)
    return -1;

  Selection* domSel =
    frameSelection->GetSelection(nsISelectionController::SELECTION_NORMAL);
  if (!domSel)
    return - 1;

  nsINode* caretNode = domSel->GetFocusNode();
  if (!caretNode || !caretNode->IsContent())
    return -1;

  nsIContent* caretContent = caretNode->AsContent();
  if (!nsCoreUtils::IsAncestorOf(GetNode(), caretContent))
    return -1;

  int32_t returnOffsetUnused;
  uint32_t caretOffset = domSel->FocusOffset();
  nsFrameSelection::HINT hint = frameSelection->GetHint();
  nsIFrame *caretFrame = frameSelection->GetFrameForNodeOffset(caretContent, caretOffset,
                                                               hint, &returnOffsetUnused);
  NS_ENSURE_TRUE(caretFrame, -1);

  int32_t lineNumber = 1;
  nsAutoLineIterator lineIterForCaret;
  nsIContent *hyperTextContent = IsContent() ? mContent.get() : nullptr;
  while (caretFrame) {
    if (hyperTextContent == caretFrame->GetContent()) {
      return lineNumber; 
    }
    nsIFrame *parentFrame = caretFrame->GetParent();
    if (!parentFrame)
      break;

    
    nsIFrame *sibling = parentFrame->GetFirstPrincipalChild();
    while (sibling && sibling != caretFrame) {
      nsAutoLineIterator lineIterForSibling = sibling->GetLineIterator();
      if (lineIterForSibling) {
        
        int32_t addLines = lineIterForSibling->GetNumLines();
        lineNumber += addLines;
      }
      sibling = sibling->GetNextSibling();
    }

    
    if (!lineIterForCaret) {   
      lineIterForCaret = parentFrame->GetLineIterator();
      if (lineIterForCaret) {
        
        int32_t addLines = lineIterForCaret->FindLineContaining(caretFrame);
        lineNumber += addLines;
      }
    }

    caretFrame = parentFrame;
  }

  NS_NOTREACHED("DOM ancestry had this hypertext but frame ancestry didn't");
  return lineNumber;
}

nsIntRect
HyperTextAccessible::GetCaretRect(nsIWidget** aWidget)
{
  *aWidget = nullptr;

  nsRefPtr<nsCaret> caret = mDoc->PresShell()->GetCaret();
  NS_ENSURE_TRUE(caret, nsIntRect());

  nsISelection* caretSelection = caret->GetCaretDOMSelection();
  NS_ENSURE_TRUE(caretSelection, nsIntRect());

  bool isVisible = false;
  caret->GetCaretVisible(&isVisible);
  if (!isVisible)
    return nsIntRect();

  nsRect rect;
  nsIFrame* frame = caret->GetGeometry(caretSelection, &rect);
  if (!frame || rect.IsEmpty())
    return nsIntRect();

  nsPoint offset;
  
  
  *aWidget = frame->GetNearestWidget(offset);
  NS_ENSURE_TRUE(*aWidget, nsIntRect());
  rect.MoveBy(offset);

  nsIntRect caretRect;
  caretRect = rect.ToOutsidePixels(frame->PresContext()->AppUnitsPerDevPixel());
  
  caretRect.MoveBy((*aWidget)->WidgetToScreenOffset() - (*aWidget)->GetClientOffset());

  
  
  
  
  nsIntRect charRect = CharBounds(CaretOffset(),
                                  nsIAccessibleCoordinateType::COORDTYPE_SCREEN_RELATIVE);
  if (!charRect.IsEmpty()) {
    caretRect.height -= charRect.y - caretRect.y;
    caretRect.y = charRect.y;
  }
  return caretRect;
}

already_AddRefed<nsFrameSelection>
HyperTextAccessible::FrameSelection() const
{
  nsIFrame* frame = GetFrame();
  return frame ? frame->GetFrameSelection() : nullptr;
}

void
HyperTextAccessible::GetSelectionDOMRanges(int16_t aType,
                                           nsTArray<nsRange*>* aRanges)
{
  
  nsRefPtr<nsFrameSelection> frameSelection = FrameSelection();
  if (!frameSelection ||
      frameSelection->GetDisplaySelection() <= nsISelectionController::SELECTION_HIDDEN)
    return;

  Selection* domSel = frameSelection->GetSelection(aType);
  if (!domSel)
    return;

  nsCOMPtr<nsINode> startNode = GetNode();

  nsCOMPtr<nsIEditor> editor = GetEditor();
  if (editor) {
    nsCOMPtr<nsIDOMElement> editorRoot;
    editor->GetRootElement(getter_AddRefs(editorRoot));
    startNode = do_QueryInterface(editorRoot);
  }

  if (!startNode)
    return;

  uint32_t childCount = startNode->GetChildCount();
  nsresult rv = domSel->
    GetRangesForIntervalArray(startNode, 0, startNode, childCount, true, aRanges);
  NS_ENSURE_SUCCESS_VOID(rv);

  
  uint32_t numRanges = aRanges->Length();
  for (uint32_t idx = 0; idx < numRanges; idx ++) {
    if ((*aRanges)[idx]->Collapsed()) {
      aRanges->RemoveElementAt(idx);
      --numRanges;
      --idx;
    }
  }
}

int32_t
HyperTextAccessible::SelectionCount()
{
  nsTArray<nsRange*> ranges;
  GetSelectionDOMRanges(nsISelectionController::SELECTION_NORMAL, &ranges);
  return ranges.Length();
}

bool
HyperTextAccessible::SelectionBoundsAt(int32_t aSelectionNum,
                                       int32_t* aStartOffset,
                                       int32_t* aEndOffset)
{
  *aStartOffset = *aEndOffset = 0;

  nsTArray<nsRange*> ranges;
  GetSelectionDOMRanges(nsISelectionController::SELECTION_NORMAL, &ranges);

  uint32_t rangeCount = ranges.Length();
  if (aSelectionNum < 0 || aSelectionNum >= rangeCount)
    return false;

  nsRange* range = ranges[aSelectionNum];

  
  nsINode* startNode = range->GetStartParent();
  nsINode* endNode = range->GetEndParent();
  int32_t startOffset = range->StartOffset(), endOffset = range->EndOffset();

  
  
  int32_t rangeCompare = nsContentUtils::ComparePoints(endNode, endOffset,
                                                       startNode, startOffset);
  if (rangeCompare < 0) {
    nsINode* tempNode = startNode;
    startNode = endNode;
    endNode = tempNode;
    int32_t tempOffset = startOffset;
    startOffset = endOffset;
    endOffset = tempOffset;
  }

  Accessible* startAccessible =
    DOMPointToHypertextOffset(startNode, startOffset, aStartOffset);
  if (!startAccessible) {
    *aStartOffset = 0; 
  }

  DOMPointToHypertextOffset(endNode, endOffset, aEndOffset, true);
  return true;
}

bool
HyperTextAccessible::SetSelectionBoundsAt(int32_t aSelectionNum,
                                          int32_t aStartOffset,
                                          int32_t aEndOffset)
{
  int32_t startOffset = ConvertMagicOffset(aStartOffset);
  int32_t endOffset = ConvertMagicOffset(aEndOffset);

  Selection* domSel = DOMSelection();
  if (!domSel)
    return false;

  nsRefPtr<nsRange> range;
  uint32_t rangeCount = domSel->GetRangeCount();
  if (aSelectionNum == rangeCount)
    range = new nsRange(mContent);
  else
    range = domSel->GetRangeAt(aSelectionNum);

  if (!range)
    return false;

  if (!OffsetsToDOMRange(startOffset, endOffset, range))
    return false;

  
  
  if (aSelectionNum == rangeCount)
    return NS_SUCCEEDED(domSel->AddRange(range));

  domSel->RemoveRange(range);
  return NS_SUCCEEDED(domSel->AddRange(range));
}

bool
HyperTextAccessible::RemoveFromSelection(int32_t aSelectionNum)
{
  Selection* domSel = DOMSelection();
  if (!domSel)
    return false;

  if (aSelectionNum < 0 || aSelectionNum >= domSel->GetRangeCount())
    return false;

  domSel->RemoveRange(domSel->GetRangeAt(aSelectionNum));
  return true;
}

void
HyperTextAccessible::ScrollSubstringTo(int32_t aStartOffset, int32_t aEndOffset,
                                       uint32_t aScrollType)
{
  nsRefPtr<nsRange> range = new nsRange(mContent);
  if (OffsetsToDOMRange(aStartOffset, aEndOffset, range))
    nsCoreUtils::ScrollSubstringTo(GetFrame(), range, aScrollType);
}

void
HyperTextAccessible::ScrollSubstringToPoint(int32_t aStartOffset,
                                            int32_t aEndOffset,
                                            uint32_t aCoordinateType,
                                            int32_t aX, int32_t aY)
{
  nsIFrame *frame = GetFrame();
  if (!frame)
    return;

  nsIntPoint coords = nsAccUtils::ConvertToScreenCoords(aX, aY, aCoordinateType,
                                                        this);

  nsRefPtr<nsRange> range = new nsRange(mContent);
  if (!OffsetsToDOMRange(aStartOffset, aEndOffset, range))
    return;

  nsPresContext* presContext = frame->PresContext();
  nsPoint coordsInAppUnits =
    coords.ToAppUnits(presContext->AppUnitsPerDevPixel());

  bool initialScrolled = false;
  nsIFrame *parentFrame = frame;
  while ((parentFrame = parentFrame->GetParent())) {
    nsIScrollableFrame *scrollableFrame = do_QueryFrame(parentFrame);
    if (scrollableFrame) {
      if (!initialScrolled) {
        
        
        nsRect frameRect = parentFrame->GetScreenRectInAppUnits();
        nscoord offsetPointX = coordsInAppUnits.x - frameRect.x;
        nscoord offsetPointY = coordsInAppUnits.y - frameRect.y;

        nsSize size(parentFrame->GetSize());

        
        size.width = size.width ? size.width : 1;
        size.height = size.height ? size.height : 1;

        int16_t hPercent = offsetPointX * 100 / size.width;
        int16_t vPercent = offsetPointY * 100 / size.height;

        nsresult rv = nsCoreUtils::ScrollSubstringTo(frame, range, vPercent, hPercent);
        if (NS_FAILED(rv))
          return;

        initialScrolled = true;
      } else {
        
        
        
        
        nsCoreUtils::ScrollFrameToPoint(parentFrame, frame, coords);
      }
    }
    frame = parentFrame;
  }
}





ENameValueFlag
HyperTextAccessible::NativeName(nsString& aName)
{
  
  bool hasImgAlt = false;
  if (mContent->IsHTML(nsGkAtoms::img)) {
    hasImgAlt = mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::alt, aName);
    if (!aName.IsEmpty())
      return eNameOK;
  }

  ENameValueFlag nameFlag = AccessibleWrap::NativeName(aName);
  if (!aName.IsEmpty())
    return nameFlag;

  
  
  
  if (IsAbbreviation() &&
      mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::title, aName))
    aName.CompressWhitespace();

  return hasImgAlt ? eNoNameOnPurpose : eNameOK;
}

void
HyperTextAccessible::InvalidateChildren()
{
  mOffsets.Clear();

  AccessibleWrap::InvalidateChildren();
}

bool
HyperTextAccessible::RemoveChild(Accessible* aAccessible)
{
  int32_t childIndex = aAccessible->IndexInParent();
  int32_t count = mOffsets.Length() - childIndex;
  if (count > 0)
    mOffsets.RemoveElementsAt(childIndex, count);

  return Accessible::RemoveChild(aAccessible);
}

void
HyperTextAccessible::CacheChildren()
{
  
  
  

  TreeWalker walker(this, mContent);
  Accessible* child = nullptr;
  Accessible* lastChild = nullptr;
  while ((child = walker.NextChild())) {
    if (lastChild)
      AppendChild(lastChild);

    lastChild = child;
  }

  if (lastChild) {
    if (lastChild->IsHTMLBr())
      Document()->UnbindFromDocument(lastChild);
    else
      AppendChild(lastChild);
  }
}




nsresult
HyperTextAccessible::ContentToRenderedOffset(nsIFrame* aFrame, int32_t aContentOffset,
                                             uint32_t* aRenderedOffset) const
{
  if (!aFrame) {
    
    
    *aRenderedOffset = 0;
    return NS_OK;
  }

  if (IsTextField()) {
    *aRenderedOffset = aContentOffset;
    return NS_OK;
  }

  NS_ASSERTION(aFrame->GetType() == nsGkAtoms::textFrame,
               "Need text frame for offset conversion");
  NS_ASSERTION(aFrame->GetPrevContinuation() == nullptr,
               "Call on primary frame only");

  gfxSkipChars skipChars;
  gfxSkipCharsIterator iter;
  
  nsresult rv = aFrame->GetRenderedText(nullptr, &skipChars, &iter, 0, aContentOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  uint32_t ourRenderedStart = iter.GetSkippedOffset();
  int32_t ourContentStart = iter.GetOriginalOffset();

  *aRenderedOffset = iter.ConvertOriginalToSkipped(aContentOffset + ourContentStart) -
                    ourRenderedStart;

  return NS_OK;
}

nsresult
HyperTextAccessible::RenderedToContentOffset(nsIFrame* aFrame, uint32_t aRenderedOffset,
                                             int32_t* aContentOffset) const
{
  if (IsTextField()) {
    *aContentOffset = aRenderedOffset;
    return NS_OK;
  }

  *aContentOffset = 0;
  NS_ENSURE_TRUE(aFrame, NS_ERROR_FAILURE);

  NS_ASSERTION(aFrame->GetType() == nsGkAtoms::textFrame,
               "Need text frame for offset conversion");
  NS_ASSERTION(aFrame->GetPrevContinuation() == nullptr,
               "Call on primary frame only");

  gfxSkipChars skipChars;
  gfxSkipCharsIterator iter;
  
  nsresult rv = aFrame->GetRenderedText(nullptr, &skipChars, &iter, 0, aRenderedOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  uint32_t ourRenderedStart = iter.GetSkippedOffset();
  int32_t ourContentStart = iter.GetOriginalOffset();

  *aContentOffset = iter.ConvertSkippedToOriginal(aRenderedOffset + ourRenderedStart) - ourContentStart;

  return NS_OK;
}




int32_t
HyperTextAccessible::GetChildOffset(uint32_t aChildIndex,
                                    bool aInvalidateAfter)
{
  if (aChildIndex == 0) {
    if (aInvalidateAfter)
      mOffsets.Clear();

    return aChildIndex;
  }

  int32_t count = mOffsets.Length() - aChildIndex;
  if (count > 0) {
    if (aInvalidateAfter)
      mOffsets.RemoveElementsAt(aChildIndex, count);

    return mOffsets[aChildIndex - 1];
  }

  uint32_t lastOffset = mOffsets.IsEmpty() ?
    0 : mOffsets[mOffsets.Length() - 1];

  while (mOffsets.Length() < aChildIndex) {
    Accessible* child = mChildren[mOffsets.Length()];
    lastOffset += nsAccUtils::TextLength(child);
    mOffsets.AppendElement(lastOffset);
  }

  return mOffsets[aChildIndex - 1];
}

int32_t
HyperTextAccessible::GetChildIndexAtOffset(uint32_t aOffset)
{
  uint32_t lastOffset = 0;
  uint32_t offsetCount = mOffsets.Length();
  if (offsetCount > 0) {
    lastOffset = mOffsets[offsetCount - 1];
    if (aOffset < lastOffset) {
      uint32_t low = 0, high = offsetCount;
      while (high > low) {
        uint32_t mid = (high + low) >> 1;
        if (mOffsets[mid] == aOffset)
          return mid < offsetCount - 1 ? mid + 1 : mid;

        if (mOffsets[mid] < aOffset)
          low = mid + 1;
        else
          high = mid;
      }
      if (high == offsetCount)
        return -1;

      return low;
    }
  }

  uint32_t childCount = ChildCount();
  while (mOffsets.Length() < childCount) {
    Accessible* child = GetChildAt(mOffsets.Length());
    lastOffset += nsAccUtils::TextLength(child);
    mOffsets.AppendElement(lastOffset);
    if (aOffset < lastOffset)
      return mOffsets.Length() - 1;
  }

  if (aOffset == lastOffset)
    return mOffsets.Length() - 1;

  return -1;
}




nsresult
HyperTextAccessible::GetDOMPointByFrameOffset(nsIFrame* aFrame, int32_t aOffset,
                                              Accessible* aAccessible,
                                              DOMPoint* aPoint)
{
  NS_ENSURE_ARG(aAccessible);

  if (!aFrame) {
    
    
    NS_ASSERTION(!aAccessible->IsDoc(), 
                 "Shouldn't be called on document accessible!");

    nsIContent* content = aAccessible->GetContent();
    NS_ASSERTION(content, "Shouldn't operate on defunct accessible!");

    nsIContent* parent = content->GetParent();

    aPoint->idx = parent->IndexOf(content) + 1;
    aPoint->node = parent;

  } else if (aFrame->GetType() == nsGkAtoms::textFrame) {
    nsIContent* content = aFrame->GetContent();
    NS_ENSURE_STATE(content);

    nsIFrame *primaryFrame = content->GetPrimaryFrame();
    nsresult rv = RenderedToContentOffset(primaryFrame, aOffset, &(aPoint->idx));
    NS_ENSURE_SUCCESS(rv, rv);

    aPoint->node = content;

  } else {
    nsIContent* content = aFrame->GetContent();
    NS_ENSURE_STATE(content);

    nsIContent* parent = content->GetParent();
    NS_ENSURE_STATE(parent);

    aPoint->idx = parent->IndexOf(content);
    aPoint->node = parent;
  }

  return NS_OK;
}


nsresult
HyperTextAccessible::RangeBoundToHypertextOffset(nsRange* aRange,
                                                 bool aIsStartBound,
                                                 bool aIsStartHTOffset,
                                                 int32_t* aHTOffset)
{
  nsINode* node = nullptr;
  int32_t nodeOffset = 0;

  if (aIsStartBound) {
    node = aRange->GetStartParent();
    nodeOffset = aRange->StartOffset();
  } else {
    node = aRange->GetEndParent();
    nodeOffset = aRange->EndOffset();
  }

  Accessible* startAcc =
    DOMPointToHypertextOffset(node, nodeOffset, aHTOffset);

  if (aIsStartHTOffset && !startAcc)
    *aHTOffset = 0;

  return NS_OK;
}


nsresult
HyperTextAccessible::GetSpellTextAttribute(nsINode* aNode,
                                           int32_t aNodeOffset,
                                           int32_t* aHTStartOffset,
                                           int32_t* aHTEndOffset,
                                           nsIPersistentProperties* aAttributes)
{
  nsRefPtr<nsFrameSelection> fs = FrameSelection();
  if (!fs)
    return NS_OK;

  Selection* domSel = fs->GetSelection(nsISelectionController::SELECTION_SPELLCHECK);
  if (!domSel)
    return NS_OK;

  int32_t rangeCount = domSel->GetRangeCount();
  if (rangeCount <= 0)
    return NS_OK;

  int32_t startHTOffset = 0, endHTOffset = 0;
  nsresult rv = NS_OK;
  for (int32_t idx = 0; idx < rangeCount; idx++) {
    nsRange* range = domSel->GetRangeAt(idx);
    if (range->Collapsed())
      continue;

    
    
    nsINode* endNode = range->GetEndParent();
    int32_t endOffset = range->EndOffset();
    if (nsContentUtils::ComparePoints(aNode, aNodeOffset, endNode, endOffset) >= 0)
      continue;

    
    
    
    
    nsINode* startNode = range->GetStartParent();
    int32_t startOffset = range->StartOffset();
    if (nsContentUtils::ComparePoints(startNode, startOffset, aNode,
                                      aNodeOffset) <= 0) {
      rv = RangeBoundToHypertextOffset(range, true, true, &startHTOffset);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = RangeBoundToHypertextOffset(range, false, false, &endHTOffset);
      NS_ENSURE_SUCCESS(rv, rv);

      if (startHTOffset > *aHTStartOffset)
        *aHTStartOffset = startHTOffset;

      if (endHTOffset < *aHTEndOffset)
        *aHTEndOffset = endHTOffset;

      if (aAttributes) {
        nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::invalid,
                               NS_LITERAL_STRING("spelling"));
      }

      return NS_OK;
    }

    
    rv = RangeBoundToHypertextOffset(range, true, false, &endHTOffset);
    NS_ENSURE_SUCCESS(rv, rv);

    if (idx > 0) {
      rv = RangeBoundToHypertextOffset(domSel->GetRangeAt(idx - 1), false,
                                       true, &startHTOffset);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (startHTOffset > *aHTStartOffset)
      *aHTStartOffset = startHTOffset;

    if (endHTOffset < *aHTEndOffset)
      *aHTEndOffset = endHTOffset;

    return NS_OK;
  }

  
  
  
  
  rv = RangeBoundToHypertextOffset(domSel->GetRangeAt(rangeCount - 1), false,
                                   true, &startHTOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  if (startHTOffset > *aHTStartOffset)
    *aHTStartOffset = startHTOffset;

  return NS_OK;
}

bool 
HyperTextAccessible::IsTextRole()
{
  if (mRoleMapEntry &&
      (mRoleMapEntry->role == roles::GRAPHIC ||
       mRoleMapEntry->role == roles::IMAGE_MAP ||
       mRoleMapEntry->role == roles::SLIDER ||
       mRoleMapEntry->role == roles::PROGRESSBAR ||
       mRoleMapEntry->role == roles::SEPARATOR))
    return false;

  return true;
}
