





#include "HyperTextAccessible.h"

#include "Accessible-inl.h"
#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "DocAccessible.h"
#include "Role.h"
#include "States.h"
#include "TextAttrs.h"

#include "nsIClipboard.h"
#include "nsContentUtils.h"
#include "nsFocusManager.h"
#include "nsIDOMRange.h"
#include "nsIEditingSession.h"
#include "nsIEditor.h"
#include "nsIFrame.h"
#include "nsFrameSelection.h"
#include "nsILineIterator.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIPlaintextEditor.h"
#include "nsIScrollableFrame.h"
#include "nsIServiceManager.h"
#include "nsTextFragment.h"
#include "mozilla/Selection.h"
#include "gfxSkipChars.h"
#include <algorithm>

using namespace mozilla;
using namespace mozilla::a11y;





HyperTextAccessible::
  HyperTextAccessible(nsIContent* aNode, DocAccessible* aDoc) :
  AccessibleWrap(aNode, aDoc)
{
  mGenericTypes |= eHyperText;
}

NS_IMPL_ADDREF_INHERITED(HyperTextAccessible, AccessibleWrap)
NS_IMPL_RELEASE_INHERITED(HyperTextAccessible, AccessibleWrap)

nsresult
HyperTextAccessible::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  *aInstancePtr = nullptr;

  
  if (!IsTextRole())
    return Accessible::QueryInterface(aIID, aInstancePtr);

  if (aIID.Equals(NS_GET_IID(nsIAccessibleText))) {
    *aInstancePtr = static_cast<nsIAccessibleText*>(this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  if (aIID.Equals(NS_GET_IID(nsIAccessibleHyperText))) {
    *aInstancePtr = static_cast<nsIAccessibleHyperText*>(this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  if (aIID.Equals(NS_GET_IID(nsIAccessibleEditableText))) {
    *aInstancePtr = static_cast<nsIAccessibleEditableText*>(this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  return Accessible::QueryInterface(aIID, aInstancePtr);
}

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

  nsCOMPtr<nsIEditor> editor = GetEditor();
  if (editor) {
    states |= states::EDITABLE;

  } else if (mContent->Tag() == nsGkAtoms::article) {
    
    states |= states::READONLY;
  }

  if (HasChildren())
    states |= states::SELECTABLE_TEXT;

  return states;
}


nsIntRect
HyperTextAccessible::GetBoundsForString(nsIFrame* aFrame, uint32_t aStartRenderedOffset,
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
    frameScreenRect.x += frameTextStartPoint.x;

    
    nsPoint frameTextEndPoint;
    rv = frame->GetPointFromOffset(startContentOffset + frameSubStringLength, &frameTextEndPoint);
    NS_ENSURE_SUCCESS(rv, nsIntRect());
    frameScreenRect.width = frameTextEndPoint.x - frameTextStartPoint.x;

    screenRect.UnionRect(frameScreenRect, screenRect);

    
    startContentOffset += frameSubStringLength;
    startContentOffsetInFrame = 0;
    frame = frame->GetNextContinuation();
  }

  return screenRect.ToNearestPixels(presContext->AppUnitsPerDevPixel());
}




nsIFrame*
HyperTextAccessible::GetPosAndText(int32_t& aStartOffset, int32_t& aEndOffset,
                                   nsAString* aText, nsIFrame** aEndFrame,
                                   nsIntRect* aBoundsRect,
                                   Accessible** aStartAcc,
                                   Accessible** aEndAcc)
{
  aStartOffset = ConvertMagicOffset(aStartOffset);
  aEndOffset = ConvertMagicOffset(aEndOffset);

  int32_t startOffset = aStartOffset;
  int32_t endOffset = aEndOffset;
  
  bool isPassword = (Role() == roles::PASSWORD_TEXT);

  
  if (aText) {
    aText->Truncate();
  }
  if (endOffset < 0) {
    const int32_t kMaxTextLength = 32767;
    endOffset = kMaxTextLength; 
  }
  else if (startOffset > endOffset) {
    return nullptr;
  }

  nsIFrame *startFrame = nullptr;
 nsIFrame* endFrame = nullptr;
  if (aEndFrame) {
    *aEndFrame = nullptr;
  }
  if (aBoundsRect) {
    aBoundsRect->SetEmpty();
  }
  if (aStartAcc)
    *aStartAcc = nullptr;
  if (aEndAcc)
    *aEndAcc = nullptr;

  nsIntRect unionRect;
  Accessible* lastAccessible = nullptr;

  gfxSkipChars skipChars;
  gfxSkipCharsIterator iter;

  
  
  uint32_t childCount = ChildCount();
  for (uint32_t childIdx = 0; childIdx < childCount; childIdx++) {
    Accessible* childAcc = mChildren[childIdx];
    lastAccessible = childAcc;

    nsIFrame *frame = childAcc->GetFrame();
    if (!frame) {
      continue;
    }
    nsIFrame *primaryFrame = frame;
    endFrame = frame;
    if (!nsAccUtils::IsEmbeddedObject(childAcc)) {
      
      
      int32_t substringEndOffset = -1;
      uint32_t ourRenderedStart = 0;
      int32_t ourContentStart = 0;
      if (frame->GetType() == nsGkAtoms::textFrame) {
        nsresult rv = frame->GetRenderedText(nullptr, &skipChars, &iter);
        if (NS_SUCCEEDED(rv)) {
          ourRenderedStart = iter.GetSkippedOffset();
          ourContentStart = iter.GetOriginalOffset();
          substringEndOffset =
            iter.ConvertOriginalToSkipped(skipChars.GetOriginalCharCount() +
                                          ourContentStart) - ourRenderedStart;
        }
      }
      if (substringEndOffset < 0) {
        
        
        substringEndOffset = nsAccUtils::TextLength(childAcc);
      }
      if (startOffset < substringEndOffset ||
          (startOffset == substringEndOffset && (childIdx == childCount - 1))) {
        
        if (startOffset > 0 || endOffset < substringEndOffset) {
          
          
          int32_t outStartLineUnused;
          int32_t contentOffset;
          if (frame->GetType() == nsGkAtoms::textFrame) {
            contentOffset = iter.ConvertSkippedToOriginal(startOffset) +
                            ourRenderedStart - ourContentStart;
          }
          else {
            contentOffset = startOffset;
          }
          frame->GetChildFrameContainingOffset(contentOffset, true,
                                               &outStartLineUnused, &frame);
          if (aEndFrame) {
            *aEndFrame = frame; 
            if (aEndAcc)
              NS_ADDREF(*aEndAcc = childAcc);
          }
          if (substringEndOffset > endOffset) {
            
            substringEndOffset = endOffset;
          }
          aEndOffset = endOffset;
        }
        if (aText) {
          if (isPassword) {
            for (int32_t count = startOffset; count < substringEndOffset; count ++)
              *aText += '*'; 
          }
          else {
            childAcc->AppendTextTo(*aText, startOffset,
                                   substringEndOffset - startOffset);
          }
        }
        if (aBoundsRect) {    
          aBoundsRect->UnionRect(*aBoundsRect,
                                 GetBoundsForString(primaryFrame, startOffset,
                                                    substringEndOffset));
        }
        if (!startFrame) {
          startFrame = frame;
          aStartOffset = startOffset;
          if (aStartAcc)
            NS_ADDREF(*aStartAcc = childAcc);
        }
        
        
        startOffset = 0;
      }
      else {
        
        
        startOffset -= substringEndOffset;
      }
      
      endOffset -= substringEndOffset;
    }
    else {
      
      
      if (startOffset >= 1) {
        -- startOffset;
      }
      else {
        if (endOffset > 0) {
          if (aText) {
            
            if (frame->GetType() == nsGkAtoms::brFrame) {
              *aText += kForcedNewLineChar;
            } else if (nsAccUtils::MustPrune(this)) {
              *aText += kImaginaryEmbeddedObjectChar;
              
              
            } else {
              *aText += kEmbeddedObjectChar;
            }
          }
          if (aBoundsRect) {
            nsIntRect frameScreenRect = frame->GetScreenRectInAppUnits().
              ToNearestPixels(frame->PresContext()->AppUnitsPerDevPixel());
            aBoundsRect->UnionRect(*aBoundsRect, frameScreenRect);
          }
        }
        if (!startFrame) {
          startFrame = frame;
          aStartOffset = 0;
          if (aStartAcc)
            NS_ADDREF(*aStartAcc = childAcc);
        }
      }
      -- endOffset;
    }
    if (endOffset <= 0 && startFrame) {
      break; 
    }
  }

  if (aStartAcc && !*aStartAcc) {
    NS_IF_ADDREF(*aStartAcc = lastAccessible);
  }
  if (aEndFrame && !*aEndFrame) {
    *aEndFrame = endFrame;
    if (aEndAcc && !*aEndAcc)
      NS_IF_ADDREF(*aEndAcc = lastAccessible);
  }

  return startFrame;
}

NS_IMETHODIMP
HyperTextAccessible::GetText(int32_t aStartOffset, int32_t aEndOffset,
                             nsAString& aText)
{
  aText.Truncate();

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  int32_t startOffset = ConvertMagicOffset(aStartOffset);
  int32_t endOffset = ConvertMagicOffset(aEndOffset);

  int32_t startChildIdx = GetChildIndexAtOffset(startOffset);
  if (startChildIdx == -1) {
    
    return (startOffset == 0 && endOffset == 0) ? NS_OK : NS_ERROR_INVALID_ARG;
  }

  int32_t endChildIdx = GetChildIndexAtOffset(endOffset);
  if (endChildIdx == -1)
    return NS_ERROR_INVALID_ARG;

  if (startChildIdx == endChildIdx) {
    int32_t childOffset =  GetChildOffset(startChildIdx);
    NS_ENSURE_STATE(childOffset != -1);

    Accessible* child = GetChildAt(startChildIdx);
    child->AppendTextTo(aText, startOffset - childOffset,
                        endOffset - startOffset);

    return NS_OK;
  }

  int32_t startChildOffset =  GetChildOffset(startChildIdx);
  NS_ENSURE_STATE(startChildOffset != -1);

  Accessible* startChild = GetChildAt(startChildIdx);
  startChild->AppendTextTo(aText, startOffset - startChildOffset);

  for (int32_t childIdx = startChildIdx + 1; childIdx < endChildIdx; childIdx++) {
    Accessible* child = GetChildAt(childIdx);
    child->AppendTextTo(aText);
  }

  int32_t endChildOffset =  GetChildOffset(endChildIdx);
  NS_ENSURE_STATE(endChildOffset != -1);

  Accessible* endChild = GetChildAt(endChildIdx);
  endChild->AppendTextTo(aText, 0, endOffset - endChildOffset);

  return NS_OK;
}




NS_IMETHODIMP
HyperTextAccessible::GetCharacterCount(int32_t* aCharacterCount)
{
  NS_ENSURE_ARG_POINTER(aCharacterCount);
  *aCharacterCount = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  *aCharacterCount = CharacterCount();
  return NS_OK;
}




NS_IMETHODIMP
HyperTextAccessible::GetCharacterAtOffset(int32_t aOffset, PRUnichar* aCharacter)
{
  NS_ENSURE_ARG_POINTER(aCharacter);
  *aCharacter = L'\0';

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsAutoString character;
  if (GetCharAt(aOffset, eGetAt, character)) {
    *aCharacter = character.First();
    return NS_OK;
  }

  return NS_ERROR_INVALID_ARG;
}

Accessible*
HyperTextAccessible::DOMPointToHypertextOffset(nsINode* aNode,
                                               int32_t aNodeOffset,
                                               int32_t* aHyperTextOffset,
                                               bool aIsEndOffset)
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
    }
    else {
      
      
      
      addTextOffset =
        (nsAccUtils::TextLength(descendantAcc) == addTextOffset) ? 1 : 0;
    }

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

nsresult
HyperTextAccessible::HypertextOffsetsToDOMRange(int32_t aStartHTOffset,
                                                int32_t aEndHTOffset,
                                                nsRange* aRange)
{
  
  
  if (aStartHTOffset == 0 && aEndHTOffset == 0) {
    nsCOMPtr<nsIEditor> editor = GetEditor();
    if (editor) {
      bool isEmpty = false;
      editor->GetDocumentIsEmpty(&isEmpty);
      if (isEmpty) {
        nsCOMPtr<nsIDOMElement> editorRootElm;
        editor->GetRootElement(getter_AddRefs(editorRootElm));

        nsCOMPtr<nsINode> editorRoot(do_QueryInterface(editorRootElm));
        if (editorRoot) {
          aRange->SetStart(editorRoot, 0);
          aRange->SetEnd(editorRoot, 0);

          return NS_OK;
        }
      }
    }
  }

  nsRefPtr<Accessible> startAcc, endAcc;
  int32_t startOffset = aStartHTOffset, endOffset = aEndHTOffset;
  nsIFrame *startFrame = nullptr, *endFrame = nullptr;

  startFrame = GetPosAndText(startOffset, endOffset, nullptr, &endFrame, nullptr,
                             getter_AddRefs(startAcc), getter_AddRefs(endAcc));
  if (!startAcc || !endAcc)
    return NS_ERROR_FAILURE;

  DOMPoint startPoint, endPoint;
  nsresult rv = GetDOMPointByFrameOffset(startFrame, startOffset, startAcc,
                                         &startPoint);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aRange->SetStart(startPoint.node, startPoint.idx);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aStartHTOffset == aEndHTOffset)
    return aRange->SetEnd(startPoint.node, startPoint.idx);

  rv = GetDOMPointByFrameOffset(endFrame, endOffset, endAcc, &endPoint);
  NS_ENSURE_SUCCESS(rv, rv);

  return aRange->SetEnd(endPoint.node, endPoint.idx);
}

int32_t
HyperTextAccessible::GetRelativeOffset(nsIPresShell* aPresShell,
                                       nsIFrame* aFromFrame,
                                       int32_t aFromOffset,
                                       Accessible* aFromAccessible,
                                       nsSelectionAmount aAmount,
                                       nsDirection aDirection,
                                       bool aNeedsStart,
                                       EWordMovementType aWordMovementType)
{
  const bool kIsJumpLinesOk = true;          
  const bool kIsScrollViewAStop = false;     
  const bool kIsKeyboardSelect = true;       
  const bool kIsVisualBidi = false;          

  

  nsresult rv;
  int32_t contentOffset = aFromOffset;
  nsIFrame *frame = aFromAccessible->GetFrame();
  NS_ENSURE_TRUE(frame, -1);

  if (frame->GetType() == nsGkAtoms::textFrame) {
    rv = RenderedToContentOffset(frame, aFromOffset, &contentOffset);
    NS_ENSURE_SUCCESS(rv, -1);
  }

  nsPeekOffsetStruct pos(aAmount, aDirection, contentOffset,
                         0, kIsJumpLinesOk, kIsScrollViewAStop, kIsKeyboardSelect, kIsVisualBidi,
                         aWordMovementType);
  rv = aFromFrame->PeekOffset(&pos);
  if (NS_FAILED(rv)) {
    if (aDirection == eDirPrevious) {
      
      
      
      
      pos.mResultContent = aFromFrame->GetContent();
      int32_t endOffsetUnused;
      aFromFrame->GetOffsets(pos.mContentOffset, endOffsetUnused);
    }
    else {
      return -1;
    }
  }

  
  int32_t hyperTextOffset;
  if (!pos.mResultContent)
    return -1;

  
  
  Accessible* finalAccessible =
    DOMPointToHypertextOffset(pos.mResultContent, pos.mContentOffset,
                              &hyperTextOffset, aDirection == eDirNext);

  if (!finalAccessible && aDirection == eDirPrevious) {
    
    
    hyperTextOffset = 0;
  }  
  else if (aAmount == eSelectBeginLine) {
    Accessible* firstChild = mChildren.SafeElementAt(0, nullptr);
    
    if (pos.mContentOffset == 0 && firstChild &&
        firstChild->Role() == roles::STATICTEXT &&
        static_cast<int32_t>(nsAccUtils::TextLength(firstChild)) == hyperTextOffset) {
      
      hyperTextOffset = 0;
    }
    if (!aNeedsStart && hyperTextOffset > 0) {
      -- hyperTextOffset;
    }
  }
  else if (aAmount == eSelectEndLine && finalAccessible) { 
    
    
    if (finalAccessible->Role() == roles::WHITESPACE) {  
      
      
      
      
      ++ hyperTextOffset;  
    }
    
    if (!aNeedsStart) {
      -- hyperTextOffset;
    }
  }

  return hyperTextOffset;
}

int32_t
HyperTextAccessible::FindWordBoundary(int32_t aOffset, nsDirection aDirection,
                                      EWordMovementType aWordMovementType)
{
  
  int32_t offsetInFrame = aOffset, notUsedOffset = aOffset;
  nsRefPtr<Accessible> accAtOffset;
  nsIFrame* frameAtOffset =
    GetPosAndText(offsetInFrame, notUsedOffset, nullptr, nullptr,
                  nullptr, getter_AddRefs(accAtOffset));
  if (!frameAtOffset) {
    if (aOffset == CharacterCount()) {
      
      if (accAtOffset)
        frameAtOffset = accAtOffset->GetFrame();
    }
    NS_ASSERTION(frameAtOffset, "No start frame for text getting!");
    if (!frameAtOffset)
      return -1;

    
    frameAtOffset = frameAtOffset->GetLastContinuation();
  }

  
  return GetRelativeOffset(mDoc->PresShell(), frameAtOffset, offsetInFrame,
                           accAtOffset, eSelectWord, aDirection,
                           (aWordMovementType == eStartWord),
                           aWordMovementType);
}










nsresult
HyperTextAccessible::GetTextHelper(EGetTextType aType,
                                   AccessibleTextBoundary aBoundaryType,
                                   int32_t aOffset,
                                   int32_t* aStartOffset, int32_t* aEndOffset,
                                   nsAString& aText)
{
  aText.Truncate();

  NS_ENSURE_ARG_POINTER(aStartOffset);
  NS_ENSURE_ARG_POINTER(aEndOffset);
  *aStartOffset = *aEndOffset = 0;

  int32_t offset = ConvertMagicOffset(aOffset);
  if (offset < 0)
    return NS_ERROR_INVALID_ARG;

  if (aOffset == nsIAccessibleText::TEXT_OFFSET_CARET && offset > 0 &&
      (aBoundaryType == BOUNDARY_LINE_START ||
       aBoundaryType == BOUNDARY_LINE_END)) {
    
    
    
    
    
    nsRefPtr<nsFrameSelection> frameSelection = FrameSelection();
    if (frameSelection &&
        frameSelection->GetHint() == nsFrameSelection::HINTLEFT) {
      -- offset;  
    }
  }

  nsSelectionAmount amount;
  bool needsStart = false;
  switch (aBoundaryType) {
    case BOUNDARY_WORD_START:
      needsStart = true;
      amount = eSelectWord;
      break;

    case BOUNDARY_WORD_END:
      amount = eSelectWord;
      break;

    case BOUNDARY_LINE_START:
      
      
      
      needsStart = true;
      amount = eSelectLine;
      break;

    case BOUNDARY_LINE_END:
      
      
      
      amount = eSelectLine;
      break;

    case BOUNDARY_ATTRIBUTE_RANGE:
    {
      nsresult rv = GetTextAttributes(false, offset,
                                      aStartOffset, aEndOffset, nullptr);
      NS_ENSURE_SUCCESS(rv, rv);
      
      return GetText(*aStartOffset, *aEndOffset, aText);
    }

    default:  
      return NS_ERROR_INVALID_ARG;
  }

  int32_t startOffset = offset + (aBoundaryType == BOUNDARY_LINE_END);  
  int32_t endOffset = startOffset;

  
  nsRefPtr<Accessible> startAcc;
  nsIFrame *startFrame = GetPosAndText(startOffset, endOffset, nullptr, nullptr,
                                       nullptr, getter_AddRefs(startAcc));

  if (!startFrame) {
    int32_t textLength = CharacterCount();
    if (aBoundaryType == BOUNDARY_LINE_START && offset > 0 && offset == textLength) {
      
      if (startAcc)
        startFrame = startAcc->GetFrame();
    }
    if (!startFrame) {
      return offset > textLength ? NS_ERROR_FAILURE : NS_OK;
    }
    else {
      
      startFrame = startFrame->GetLastContinuation();
    }
  }

  int32_t finalStartOffset = 0, finalEndOffset = 0;
  EWordMovementType wordMovementType = needsStart ? eStartWord : eEndWord;

  nsIPresShell* presShell = mDoc->PresShell();
  
  
  if (aType == eGetAfter) {
    finalStartOffset = offset;
  }
  else {
    finalStartOffset = GetRelativeOffset(presShell, startFrame, startOffset,
                                         startAcc,
                                         (amount == eSelectLine ? eSelectBeginLine : amount),
                                         eDirPrevious, needsStart,
                                         wordMovementType);
    NS_ENSURE_TRUE(finalStartOffset >= 0, NS_ERROR_FAILURE);
  }

  if (aType == eGetBefore) {
    finalEndOffset = offset;
  }
  else {
    
    
    
    
    startOffset = endOffset = finalStartOffset + (aBoundaryType == BOUNDARY_LINE_END);
    nsRefPtr<Accessible> endAcc;
    nsIFrame *endFrame = GetPosAndText(startOffset, endOffset, nullptr, nullptr,
                                       nullptr, getter_AddRefs(endAcc));
    if (endAcc && endAcc->Role() == roles::STATICTEXT) {
      
      
      startOffset = endOffset = finalStartOffset +
                                (aBoundaryType == BOUNDARY_LINE_END) +
                                nsAccUtils::TextLength(endAcc);

      endFrame = GetPosAndText(startOffset, endOffset, nullptr, nullptr,
                               nullptr, getter_AddRefs(endAcc));
    }
    if (!endFrame) {
      return NS_ERROR_FAILURE;
    }
    finalEndOffset = GetRelativeOffset(presShell, endFrame, endOffset, endAcc,
                                       (amount == eSelectLine ? eSelectEndLine : amount),
                                       eDirNext, needsStart, wordMovementType);
    NS_ENSURE_TRUE(endOffset >= 0, NS_ERROR_FAILURE);
    if (finalEndOffset == offset) {
      if (aType == eGetAt && amount == eSelectWord) { 
        
        
        
        return GetTextHelper(eGetAfter, aBoundaryType, offset,
                             aStartOffset, aEndOffset, aText);
      }
      int32_t textLength = CharacterCount();
      if (finalEndOffset < textLength) {
        
        
        
        ++ finalEndOffset;
      }
    }
  }

  *aStartOffset = finalStartOffset;
  *aEndOffset = finalEndOffset;

  NS_ASSERTION((finalStartOffset < offset && finalEndOffset >= offset) || aType != eGetBefore, "Incorrect results for GetTextHelper");
  NS_ASSERTION((finalStartOffset <= offset && finalEndOffset > offset) || aType == eGetBefore, "Incorrect results for GetTextHelper");

  GetPosAndText(finalStartOffset, finalEndOffset, &aText);
  return NS_OK;
}




NS_IMETHODIMP
HyperTextAccessible::GetTextBeforeOffset(int32_t aOffset,
                                         AccessibleTextBoundary aBoundaryType,
                                         int32_t* aStartOffset,
                                         int32_t* aEndOffset, nsAString& aText)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  int32_t offset = ConvertMagicOffset(aOffset);
  if (offset < 0)
    return NS_ERROR_INVALID_ARG;

  switch (aBoundaryType) {
    case BOUNDARY_CHAR:
      GetCharAt(offset, eGetBefore, aText, aStartOffset, aEndOffset);
      return NS_OK;

    case BOUNDARY_WORD_START: {
      
      
      
      if (offset == CharacterCount()) {
        *aEndOffset = FindWordBoundary(offset, eDirPrevious, eStartWord);
        *aStartOffset = FindWordBoundary(*aEndOffset, eDirPrevious, eStartWord);
      } else {
        *aStartOffset = FindWordBoundary(offset, eDirPrevious, eStartWord);
        *aEndOffset = FindWordBoundary(*aStartOffset, eDirNext, eStartWord);
        if (*aEndOffset != offset) {
          *aEndOffset = *aStartOffset;
          *aStartOffset = FindWordBoundary(*aEndOffset, eDirPrevious, eStartWord);
        }
      }
      return GetText(*aStartOffset, *aEndOffset, aText);
    }

    case BOUNDARY_WORD_END: {
      
      *aEndOffset = FindWordBoundary(offset, eDirPrevious, eEndWord);
      *aStartOffset = FindWordBoundary(*aEndOffset, eDirPrevious, eEndWord);
      return GetText(*aStartOffset, *aEndOffset, aText);
    }

    case BOUNDARY_LINE_START:
    case BOUNDARY_LINE_END:
    case BOUNDARY_ATTRIBUTE_RANGE:
      return GetTextHelper(eGetBefore, aBoundaryType, aOffset,
                           aStartOffset, aEndOffset, aText);

    default:
      return NS_ERROR_INVALID_ARG;
  }
}

NS_IMETHODIMP
HyperTextAccessible::GetTextAtOffset(int32_t aOffset,
                                     AccessibleTextBoundary aBoundaryType,
                                     int32_t* aStartOffset,
                                     int32_t* aEndOffset, nsAString& aText)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  int32_t offset = ConvertMagicOffset(aOffset);
  if (offset < 0)
    return NS_ERROR_INVALID_ARG;

  switch (aBoundaryType) {
    case BOUNDARY_CHAR:
      return GetCharAt(aOffset, eGetAt, aText, aStartOffset, aEndOffset) ?
        NS_OK : NS_ERROR_INVALID_ARG;

    case BOUNDARY_WORD_START:
      *aEndOffset = FindWordBoundary(offset, eDirNext, eStartWord);
      *aStartOffset = FindWordBoundary(*aEndOffset, eDirPrevious, eStartWord);
      return GetText(*aStartOffset, *aEndOffset, aText);

    case BOUNDARY_WORD_END:
      
      
      
      *aEndOffset = FindWordBoundary(offset, eDirNext, eEndWord);
      *aStartOffset = FindWordBoundary(*aEndOffset, eDirPrevious, eEndWord);
      return GetText(*aStartOffset, *aEndOffset, aText);

    case BOUNDARY_LINE_START:
    case BOUNDARY_LINE_END:
    case BOUNDARY_ATTRIBUTE_RANGE:
      return GetTextHelper(eGetAt, aBoundaryType, aOffset,
                           aStartOffset, aEndOffset, aText);

    default:
      return NS_ERROR_INVALID_ARG;
  }
}

NS_IMETHODIMP
HyperTextAccessible::GetTextAfterOffset(int32_t aOffset,
                                        AccessibleTextBoundary aBoundaryType,
                                        int32_t* aStartOffset,
                                        int32_t* aEndOffset, nsAString& aText)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  int32_t offset = ConvertMagicOffset(aOffset);
  if (offset < 0)
    return NS_ERROR_INVALID_ARG;

  switch (aBoundaryType) {
    case BOUNDARY_CHAR:
      GetCharAt(aOffset, eGetAfter, aText, aStartOffset, aEndOffset);
      return NS_OK;

    case BOUNDARY_WORD_START:
      
      *aStartOffset = FindWordBoundary(offset, eDirNext, eStartWord);
      *aEndOffset = FindWordBoundary(*aStartOffset, eDirNext, eStartWord);
      return GetText(*aStartOffset, *aEndOffset, aText);

    case BOUNDARY_WORD_END:
      
      
      
      if (offset == 0) {
        *aStartOffset = FindWordBoundary(offset, eDirNext, eEndWord);
        *aEndOffset = FindWordBoundary(*aStartOffset, eDirNext, eEndWord);
      } else {
        *aEndOffset = FindWordBoundary(offset, eDirNext, eEndWord);
        *aStartOffset = FindWordBoundary(*aEndOffset, eDirPrevious, eEndWord);
        if (*aStartOffset != offset) {
          *aStartOffset = *aEndOffset;
          *aEndOffset = FindWordBoundary(*aStartOffset, eDirNext, eEndWord);
        }
      }
      return GetText(*aStartOffset, *aEndOffset, aText);

    case BOUNDARY_LINE_START:
    case BOUNDARY_LINE_END:
    case BOUNDARY_ATTRIBUTE_RANGE:
      return GetTextHelper(eGetAfter, aBoundaryType, aOffset,
                           aStartOffset, aEndOffset, aText);

    default:
      return NS_ERROR_INVALID_ARG;
  }
}






NS_IMETHODIMP
HyperTextAccessible::GetTextAttributes(bool aIncludeDefAttrs,
                                       int32_t aOffset,
                                       int32_t* aStartOffset,
                                       int32_t* aEndOffset,
                                       nsIPersistentProperties** aAttributes)
{
  
  
  
  

  NS_ENSURE_ARG_POINTER(aStartOffset);
  *aStartOffset = 0;

  NS_ENSURE_ARG_POINTER(aEndOffset);
  *aEndOffset = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  int32_t offset = ConvertMagicOffset(aOffset);

  if (aAttributes) {
    *aAttributes = nullptr;

    nsCOMPtr<nsIPersistentProperties> attributes =
      do_CreateInstance(NS_PERSISTENTPROPERTIES_CONTRACTID);
    NS_ENSURE_TRUE(attributes, NS_ERROR_OUT_OF_MEMORY);

    NS_ADDREF(*aAttributes = attributes);
  }

  Accessible* accAtOffset = GetChildAtOffset(offset);
  if (!accAtOffset) {
    
    
    if (offset == 0) {
      if (aIncludeDefAttrs) {
        TextAttrsMgr textAttrsMgr(this);
        textAttrsMgr.GetAttributes(*aAttributes);
      }
      return NS_OK;
    }
    return NS_ERROR_INVALID_ARG;
  }

  int32_t accAtOffsetIdx = accAtOffset->IndexInParent();
  int32_t startOffset = GetChildOffset(accAtOffsetIdx);
  int32_t endOffset = GetChildOffset(accAtOffsetIdx + 1);
  int32_t offsetInAcc = offset - startOffset;

  TextAttrsMgr textAttrsMgr(this, aIncludeDefAttrs, accAtOffset,
                            accAtOffsetIdx);
  textAttrsMgr.GetAttributes(*aAttributes, &startOffset, &endOffset);

  
  nsIFrame *offsetFrame = accAtOffset->GetFrame();
  if (offsetFrame && offsetFrame->GetType() == nsGkAtoms::textFrame) {
    int32_t nodeOffset = 0;
    nsresult rv = RenderedToContentOffset(offsetFrame, offsetInAcc,
                                          &nodeOffset);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = GetSpellTextAttribute(accAtOffset->GetNode(), nodeOffset,
                               &startOffset, &endOffset,
                               aAttributes ? *aAttributes : nullptr);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  *aStartOffset = startOffset;
  *aEndOffset = endOffset;
  return NS_OK;
}



NS_IMETHODIMP
HyperTextAccessible::GetDefaultTextAttributes(nsIPersistentProperties** aAttributes)
{
  NS_ENSURE_ARG_POINTER(aAttributes);
  *aAttributes = nullptr;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIPersistentProperties> attributes =
    do_CreateInstance(NS_PERSISTENTPROPERTIES_CONTRACTID);
  NS_ENSURE_TRUE(attributes, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*aAttributes = attributes);

  TextAttrsMgr textAttrsMgr(this);
  textAttrsMgr.GetAttributes(*aAttributes);
  return NS_OK;
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




NS_IMETHODIMP
HyperTextAccessible::GetCharacterExtents(int32_t aOffset, int32_t* aX, int32_t* aY,
                                         int32_t* aWidth, int32_t* aHeight,
                                         uint32_t aCoordType)
{
  return GetRangeExtents(aOffset, aOffset + 1, aX, aY, aWidth, aHeight, aCoordType);
}




NS_IMETHODIMP
HyperTextAccessible::GetRangeExtents(int32_t aStartOffset, int32_t aEndOffset,
                                     int32_t* aX, int32_t* aY,
                                     int32_t* aWidth, int32_t* aHeight,
                                     uint32_t aCoordType)
{
  nsIntRect boundsRect;
  nsIFrame *endFrameUnused;
  if (!GetPosAndText(aStartOffset, aEndOffset, nullptr, &endFrameUnused, &boundsRect) ||
      boundsRect.IsEmpty()) {
    return NS_ERROR_FAILURE;
  }

  *aX = boundsRect.x;
  *aY = boundsRect.y;
  *aWidth = boundsRect.width;
  *aHeight = boundsRect.height;

  nsAccUtils::ConvertScreenCoordsTo(aX, aY, aCoordType, this);
  return NS_OK;
}





NS_IMETHODIMP
HyperTextAccessible::GetOffsetAtPoint(int32_t aX, int32_t aY,
                                      uint32_t aCoordType, int32_t* aOffset)
{
  *aOffset = -1;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsIFrame *hyperFrame = GetFrame();
  if (!hyperFrame) {
    return NS_ERROR_FAILURE;
  }

  nsIntPoint coords = nsAccUtils::ConvertToScreenCoords(aX, aY, aCoordType,
                                                        this);

  nsPresContext* presContext = mDoc->PresContext();
  nsPoint coordsInAppUnits =
    coords.ToAppUnits(presContext->AppUnitsPerDevPixel());

  nsRect frameScreenRect = hyperFrame->GetScreenRectInAppUnits();
  if (!frameScreenRect.Contains(coordsInAppUnits.x, coordsInAppUnits.y))
    return NS_OK;   

  nsPoint pointInHyperText(coordsInAppUnits.x - frameScreenRect.x,
                           coordsInAppUnits.y - frameScreenRect.y);

  
  

  
  
  int32_t offset = 0;
  uint32_t childCount = ChildCount();
  for (uint32_t childIdx = 0; childIdx < childCount; childIdx++) {
    Accessible* childAcc = mChildren[childIdx];

    nsIFrame *primaryFrame = childAcc->GetFrame();
    NS_ENSURE_TRUE(primaryFrame, NS_ERROR_FAILURE);

    nsIFrame *frame = primaryFrame;
    while (frame) {
      nsIContent *content = frame->GetContent();
      NS_ENSURE_TRUE(content, NS_ERROR_FAILURE);
      nsPoint pointInFrame = pointInHyperText - frame->GetOffsetTo(hyperFrame);
      nsSize frameSize = frame->GetSize();
      if (pointInFrame.x < frameSize.width && pointInFrame.y < frameSize.height) {
        
        if (frame->GetType() == nsGkAtoms::textFrame) {
          nsIFrame::ContentOffsets contentOffsets =
            frame->GetContentOffsetsFromPointExternal(pointInFrame, nsIFrame::IGNORE_SELECTION_STYLE);
          if (contentOffsets.IsNull() || contentOffsets.content != content) {
            return NS_OK; 
          }
          uint32_t addToOffset;
          nsresult rv = ContentToRenderedOffset(primaryFrame,
                                                contentOffsets.offset,
                                                &addToOffset);
          NS_ENSURE_SUCCESS(rv, rv);
          offset += addToOffset;
        }
        *aOffset = offset;
        return NS_OK;
      }
      frame = frame->GetNextContinuation();
    }

    offset += nsAccUtils::TextLength(childAcc);
  }

  return NS_OK; 
}





NS_IMETHODIMP
HyperTextAccessible::GetLinkCount(int32_t* aLinkCount)
{
  NS_ENSURE_ARG_POINTER(aLinkCount);
  *aLinkCount = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  *aLinkCount = GetLinkCount();
  return NS_OK;
}

NS_IMETHODIMP
HyperTextAccessible::GetLinkAt(int32_t aIndex, nsIAccessibleHyperLink** aLink)
{
  NS_ENSURE_ARG_POINTER(aLink);
  *aLink = nullptr;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  Accessible* link = GetLinkAt(aIndex);
  if (link)
    CallQueryInterface(link, aLink);

  return NS_OK;
}

NS_IMETHODIMP
HyperTextAccessible::GetLinkIndex(nsIAccessibleHyperLink* aLink,
                                  int32_t* aIndex)
{
  NS_ENSURE_ARG_POINTER(aLink);

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsRefPtr<Accessible> link(do_QueryObject(aLink));
  *aIndex = GetLinkIndex(link);
  return NS_OK;
}

NS_IMETHODIMP
HyperTextAccessible::GetLinkIndexAtOffset(int32_t aOffset, int32_t* aLinkIndex)
{
  NS_ENSURE_ARG_POINTER(aLinkIndex);
  *aLinkIndex = -1; 

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  *aLinkIndex = GetLinkIndexAtOffset(aOffset);
  return NS_OK;
}




NS_IMETHODIMP
HyperTextAccessible::SetAttributes(int32_t aStartPos, int32_t aEndPos,
                                   nsISupports* aAttributes)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
HyperTextAccessible::SetTextContents(const nsAString& aText)
{
  int32_t numChars = CharacterCount();
  if (numChars == 0 || NS_SUCCEEDED(DeleteText(0, numChars))) {
    return InsertText(aText, 0);
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
HyperTextAccessible::InsertText(const nsAString& aText, int32_t aPosition)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIEditor> editor = GetEditor();

  nsCOMPtr<nsIPlaintextEditor> peditor(do_QueryInterface(editor));
  NS_ENSURE_STATE(peditor);

  nsresult rv = SetSelectionRange(aPosition, aPosition);
  NS_ENSURE_SUCCESS(rv, rv);

  return peditor->InsertText(aText);
}

NS_IMETHODIMP
HyperTextAccessible::CopyText(int32_t aStartPos, int32_t aEndPos)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIEditor> editor = GetEditor();
  NS_ENSURE_STATE(editor);

  nsresult rv = SetSelectionRange(aStartPos, aEndPos);
  NS_ENSURE_SUCCESS(rv, rv);

  return editor->Copy();
}

NS_IMETHODIMP
HyperTextAccessible::CutText(int32_t aStartPos, int32_t aEndPos)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIEditor> editor = GetEditor();
  NS_ENSURE_STATE(editor);

  nsresult rv = SetSelectionRange(aStartPos, aEndPos);
  NS_ENSURE_SUCCESS(rv, rv);

  return editor->Cut();
}

NS_IMETHODIMP
HyperTextAccessible::DeleteText(int32_t aStartPos, int32_t aEndPos)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIEditor> editor = GetEditor();
  NS_ENSURE_STATE(editor);

  nsresult rv = SetSelectionRange(aStartPos, aEndPos);
  NS_ENSURE_SUCCESS(rv, rv);

  return editor->DeleteSelection(nsIEditor::eNone, nsIEditor::eStrip);
}

NS_IMETHODIMP
HyperTextAccessible::PasteText(int32_t aPosition)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIEditor> editor = GetEditor();
  NS_ENSURE_STATE(editor);

  nsresult rv = SetSelectionRange(aPosition, aPosition);
  NS_ENSURE_SUCCESS(rv, rv);

  return editor->Paste(nsIClipboard::kGlobalClipboard);
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

  
  SetSelectionBounds(0, aStartPos, aEndPos);

  
  
  nsRefPtr<nsFrameSelection> frameSelection = FrameSelection();
  NS_ENSURE_STATE(frameSelection);

  Selection* domSel =
    frameSelection->GetSelection(nsISelectionController::SELECTION_NORMAL);
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

NS_IMETHODIMP
HyperTextAccessible::SetCaretOffset(int32_t aCaretOffset)
{
  return SetSelectionRange(aCaretOffset, aCaretOffset);
}




NS_IMETHODIMP
HyperTextAccessible::GetCaretOffset(int32_t* aCaretOffset)
{
  NS_ENSURE_ARG_POINTER(aCaretOffset);
  *aCaretOffset = -1;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  
  
  if (!IsDoc() && !FocusMgr()->IsFocused(this) &&
      (InteractiveState() & states::FOCUSABLE)) {
    return NS_OK;
  }

  
  
  FocusManager::FocusDisposition focusDisp =
    FocusMgr()->IsInOrContainsFocus(this);
  if (focusDisp == FocusManager::eNone)
    return NS_OK;

  
  
  nsRefPtr<nsFrameSelection> frameSelection = FrameSelection();
  NS_ENSURE_STATE(frameSelection);

  Selection* domSel =
    frameSelection->GetSelection(nsISelectionController::SELECTION_NORMAL);
  NS_ENSURE_STATE(domSel);

  nsINode* focusNode = domSel->GetFocusNode();
  int32_t focusOffset = domSel->GetFocusOffset();

  
  
  if (focusDisp == FocusManager::eContainedByFocus) {
    nsINode *resultNode =
      nsCoreUtils::GetDOMNodeFromDOMPoint(focusNode, focusOffset);

    nsINode* thisNode = GetNode();
    if (resultNode != thisNode &&
        !nsCoreUtils::IsAncestorOf(thisNode, resultNode))
      return NS_OK;
  }

  DOMPointToHypertextOffset(focusNode, focusOffset, aCaretOffset);
  return NS_OK;
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
  int32_t caretOffset = domSel->GetFocusOffset();
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

already_AddRefed<nsFrameSelection>
HyperTextAccessible::FrameSelection()
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




NS_IMETHODIMP
HyperTextAccessible::GetSelectionCount(int32_t* aSelectionCount)
{
  NS_ENSURE_ARG_POINTER(aSelectionCount);
  *aSelectionCount = 0;

  nsTArray<nsRange*> ranges;
  GetSelectionDOMRanges(nsISelectionController::SELECTION_NORMAL, &ranges);
  *aSelectionCount = int32_t(ranges.Length());

  return NS_OK;
}




NS_IMETHODIMP
HyperTextAccessible::GetSelectionBounds(int32_t aSelectionNum,
                                        int32_t* aStartOffset,
                                        int32_t* aEndOffset)
{
  NS_ENSURE_ARG_POINTER(aStartOffset);
  NS_ENSURE_ARG_POINTER(aEndOffset);
  *aStartOffset = *aEndOffset = 0;

  nsTArray<nsRange*> ranges;
  GetSelectionDOMRanges(nsISelectionController::SELECTION_NORMAL, &ranges);

  uint32_t rangeCount = ranges.Length();
  if (aSelectionNum < 0 || aSelectionNum >= rangeCount)
    return NS_ERROR_INVALID_ARG;

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
  return NS_OK;
}




NS_IMETHODIMP
HyperTextAccessible::SetSelectionBounds(int32_t aSelectionNum,
                                        int32_t aStartOffset,
                                        int32_t aEndOffset)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  if (aSelectionNum < 0)
    return NS_ERROR_INVALID_ARG;

  int32_t startOffset = ConvertMagicOffset(aStartOffset);
  int32_t endOffset = ConvertMagicOffset(aEndOffset);

  nsRefPtr<nsFrameSelection> frameSelection = FrameSelection();
  NS_ENSURE_STATE(frameSelection);

  Selection* domSel =
    frameSelection->GetSelection(nsISelectionController::SELECTION_NORMAL);
  NS_ENSURE_STATE(domSel);

  uint32_t rangeCount = domSel->GetRangeCount();
  if (rangeCount < static_cast<uint32_t>(aSelectionNum))
    return NS_ERROR_INVALID_ARG;

  nsRefPtr<nsRange> range;
  if (aSelectionNum == rangeCount)
    range = new nsRange(mContent);
  else
    range = domSel->GetRangeAt(aSelectionNum);

  nsresult rv = HypertextOffsetsToDOMRange(startOffset, endOffset, range);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (aSelectionNum == rangeCount)
    return domSel->AddRange(range);

  domSel->RemoveRange(range);
  domSel->AddRange(range);
  return NS_OK;
}




NS_IMETHODIMP
HyperTextAccessible::AddSelection(int32_t aStartOffset, int32_t aEndOffset)
{
  nsRefPtr<nsFrameSelection> frameSelection = FrameSelection();
  NS_ENSURE_STATE(frameSelection);

  Selection* domSel =
    frameSelection->GetSelection(nsISelectionController::SELECTION_NORMAL);
  NS_ENSURE_STATE(domSel);

  return SetSelectionBounds(domSel->GetRangeCount(), aStartOffset, aEndOffset);
}




NS_IMETHODIMP
HyperTextAccessible::RemoveSelection(int32_t aSelectionNum)
{
  nsRefPtr<nsFrameSelection> frameSelection = FrameSelection();
  NS_ENSURE_STATE(frameSelection);

  Selection* domSel =
    frameSelection->GetSelection(nsISelectionController::SELECTION_NORMAL);
  NS_ENSURE_STATE(domSel);

  if (aSelectionNum < 0 || aSelectionNum >= domSel->GetRangeCount())
    return NS_ERROR_INVALID_ARG;

  return domSel->RemoveRange(domSel->GetRangeAt(aSelectionNum));
}




NS_IMETHODIMP
HyperTextAccessible::ScrollSubstringTo(int32_t aStartIndex, int32_t aEndIndex,
                                       uint32_t aScrollType)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsRefPtr<nsRange> range = new nsRange(mContent);
  nsresult rv = HypertextOffsetsToDOMRange(aStartIndex, aEndIndex, range);
  NS_ENSURE_SUCCESS(rv, rv);

  return nsCoreUtils::ScrollSubstringTo(GetFrame(), range, aScrollType);
}





NS_IMETHODIMP
HyperTextAccessible::ScrollSubstringToPoint(int32_t aStartIndex,
                                            int32_t aEndIndex,
                                            uint32_t aCoordinateType,
                                            int32_t aX, int32_t aY)
{
  nsIFrame *frame = GetFrame();
  if (!frame)
    return NS_ERROR_FAILURE;

  nsIntPoint coords = nsAccUtils::ConvertToScreenCoords(aX, aY, aCoordinateType,
                                                        this);

  nsRefPtr<nsRange> range = new nsRange(mContent);
  nsresult rv = HypertextOffsetsToDOMRange(aStartIndex, aEndIndex, range);
  NS_ENSURE_SUCCESS(rv, rv);

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

        rv = nsCoreUtils::ScrollSubstringTo(frame, range, vPercent, hPercent);
        NS_ENSURE_SUCCESS(rv, rv);

        initialScrolled = true;
      } else {
        
        
        
        
        nsCoreUtils::ScrollFrameToPoint(parentFrame, frame, coords);
      }
    }
    frame = parentFrame;
  }

  return NS_OK;
}





ENameValueFlag
HyperTextAccessible::NativeName(nsString& aName)
{
  ENameValueFlag nameFlag = AccessibleWrap::NativeName(aName);
  if (!aName.IsEmpty())
    return nameFlag;

  
  
  
  if (IsAbbreviation() &&
      mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::title, aName))
    aName.CompressWhitespace();

  return eNameOK;
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




nsresult
HyperTextAccessible::ContentToRenderedOffset(nsIFrame* aFrame, int32_t aContentOffset,
                                             uint32_t* aRenderedOffset)
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
                                             int32_t* aContentOffset)
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




bool
HyperTextAccessible::GetCharAt(int32_t aOffset, EGetTextType aShift,
                               nsAString& aChar, int32_t* aStartOffset,
                               int32_t* aEndOffset)
{
  aChar.Truncate();

  int32_t offset = ConvertMagicOffset(aOffset) + static_cast<int32_t>(aShift);
  int32_t childIdx = GetChildIndexAtOffset(offset);
  if (childIdx == -1)
    return false;

  Accessible* child = GetChildAt(childIdx);
  child->AppendTextTo(aChar, offset - GetChildOffset(childIdx), 1);

  if (aStartOffset)
    *aStartOffset = offset;
  if (aEndOffset)
    *aEndOffset = aChar.IsEmpty() ? offset : offset + 1;

  return true;
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
