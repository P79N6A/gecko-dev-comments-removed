




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

using namespace mozilla;
using namespace mozilla::a11y;





HyperTextAccessible::
  HyperTextAccessible(nsIContent* aNode, DocAccessible* aDoc) :
  AccessibleWrap(aNode, aDoc)
{
  mFlags |= eHyperTextAccessible;
}

NS_IMPL_ADDREF_INHERITED(HyperTextAccessible, AccessibleWrap)
NS_IMPL_RELEASE_INHERITED(HyperTextAccessible, AccessibleWrap)

nsresult
HyperTextAccessible::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  *aInstancePtr = nsnull;

  
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

PRUint64
HyperTextAccessible::NativeState()
{
  PRUint64 states = AccessibleWrap::NativeState();

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
HyperTextAccessible::GetBoundsForString(nsIFrame* aFrame, PRUint32 aStartRenderedOffset,
                                        PRUint32 aEndRenderedOffset)
{
  nsIntRect screenRect;
  NS_ENSURE_TRUE(aFrame, screenRect);
  if (aFrame->GetType() != nsGkAtoms::textFrame) {
    
    
    return aFrame->GetScreenRectExternal();
  }

  PRInt32 startContentOffset, endContentOffset;
  nsresult rv = RenderedToContentOffset(aFrame, aStartRenderedOffset, &startContentOffset);
  NS_ENSURE_SUCCESS(rv, screenRect);
  rv = RenderedToContentOffset(aFrame, aEndRenderedOffset, &endContentOffset);
  NS_ENSURE_SUCCESS(rv, screenRect);

  nsIFrame *frame;
  PRInt32 startContentOffsetInFrame;
  
  
  rv = aFrame->GetChildFrameContainingOffset(startContentOffset, false,
                                             &startContentOffsetInFrame, &frame);
  NS_ENSURE_SUCCESS(rv, screenRect);

  nsPresContext* context = mDoc->PresContext();

  while (frame && startContentOffset < endContentOffset) {
    
    
    
    
    nsIntRect frameScreenRect = frame->GetScreenRectExternal();

    
    PRInt32 startFrameTextOffset, endFrameTextOffset;
    frame->GetOffsets(startFrameTextOffset, endFrameTextOffset);
    PRInt32 frameTotalTextLength = endFrameTextOffset - startFrameTextOffset;
    PRInt32 seekLength = endContentOffset - startContentOffset;
    PRInt32 frameSubStringLength = NS_MIN(frameTotalTextLength - startContentOffsetInFrame, seekLength);

    
    nsPoint frameTextStartPoint;
    rv = frame->GetPointFromOffset(startContentOffset, &frameTextStartPoint);
    NS_ENSURE_SUCCESS(rv, nsIntRect());
    frameScreenRect.x += context->AppUnitsToDevPixels(frameTextStartPoint.x);

    
    nsPoint frameTextEndPoint;
    rv = frame->GetPointFromOffset(startContentOffset + frameSubStringLength, &frameTextEndPoint);
    NS_ENSURE_SUCCESS(rv, nsIntRect());
    frameScreenRect.width = context->AppUnitsToDevPixels(frameTextEndPoint.x - frameTextStartPoint.x);

    screenRect.UnionRect(frameScreenRect, screenRect);

    
    startContentOffset += frameSubStringLength;
    startContentOffsetInFrame = 0;
    frame = frame->GetNextContinuation();
  }

  return screenRect;
}




nsIFrame*
HyperTextAccessible::GetPosAndText(PRInt32& aStartOffset, PRInt32& aEndOffset,
                                   nsAString* aText, nsIFrame** aEndFrame,
                                   nsIntRect* aBoundsRect,
                                   Accessible** aStartAcc,
                                   Accessible** aEndAcc)
{
  if (aStartOffset == nsIAccessibleText::TEXT_OFFSET_END_OF_TEXT) {
    aStartOffset = CharacterCount();
  }
  if (aStartOffset == nsIAccessibleText::TEXT_OFFSET_CARET) {
    GetCaretOffset(&aStartOffset);
  }
  if (aEndOffset == nsIAccessibleText::TEXT_OFFSET_END_OF_TEXT) {
    aEndOffset = CharacterCount();
  }
  if (aEndOffset == nsIAccessibleText::TEXT_OFFSET_CARET) {
    GetCaretOffset(&aEndOffset);
  }

  PRInt32 startOffset = aStartOffset;
  PRInt32 endOffset = aEndOffset;
  
  bool isPassword = (Role() == roles::PASSWORD_TEXT);

  
  if (aText) {
    aText->Truncate();
  }
  if (endOffset < 0) {
    const PRInt32 kMaxTextLength = 32767;
    endOffset = kMaxTextLength; 
  }
  else if (startOffset > endOffset) {
    return nsnull;
  }

  nsIFrame *startFrame = nsnull;
  if (aEndFrame) {
    *aEndFrame = nsnull;
  }
  if (aBoundsRect) {
    aBoundsRect->SetEmpty();
  }
  if (aStartAcc)
    *aStartAcc = nsnull;
  if (aEndAcc)
    *aEndAcc = nsnull;

  nsIntRect unionRect;
  Accessible* lastAccessible = nsnull;

  gfxSkipChars skipChars;
  gfxSkipCharsIterator iter;

  
  
  PRUint32 childCount = ChildCount();
  for (PRUint32 childIdx = 0; childIdx < childCount; childIdx++) {
    Accessible* childAcc = mChildren[childIdx];
    lastAccessible = childAcc;

    nsIFrame *frame = childAcc->GetFrame();
    if (!frame) {
      continue;
    }
    nsIFrame *primaryFrame = frame;
    if (nsAccUtils::IsText(childAcc)) {
      
      
      PRInt32 substringEndOffset = -1;
      PRUint32 ourRenderedStart = 0;
      PRInt32 ourContentStart = 0;
      if (frame->GetType() == nsGkAtoms::textFrame) {
        nsresult rv = frame->GetRenderedText(nsnull, &skipChars, &iter);
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
      if (startOffset < substringEndOffset) {
        
        if (startOffset > 0 || endOffset < substringEndOffset) {
          
          
          PRInt32 outStartLineUnused;
          PRInt32 contentOffset;
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
            for (PRInt32 count = startOffset; count < substringEndOffset; count ++)
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
            aBoundsRect->UnionRect(*aBoundsRect,
                                   frame->GetScreenRectExternal());
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
    *aEndFrame = startFrame;
    if (aStartAcc && aEndAcc)
      NS_IF_ADDREF(*aEndAcc = *aStartAcc);
  }

  return startFrame;
}

NS_IMETHODIMP
HyperTextAccessible::GetText(PRInt32 aStartOffset, PRInt32 aEndOffset,
                             nsAString& aText)
{
  aText.Truncate();

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  PRInt32 startOffset = ConvertMagicOffset(aStartOffset);
  PRInt32 endOffset = ConvertMagicOffset(aEndOffset);

  PRInt32 startChildIdx = GetChildIndexAtOffset(startOffset);
  if (startChildIdx == -1) {
    
    return (startOffset == 0 && endOffset == 0) ? NS_OK : NS_ERROR_INVALID_ARG;
  }

  PRInt32 endChildIdx = GetChildIndexAtOffset(endOffset);
  if (endChildIdx == -1)
    return NS_ERROR_INVALID_ARG;

  if (startChildIdx == endChildIdx) {
    PRInt32 childOffset =  GetChildOffset(startChildIdx);
    NS_ENSURE_STATE(childOffset != -1);

    Accessible* child = GetChildAt(startChildIdx);
    child->AppendTextTo(aText, startOffset - childOffset,
                        endOffset - startOffset);

    return NS_OK;
  }

  PRInt32 startChildOffset =  GetChildOffset(startChildIdx);
  NS_ENSURE_STATE(startChildOffset != -1);

  Accessible* startChild = GetChildAt(startChildIdx);
  startChild->AppendTextTo(aText, startOffset - startChildOffset);

  for (PRInt32 childIdx = startChildIdx + 1; childIdx < endChildIdx; childIdx++) {
    Accessible* child = GetChildAt(childIdx);
    child->AppendTextTo(aText);
  }

  PRInt32 endChildOffset =  GetChildOffset(endChildIdx);
  NS_ENSURE_STATE(endChildOffset != -1);

  Accessible* endChild = GetChildAt(endChildIdx);
  endChild->AppendTextTo(aText, 0, endOffset - endChildOffset);

  return NS_OK;
}




NS_IMETHODIMP
HyperTextAccessible::GetCharacterCount(PRInt32* aCharacterCount)
{
  NS_ENSURE_ARG_POINTER(aCharacterCount);
  *aCharacterCount = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  *aCharacterCount = CharacterCount();
  return NS_OK;
}




NS_IMETHODIMP
HyperTextAccessible::GetCharacterAtOffset(PRInt32 aOffset, PRUnichar* aCharacter)
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
                                               PRInt32 aNodeOffset,
                                               PRInt32* aHyperTextOffset,
                                               bool aIsEndOffset)
{
  if (!aHyperTextOffset)
    return nsnull;
  *aHyperTextOffset = 0;

  if (!aNode)
    return nsnull;

  PRUint32 addTextOffset = 0;
  nsINode* findNode = nsnull;

  if (aNodeOffset == -1) {
    findNode = aNode;

  } else if (aNode->IsNodeOfType(nsINode::eTEXT)) {
    
    
    
    nsIFrame *frame = aNode->AsContent()->GetPrimaryFrame();
    NS_ENSURE_TRUE(frame, nsnull);
    nsresult rv = ContentToRenderedOffset(frame, aNodeOffset, &addTextOffset);
    NS_ENSURE_SUCCESS(rv, nsnull);
    
    findNode = aNode;

  } else {
    
    
    
    
    
    
    
    

    findNode = aNode->GetChildAt(aNodeOffset);
    if (!findNode && !aNodeOffset) {
      if (aNode == GetNode()) {
        
        
        *aHyperTextOffset = 0;
        return nsnull;
      }
      findNode = aNode; 
    }
  }

  
  
  Accessible* descendantAcc = nsnull;
  if (findNode) {
    nsCOMPtr<nsIContent> findContent(do_QueryInterface(findNode));
    if (findContent && findContent->IsHTML() &&
        findContent->NodeInfo()->Equals(nsGkAtoms::br) &&
        findContent->AttrValueIs(kNameSpaceID_None,
                                 nsGkAtoms::mozeditorbogusnode,
                                 nsGkAtoms::_true,
                                 eIgnoreCase)) {
      
      *aHyperTextOffset = 0;
      return nsnull;
    }
    descendantAcc = GetFirstAvailableAccessible(findNode);
  }

  
  Accessible* childAccAtOffset = nsnull;
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

  
  
  
  
  PRUint32 childCount = ChildCount();

  PRUint32 childIdx = 0;
  Accessible* childAcc = nsnull;
  for (; childIdx < childCount; childIdx++) {
    childAcc = mChildren[childIdx];
    if (childAcc == childAccAtOffset)
      break;

    *aHyperTextOffset += nsAccUtils::TextLength(childAcc);
  }

  if (childIdx < childCount) {
    *aHyperTextOffset += addTextOffset;
    NS_ASSERTION(childAcc == childAccAtOffset,
                 "These should be equal whenever we exit loop and childAcc != nsnull");

    if (childIdx < childCount - 1 ||
        addTextOffset < nsAccUtils::TextLength(childAccAtOffset)) {
      
      return childAccAtOffset;
    }
  }

  return nsnull;
}

nsresult
HyperTextAccessible::HypertextOffsetsToDOMRange(PRInt32 aStartHTOffset,
                                                PRInt32 aEndHTOffset,
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
  PRInt32 startOffset = aStartHTOffset, endOffset = aEndHTOffset;
  nsIFrame *startFrame = nsnull, *endFrame = nsnull;

  startFrame = GetPosAndText(startOffset, endOffset, nsnull, &endFrame, nsnull,
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

PRInt32
HyperTextAccessible::GetRelativeOffset(nsIPresShell* aPresShell,
                                       nsIFrame* aFromFrame,
                                       PRInt32 aFromOffset,
                                       Accessible* aFromAccessible,
                                       nsSelectionAmount aAmount,
                                       nsDirection aDirection,
                                       bool aNeedsStart)
{
  const bool kIsJumpLinesOk = true;          
  const bool kIsScrollViewAStop = false;     
  const bool kIsKeyboardSelect = true;       
  const bool kIsVisualBidi = false;          

  EWordMovementType wordMovementType = aNeedsStart ? eStartWord : eEndWord;
  if (aAmount == eSelectLine) {
    aAmount = (aDirection == eDirNext) ? eSelectEndLine : eSelectBeginLine;
  }

  

  nsresult rv;
  PRInt32 contentOffset = aFromOffset;
  if (nsAccUtils::IsText(aFromAccessible)) {
    nsIFrame *frame = aFromAccessible->GetFrame();
    NS_ENSURE_TRUE(frame, -1);

    if (frame->GetType() == nsGkAtoms::textFrame) {
      rv = RenderedToContentOffset(frame, aFromOffset, &contentOffset);
      NS_ENSURE_SUCCESS(rv, -1);
    }
  }

  nsPeekOffsetStruct pos(aAmount, aDirection, contentOffset,
                         0, kIsJumpLinesOk, kIsScrollViewAStop, kIsKeyboardSelect, kIsVisualBidi,
                         wordMovementType);
  rv = aFromFrame->PeekOffset(&pos);
  if (NS_FAILED(rv)) {
    if (aDirection == eDirPrevious) {
      
      
      
      
      pos.mResultContent = aFromFrame->GetContent();
      PRInt32 endOffsetUnused;
      aFromFrame->GetOffsets(pos.mContentOffset, endOffsetUnused);
    }
    else {
      return -1;
    }
  }

  
  PRInt32 hyperTextOffset;
  if (!pos.mResultContent)
    return -1;

  
  
  Accessible* finalAccessible =
    DOMPointToHypertextOffset(pos.mResultContent, pos.mContentOffset,
                              &hyperTextOffset, aDirection == eDirNext);

  if (!finalAccessible && aDirection == eDirPrevious) {
    
    
    hyperTextOffset = 0;
  }  
  else if (aAmount == eSelectBeginLine) {
    Accessible* firstChild = mChildren.SafeElementAt(0, nsnull);
    
    if (pos.mContentOffset == 0 && firstChild &&
        firstChild->Role() == roles::STATICTEXT &&
        static_cast<PRInt32>(nsAccUtils::TextLength(firstChild)) == hyperTextOffset) {
      
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










nsresult
HyperTextAccessible::GetTextHelper(EGetTextType aType, AccessibleTextBoundary aBoundaryType,
                                   PRInt32 aOffset, PRInt32* aStartOffset, PRInt32* aEndOffset,
                                   nsAString& aText)
{
  aText.Truncate();

  NS_ENSURE_ARG_POINTER(aStartOffset);
  NS_ENSURE_ARG_POINTER(aEndOffset);
  *aStartOffset = *aEndOffset = 0;

  if (!mDoc)
    return NS_ERROR_FAILURE;

  nsIPresShell* presShell = mDoc->PresShell();
  if (!presShell) {
    return NS_ERROR_FAILURE;
  }

  if (aOffset == nsIAccessibleText::TEXT_OFFSET_END_OF_TEXT) {
    aOffset = CharacterCount();
  }
  if (aOffset == nsIAccessibleText::TEXT_OFFSET_CARET) {
    GetCaretOffset(&aOffset);
    if (aOffset > 0 && (aBoundaryType == BOUNDARY_LINE_START ||
                        aBoundaryType == BOUNDARY_LINE_END)) {
      
      
      
      
      nsRefPtr<nsFrameSelection> frameSelection = FrameSelection();
      if (frameSelection &&
          frameSelection->GetHint() == nsFrameSelection::HINTLEFT) {
        -- aOffset;  
      }
    }
  }
  else if (aOffset < 0) {
    return NS_ERROR_FAILURE;
  }

  nsSelectionAmount amount;
  bool needsStart = false;
  switch (aBoundaryType) {
    case BOUNDARY_CHAR:
      amount = eSelectCluster;
      if (aType == eGetAt)
        aType = eGetAfter; 
      break;

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
      nsresult rv = GetTextAttributes(false, aOffset,
                                      aStartOffset, aEndOffset, nsnull);
      NS_ENSURE_SUCCESS(rv, rv);
      
      return GetText(*aStartOffset, *aEndOffset, aText);
    }

    default:  
      return NS_ERROR_INVALID_ARG;
  }

  PRInt32 startOffset = aOffset + (aBoundaryType == BOUNDARY_LINE_END);  
  PRInt32 endOffset = startOffset;

  
  nsRefPtr<Accessible> startAcc;
  nsIFrame *startFrame = GetPosAndText(startOffset, endOffset, nsnull, nsnull,
                                       nsnull, getter_AddRefs(startAcc));

  if (!startFrame) {
    PRInt32 textLength = CharacterCount();
    if (aBoundaryType == BOUNDARY_LINE_START && aOffset > 0 && aOffset == textLength) {
      
      if (startAcc)
        startFrame = startAcc->GetFrame();
    }
    if (!startFrame) {
      return aOffset > textLength ? NS_ERROR_FAILURE : NS_OK;
    }
    else {
      
      startFrame = startFrame->GetLastContinuation();
    }
  }

  PRInt32 finalStartOffset, finalEndOffset;

  
  
  if (aType == eGetAfter) {
    finalStartOffset = aOffset;
  }
  else {
    finalStartOffset = GetRelativeOffset(presShell, startFrame, startOffset,
                                         startAcc, amount, eDirPrevious,
                                         needsStart);
    NS_ENSURE_TRUE(finalStartOffset >= 0, NS_ERROR_FAILURE);
  }

  if (aType == eGetBefore) {
    finalEndOffset = aOffset;
  }
  else {
    
    
    
    
    startOffset = endOffset = finalStartOffset + (aBoundaryType == BOUNDARY_LINE_END);
    nsRefPtr<Accessible> endAcc;
    nsIFrame *endFrame = GetPosAndText(startOffset, endOffset, nsnull, nsnull,
                                       nsnull, getter_AddRefs(endAcc));
    if (endAcc && endAcc->Role() == roles::STATICTEXT) {
      
      
      startOffset = endOffset = finalStartOffset +
                                (aBoundaryType == BOUNDARY_LINE_END) +
                                nsAccUtils::TextLength(endAcc);

      endFrame = GetPosAndText(startOffset, endOffset, nsnull, nsnull,
                               nsnull, getter_AddRefs(endAcc));
    }
    if (!endFrame) {
      return NS_ERROR_FAILURE;
    }
    finalEndOffset = GetRelativeOffset(presShell, endFrame, endOffset, endAcc,
                                       amount, eDirNext, needsStart);
    NS_ENSURE_TRUE(endOffset >= 0, NS_ERROR_FAILURE);
    if (finalEndOffset == aOffset) {
      if (aType == eGetAt && amount == eSelectWord) { 
        
        
        
        return GetTextHelper(eGetAfter, aBoundaryType, aOffset, aStartOffset, aEndOffset, aText);
      }
      PRInt32 textLength = CharacterCount();
      if (finalEndOffset < textLength) {
        
        
        
        ++ finalEndOffset;
      }
    }
  }

  *aStartOffset = finalStartOffset;
  *aEndOffset = finalEndOffset;

  NS_ASSERTION((finalStartOffset < aOffset && finalEndOffset >= aOffset) || aType != eGetBefore, "Incorrect results for GetTextHelper");
  NS_ASSERTION((finalStartOffset <= aOffset && finalEndOffset > aOffset) || aType == eGetBefore, "Incorrect results for GetTextHelper");

  GetPosAndText(finalStartOffset, finalEndOffset, &aText);
  return NS_OK;
}




NS_IMETHODIMP
HyperTextAccessible::GetTextBeforeOffset(PRInt32 aOffset,
                                         AccessibleTextBoundary aBoundaryType,
                                         PRInt32* aStartOffset,
                                         PRInt32* aEndOffset, nsAString& aText)
{
  if (aBoundaryType == BOUNDARY_CHAR) {
    GetCharAt(aOffset, eGetBefore, aText, aStartOffset, aEndOffset);
    return NS_OK;
  }

  return GetTextHelper(eGetBefore, aBoundaryType, aOffset, aStartOffset, aEndOffset, aText);
}

NS_IMETHODIMP
HyperTextAccessible::GetTextAtOffset(PRInt32 aOffset,
                                     AccessibleTextBoundary aBoundaryType,
                                     PRInt32* aStartOffset,
                                     PRInt32* aEndOffset, nsAString& aText)
{
  if (aBoundaryType == BOUNDARY_CHAR) {
    GetCharAt(aOffset, eGetAt, aText, aStartOffset, aEndOffset);
    return NS_OK;
  }

  return GetTextHelper(eGetAt, aBoundaryType, aOffset, aStartOffset, aEndOffset, aText);
}

NS_IMETHODIMP
HyperTextAccessible::GetTextAfterOffset(PRInt32 aOffset, AccessibleTextBoundary aBoundaryType,
                                        PRInt32* aStartOffset, PRInt32* aEndOffset, nsAString& aText)
{
  if (aBoundaryType == BOUNDARY_CHAR) {
    GetCharAt(aOffset, eGetAfter, aText, aStartOffset, aEndOffset);
    return NS_OK;
  }

  return GetTextHelper(eGetAfter, aBoundaryType, aOffset, aStartOffset, aEndOffset, aText);
}






NS_IMETHODIMP
HyperTextAccessible::GetTextAttributes(bool aIncludeDefAttrs,
                                       PRInt32 aOffset,
                                       PRInt32* aStartOffset,
                                       PRInt32* aEndOffset,
                                       nsIPersistentProperties** aAttributes)
{
  
  
  
  

  NS_ENSURE_ARG_POINTER(aStartOffset);
  *aStartOffset = 0;

  NS_ENSURE_ARG_POINTER(aEndOffset);
  *aEndOffset = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  if (aAttributes) {
    *aAttributes = nsnull;

    nsCOMPtr<nsIPersistentProperties> attributes =
      do_CreateInstance(NS_PERSISTENTPROPERTIES_CONTRACTID);
    NS_ENSURE_TRUE(attributes, NS_ERROR_OUT_OF_MEMORY);

    NS_ADDREF(*aAttributes = attributes);
  }

  Accessible* accAtOffset = GetChildAtOffset(aOffset);
  if (!accAtOffset) {
    
    
    if (aOffset == 0) {
      if (aIncludeDefAttrs) {
        TextAttrsMgr textAttrsMgr(this);
        textAttrsMgr.GetAttributes(*aAttributes);
      }
      return NS_OK;
    }
    return NS_ERROR_INVALID_ARG;
  }

  PRInt32 accAtOffsetIdx = accAtOffset->IndexInParent();
  PRInt32 startOffset = GetChildOffset(accAtOffsetIdx);
  PRInt32 endOffset = GetChildOffset(accAtOffsetIdx + 1);
  PRInt32 offsetInAcc = aOffset - startOffset;

  TextAttrsMgr textAttrsMgr(this, aIncludeDefAttrs, accAtOffset,
                            accAtOffsetIdx);
  textAttrsMgr.GetAttributes(*aAttributes, &startOffset, &endOffset);

  
  nsIFrame *offsetFrame = accAtOffset->GetFrame();
  if (offsetFrame && offsetFrame->GetType() == nsGkAtoms::textFrame) {
    PRInt32 nodeOffset = 0;
    nsresult rv = RenderedToContentOffset(offsetFrame, offsetInAcc,
                                          &nodeOffset);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = GetSpellTextAttribute(accAtOffset->GetNode(), nodeOffset,
                               &startOffset, &endOffset,
                               aAttributes ? *aAttributes : nsnull);
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
  *aAttributes = nsnull;

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

PRInt32
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

nsresult
HyperTextAccessible::GetAttributesInternal(nsIPersistentProperties* aAttributes)
{
  nsresult rv = AccessibleWrap::GetAttributesInternal(aAttributes);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  nsIFrame *frame = GetFrame();
  if (frame && frame->GetType() == nsGkAtoms::blockFrame) {
    nsAutoString oldValueUnused;
    aAttributes->SetStringProperty(NS_LITERAL_CSTRING("formatting"), NS_LITERAL_STRING("block"),
                                   oldValueUnused);
  }

  if (FocusMgr()->IsFocused(this)) {
    PRInt32 lineNumber = CaretLineNumber();
    if (lineNumber >= 1) {
      nsAutoString strLineNumber;
      strLineNumber.AppendInt(lineNumber);
      nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::lineNumber,
                             strLineNumber);
    }
  }

  
  
  
  
  if (mContent->Tag() == nsGkAtoms::nav)
    nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::xmlroles,
                           NS_LITERAL_STRING("navigation"));
  else if (mContent->Tag() == nsGkAtoms::section) 
    nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::xmlroles,
                           NS_LITERAL_STRING("region"));
  else if (mContent->Tag() == nsGkAtoms::footer) 
    nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::xmlroles,
                           NS_LITERAL_STRING("contentinfo"));
  else if (mContent->Tag() == nsGkAtoms::aside) 
    nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::xmlroles,
                           NS_LITERAL_STRING("complementary"));
  else if (mContent->Tag() == nsGkAtoms::article)
    nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::xmlroles,
                           NS_LITERAL_STRING("article"));

  return  NS_OK;
}




NS_IMETHODIMP
HyperTextAccessible::GetCharacterExtents(PRInt32 aOffset, PRInt32* aX, PRInt32* aY,
                                         PRInt32* aWidth, PRInt32* aHeight,
                                         PRUint32 aCoordType)
{
  return GetRangeExtents(aOffset, aOffset + 1, aX, aY, aWidth, aHeight, aCoordType);
}




NS_IMETHODIMP
HyperTextAccessible::GetRangeExtents(PRInt32 aStartOffset, PRInt32 aEndOffset,
                                     PRInt32* aX, PRInt32* aY,
                                     PRInt32* aWidth, PRInt32* aHeight,
                                     PRUint32 aCoordType)
{
  nsIntRect boundsRect;
  nsIFrame *endFrameUnused;
  if (!GetPosAndText(aStartOffset, aEndOffset, nsnull, &endFrameUnused, &boundsRect) ||
      boundsRect.IsEmpty()) {
    return NS_ERROR_FAILURE;
  }

  *aX = boundsRect.x;
  *aY = boundsRect.y;
  *aWidth = boundsRect.width;
  *aHeight = boundsRect.height;

  return nsAccUtils::ConvertScreenCoordsTo(aX, aY, aCoordType, this);
}





NS_IMETHODIMP
HyperTextAccessible::GetOffsetAtPoint(PRInt32 aX, PRInt32 aY,
                                      PRUint32 aCoordType, PRInt32* aOffset)
{
  *aOffset = -1;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsIFrame *hyperFrame = GetFrame();
  if (!hyperFrame) {
    return NS_ERROR_FAILURE;
  }
  nsIntRect frameScreenRect = hyperFrame->GetScreenRectExternal();

  nsIntPoint coords;
  nsresult rv = nsAccUtils::ConvertToScreenCoords(aX, aY, aCoordType,
                                                  this, &coords);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (!frameScreenRect.Contains(coords.x, coords.y)) {
    return NS_OK;   
  }
  nsIntPoint pxInHyperText(coords.x - frameScreenRect.x,
                           coords.y - frameScreenRect.y);
  nsPresContext* context = mDoc->PresContext();
  nsPoint pointInHyperText(context->DevPixelsToAppUnits(pxInHyperText.x),
                           context->DevPixelsToAppUnits(pxInHyperText.y));

  
  

  
  
  PRInt32 offset = 0;
  PRUint32 childCount = ChildCount();
  for (PRUint32 childIdx = 0; childIdx < childCount; childIdx++) {
    Accessible* childAcc = mChildren[childIdx];

    nsIFrame *primaryFrame = childAcc->GetFrame();
    NS_ENSURE_TRUE(primaryFrame, NS_ERROR_FAILURE);

    nsIFrame *frame = primaryFrame;
    while (frame) {
      nsIContent *content = frame->GetContent();
      NS_ENSURE_TRUE(content, NS_ERROR_FAILURE);
      nsPoint pointInFrame = pointInHyperText - frame->GetOffsetToExternal(hyperFrame);
      nsSize frameSize = frame->GetSize();
      if (pointInFrame.x < frameSize.width && pointInFrame.y < frameSize.height) {
        
        if (frame->GetType() == nsGkAtoms::textFrame) {
          nsIFrame::ContentOffsets contentOffsets =
            frame->GetContentOffsetsFromPointExternal(pointInFrame, nsIFrame::IGNORE_SELECTION_STYLE);
          if (contentOffsets.IsNull() || contentOffsets.content != content) {
            return NS_OK; 
          }
          PRUint32 addToOffset;
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
HyperTextAccessible::GetLinkCount(PRInt32* aLinkCount)
{
  NS_ENSURE_ARG_POINTER(aLinkCount);
  *aLinkCount = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  *aLinkCount = GetLinkCount();
  return NS_OK;
}

NS_IMETHODIMP
HyperTextAccessible::GetLinkAt(PRInt32 aIndex, nsIAccessibleHyperLink** aLink)
{
  NS_ENSURE_ARG_POINTER(aLink);
  *aLink = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  Accessible* link = GetLinkAt(aIndex);
  if (link)
    CallQueryInterface(link, aLink);

  return NS_OK;
}

NS_IMETHODIMP
HyperTextAccessible::GetLinkIndex(nsIAccessibleHyperLink* aLink,
                                  PRInt32* aIndex)
{
  NS_ENSURE_ARG_POINTER(aLink);

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsRefPtr<Accessible> link(do_QueryObject(aLink));
  *aIndex = GetLinkIndex(link);
  return NS_OK;
}

NS_IMETHODIMP
HyperTextAccessible::GetLinkIndexAtOffset(PRInt32 aOffset, PRInt32* aLinkIndex)
{
  NS_ENSURE_ARG_POINTER(aLinkIndex);
  *aLinkIndex = -1; 

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  *aLinkIndex = GetLinkIndexAtOffset(aOffset);
  return NS_OK;
}




NS_IMETHODIMP
HyperTextAccessible::SetAttributes(PRInt32 aStartPos, PRInt32 aEndPos,
                                   nsISupports* aAttributes)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
HyperTextAccessible::SetTextContents(const nsAString& aText)
{
  PRInt32 numChars = CharacterCount();
  if (numChars == 0 || NS_SUCCEEDED(DeleteText(0, numChars))) {
    return InsertText(aText, 0);
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
HyperTextAccessible::InsertText(const nsAString& aText, PRInt32 aPosition)
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
HyperTextAccessible::CopyText(PRInt32 aStartPos, PRInt32 aEndPos)
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
HyperTextAccessible::CutText(PRInt32 aStartPos, PRInt32 aEndPos)
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
HyperTextAccessible::DeleteText(PRInt32 aStartPos, PRInt32 aEndPos)
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
HyperTextAccessible::PasteText(PRInt32 aPosition)
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

    return nsnull;
  }

  nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem =
    nsCoreUtils::GetDocShellTreeItemFor(mContent);
  nsCOMPtr<nsIEditingSession> editingSession(do_GetInterface(docShellTreeItem));
  if (!editingSession)
    return nsnull; 

  nsCOMPtr<nsIEditor> editor;
  nsIDocument* docNode = mDoc->GetDocumentNode();
  editingSession->GetEditorForWindow(docNode->GetWindow(),
                                     getter_AddRefs(editor));
  return editor.forget();
}





nsresult
HyperTextAccessible::SetSelectionRange(PRInt32 aStartPos, PRInt32 aEndPos)
{
  bool isFocusable = InteractiveState() & states::FOCUSABLE;

  
  
  
  
  if (isFocusable)
    TakeFocus();

  
  SetSelectionBounds(0, aStartPos, aEndPos);

  
  
  nsRefPtr<nsFrameSelection> frameSelection = FrameSelection();
  NS_ENSURE_STATE(frameSelection);

  Selection* domSel =
    frameSelection->GetSelection(nsISelectionController::SELECTION_NORMAL);
  NS_ENSURE_STATE(domSel);

  for (PRInt32 idx = domSel->GetRangeCount() - 1; idx > 0; idx--)
    domSel->RemoveRange(domSel->GetRangeAt(idx));

  
  
  
  if (isFocusable)
    return NS_OK;

  nsFocusManager* DOMFocusManager = nsFocusManager::GetFocusManager();
  if (DOMFocusManager) {
    NS_ENSURE_TRUE(mDoc, NS_ERROR_FAILURE);
    nsIDocument* docNode = mDoc->GetDocumentNode();
    NS_ENSURE_TRUE(docNode, NS_ERROR_FAILURE);
    nsCOMPtr<nsPIDOMWindow> window = docNode->GetWindow();
    nsCOMPtr<nsIDOMElement> result;
    DOMFocusManager->MoveFocus(window, nsnull, nsIFocusManager::MOVEFOCUS_CARET,
                               nsIFocusManager::FLAG_BYMOVEFOCUS, getter_AddRefs(result));
  }

  return NS_OK;
}

NS_IMETHODIMP
HyperTextAccessible::SetCaretOffset(PRInt32 aCaretOffset)
{
  return SetSelectionRange(aCaretOffset, aCaretOffset);
}




NS_IMETHODIMP
HyperTextAccessible::GetCaretOffset(PRInt32* aCaretOffset)
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
  PRInt32 focusOffset = domSel->GetFocusOffset();

  
  
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

PRInt32
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

  PRInt32 returnOffsetUnused;
  PRInt32 caretOffset = domSel->GetFocusOffset();
  nsFrameSelection::HINT hint = frameSelection->GetHint();
  nsIFrame *caretFrame = frameSelection->GetFrameForNodeOffset(caretContent, caretOffset,
                                                               hint, &returnOffsetUnused);
  NS_ENSURE_TRUE(caretFrame, -1);

  PRInt32 lineNumber = 1;
  nsAutoLineIterator lineIterForCaret;
  nsIContent *hyperTextContent = IsContent() ? mContent.get() : nsnull;
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
        
        PRInt32 addLines = lineIterForSibling->GetNumLines();
        lineNumber += addLines;
      }
      sibling = sibling->GetNextSibling();
    }

    
    if (!lineIterForCaret) {   
      lineIterForCaret = parentFrame->GetLineIterator();
      if (lineIterForCaret) {
        
        PRInt32 addLines = lineIterForCaret->FindLineContaining(caretFrame);
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
  return frame ? frame->GetFrameSelection() : nsnull;
}

void
HyperTextAccessible::GetSelectionDOMRanges(PRInt16 aType,
                                           nsTArray<nsRange*>* aRanges)
{
  nsRefPtr<nsFrameSelection> frameSelection = FrameSelection();
  if (!frameSelection)
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

  PRUint32 childCount = startNode->GetChildCount();
  nsresult rv = domSel->
    GetRangesForIntervalArray(startNode, 0, startNode, childCount, true, aRanges);
  NS_ENSURE_SUCCESS(rv,);

  
  PRUint32 numRanges = aRanges->Length();
  for (PRUint32 idx = 0; idx < numRanges; idx ++) {
    if ((*aRanges)[idx]->Collapsed()) {
      aRanges->RemoveElementAt(idx);
      --numRanges;
      --idx;
    }
  }
}




NS_IMETHODIMP
HyperTextAccessible::GetSelectionCount(PRInt32* aSelectionCount)
{
  NS_ENSURE_ARG_POINTER(aSelectionCount);
  *aSelectionCount = 0;

  nsTArray<nsRange*> ranges;
  GetSelectionDOMRanges(nsISelectionController::SELECTION_NORMAL, &ranges);
  *aSelectionCount = PRInt32(ranges.Length());

  return NS_OK;
}




NS_IMETHODIMP
HyperTextAccessible::GetSelectionBounds(PRInt32 aSelectionNum,
                                        PRInt32* aStartOffset,
                                        PRInt32* aEndOffset)
{
  NS_ENSURE_ARG_POINTER(aStartOffset);
  NS_ENSURE_ARG_POINTER(aEndOffset);
  *aStartOffset = *aEndOffset = 0;

  nsTArray<nsRange*> ranges;
  GetSelectionDOMRanges(nsISelectionController::SELECTION_NORMAL, &ranges);

  PRUint32 rangeCount = ranges.Length();
  if (aSelectionNum < 0 || aSelectionNum >= rangeCount)
    return NS_ERROR_INVALID_ARG;

  nsRange* range = ranges[aSelectionNum];

  
  nsINode* startNode = range->GetStartParent();
  nsINode* endNode = range->GetEndParent();
  PRInt32 startOffset = range->StartOffset(), endOffset = range->EndOffset();

  
  
  PRInt32 rangeCompare = nsContentUtils::ComparePoints(endNode, endOffset,
                                                       startNode, startOffset);
  if (rangeCompare < 0) {
    nsINode* tempNode = startNode;
    startNode = endNode;
    endNode = tempNode;
    PRInt32 tempOffset = startOffset;
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
HyperTextAccessible::SetSelectionBounds(PRInt32 aSelectionNum,
                                        PRInt32 aStartOffset,
                                        PRInt32 aEndOffset)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  if (aSelectionNum < 0)
    return NS_ERROR_INVALID_ARG;

  nsRefPtr<nsFrameSelection> frameSelection = FrameSelection();
  NS_ENSURE_STATE(frameSelection);

  Selection* domSel =
    frameSelection->GetSelection(nsISelectionController::SELECTION_NORMAL);
  NS_ENSURE_STATE(domSel);

  PRUint32 rangeCount = domSel->GetRangeCount();
  if (rangeCount < static_cast<PRUint32>(aSelectionNum))
    return NS_ERROR_INVALID_ARG;

  nsRefPtr<nsRange> range;
  if (aSelectionNum == rangeCount)
    range = new nsRange();
  else
    range = domSel->GetRangeAt(aSelectionNum);

  nsresult rv = HypertextOffsetsToDOMRange(aStartOffset, aEndOffset, range);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (aSelectionNum == rangeCount)
    return domSel->AddRange(range);

  domSel->RemoveRange(range);
  domSel->AddRange(range);
  return NS_OK;
}




NS_IMETHODIMP
HyperTextAccessible::AddSelection(PRInt32 aStartOffset, PRInt32 aEndOffset)
{
  nsRefPtr<nsFrameSelection> frameSelection = FrameSelection();
  NS_ENSURE_STATE(frameSelection);

  Selection* domSel =
    frameSelection->GetSelection(nsISelectionController::SELECTION_NORMAL);
  NS_ENSURE_STATE(domSel);

  return SetSelectionBounds(domSel->GetRangeCount(), aStartOffset, aEndOffset);
}




NS_IMETHODIMP
HyperTextAccessible::RemoveSelection(PRInt32 aSelectionNum)
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
HyperTextAccessible::ScrollSubstringTo(PRInt32 aStartIndex, PRInt32 aEndIndex,
                                       PRUint32 aScrollType)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsRefPtr<nsRange> range = new nsRange();
  nsresult rv = HypertextOffsetsToDOMRange(aStartIndex, aEndIndex, range);
  NS_ENSURE_SUCCESS(rv, rv);

  return nsCoreUtils::ScrollSubstringTo(GetFrame(), range, aScrollType);
}





NS_IMETHODIMP
HyperTextAccessible::ScrollSubstringToPoint(PRInt32 aStartIndex,
                                            PRInt32 aEndIndex,
                                            PRUint32 aCoordinateType,
                                            PRInt32 aX, PRInt32 aY)
{
  nsIFrame *frame = GetFrame();
  if (!frame)
    return NS_ERROR_FAILURE;

  nsIntPoint coords;
  nsresult rv = nsAccUtils::ConvertToScreenCoords(aX, aY, aCoordinateType,
                                                  this, &coords);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<nsRange> range = new nsRange();
  rv = HypertextOffsetsToDOMRange(aStartIndex, aEndIndex, range);
  NS_ENSURE_SUCCESS(rv, rv);

  nsPresContext *presContext = frame->PresContext();

  bool initialScrolled = false;
  nsIFrame *parentFrame = frame;
  while ((parentFrame = parentFrame->GetParent())) {
    nsIScrollableFrame *scrollableFrame = do_QueryFrame(parentFrame);
    if (scrollableFrame) {
      if (!initialScrolled) {
        
        
        nsIntRect frameRect = parentFrame->GetScreenRectExternal();
        PRInt32 devOffsetX = coords.x - frameRect.x;
        PRInt32 devOffsetY = coords.y - frameRect.y;

        nsPoint offsetPoint(presContext->DevPixelsToAppUnits(devOffsetX),
                            presContext->DevPixelsToAppUnits(devOffsetY));

        nsSize size(parentFrame->GetSize());

        
        size.width = size.width ? size.width : 1;
        size.height = size.height ? size.height : 1;

        PRInt16 hPercent = offsetPoint.x * 100 / size.width;
        PRInt16 vPercent = offsetPoint.y * 100 / size.height;

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




nsresult
HyperTextAccessible::GetNameInternal(nsAString& aName)
{
  nsresult rv = AccessibleWrap::GetNameInternal(aName);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  if (aName.IsEmpty() && IsAbbreviation()) {
    nsAutoString name;
    if (mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::title, name)) {
      name.CompressWhitespace();
      aName = name;
    }
  }
  return NS_OK;
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
  PRInt32 childIndex = aAccessible->IndexInParent();
  PRInt32 count = mOffsets.Length() - childIndex;
  if (count > 0)
    mOffsets.RemoveElementsAt(childIndex, count);

  return Accessible::RemoveChild(aAccessible);
}




nsresult
HyperTextAccessible::ContentToRenderedOffset(nsIFrame* aFrame, PRInt32 aContentOffset,
                                             PRUint32* aRenderedOffset)
{
  if (!aFrame) {
    
    
    *aRenderedOffset = 0;
    return NS_OK;
  }
  NS_ASSERTION(aFrame->GetType() == nsGkAtoms::textFrame,
               "Need text frame for offset conversion");
  NS_ASSERTION(aFrame->GetPrevContinuation() == nsnull,
               "Call on primary frame only");

  gfxSkipChars skipChars;
  gfxSkipCharsIterator iter;
  
  nsresult rv = aFrame->GetRenderedText(nsnull, &skipChars, &iter, 0, aContentOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 ourRenderedStart = iter.GetSkippedOffset();
  PRInt32 ourContentStart = iter.GetOriginalOffset();

  *aRenderedOffset = iter.ConvertOriginalToSkipped(aContentOffset + ourContentStart) -
                    ourRenderedStart;

  return NS_OK;
}

nsresult
HyperTextAccessible::RenderedToContentOffset(nsIFrame* aFrame, PRUint32 aRenderedOffset,
                                             PRInt32* aContentOffset)
{
  *aContentOffset = 0;
  NS_ENSURE_TRUE(aFrame, NS_ERROR_FAILURE);

  NS_ASSERTION(aFrame->GetType() == nsGkAtoms::textFrame,
               "Need text frame for offset conversion");
  NS_ASSERTION(aFrame->GetPrevContinuation() == nsnull,
               "Call on primary frame only");

  gfxSkipChars skipChars;
  gfxSkipCharsIterator iter;
  
  nsresult rv = aFrame->GetRenderedText(nsnull, &skipChars, &iter, 0, aRenderedOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 ourRenderedStart = iter.GetSkippedOffset();
  PRInt32 ourContentStart = iter.GetOriginalOffset();

  *aContentOffset = iter.ConvertSkippedToOriginal(aRenderedOffset + ourRenderedStart) - ourContentStart;

  return NS_OK;
}




bool
HyperTextAccessible::GetCharAt(PRInt32 aOffset, EGetTextType aShift,
                               nsAString& aChar, PRInt32* aStartOffset,
                               PRInt32* aEndOffset)
{
  aChar.Truncate();

  PRInt32 offset = ConvertMagicOffset(aOffset) + static_cast<PRInt32>(aShift);
  PRInt32 childIdx = GetChildIndexAtOffset(offset);
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

PRInt32
HyperTextAccessible::GetChildOffset(PRUint32 aChildIndex,
                                    bool aInvalidateAfter)
{
  if (aChildIndex == 0) {
    if (aInvalidateAfter)
      mOffsets.Clear();

    return aChildIndex;
  }

  PRInt32 count = mOffsets.Length() - aChildIndex;
  if (count > 0) {
    if (aInvalidateAfter)
      mOffsets.RemoveElementsAt(aChildIndex, count);

    return mOffsets[aChildIndex - 1];
  }

  PRUint32 lastOffset = mOffsets.IsEmpty() ?
    0 : mOffsets[mOffsets.Length() - 1];

  while (mOffsets.Length() < aChildIndex) {
    Accessible* child = mChildren[mOffsets.Length()];
    lastOffset += nsAccUtils::TextLength(child);
    mOffsets.AppendElement(lastOffset);
  }

  return mOffsets[aChildIndex - 1];
}

PRInt32
HyperTextAccessible::GetChildIndexAtOffset(PRUint32 aOffset)
{
  PRUint32 lastOffset = 0;
  PRUint32 offsetCount = mOffsets.Length();
  if (offsetCount > 0) {
    lastOffset = mOffsets[offsetCount - 1];
    if (aOffset < lastOffset) {
      PRUint32 low = 0, high = offsetCount;
      while (high > low) {
        PRUint32 mid = (high + low) >> 1;
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

  PRUint32 childCount = ChildCount();
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
HyperTextAccessible::GetDOMPointByFrameOffset(nsIFrame* aFrame, PRInt32 aOffset,
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
                                                 PRInt32* aHTOffset)
{
  nsINode* node = nsnull;
  PRInt32 nodeOffset = 0;

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
                                           PRInt32 aNodeOffset,
                                           PRInt32* aHTStartOffset,
                                           PRInt32* aHTEndOffset,
                                           nsIPersistentProperties* aAttributes)
{
  nsTArray<nsRange*> ranges;
  GetSelectionDOMRanges(nsISelectionController::SELECTION_SPELLCHECK, &ranges);

  PRUint32 rangeCount = ranges.Length();
  if (!rangeCount)
    return NS_OK;

  nsCOMPtr<nsIDOMNode> DOMNode = do_QueryInterface(aNode);
  for (PRUint32 index = 0; index < rangeCount; index++) {
    nsRange* range = ranges[index];

    PRInt16 result;
    nsresult rv = range->ComparePoint(DOMNode, aNodeOffset, &result);
    NS_ENSURE_SUCCESS(rv, rv);
    
    
    
    if (result == 0) {
      if (aNode == range->GetEndParent() && aNodeOffset == range->EndOffset())
        result = 1;
    }

    if (result == 1) { 
      PRInt32 startHTOffset = 0;
      nsresult rv = RangeBoundToHypertextOffset(range, false, true,
                                                &startHTOffset);
      NS_ENSURE_SUCCESS(rv, rv);

      if (startHTOffset > *aHTStartOffset)
        *aHTStartOffset = startHTOffset;

    } else if (result == -1) { 
      PRInt32 endHTOffset = 0;
      nsresult rv = RangeBoundToHypertextOffset(range, true, false,
                                                &endHTOffset);
      NS_ENSURE_SUCCESS(rv, rv);

      if (endHTOffset < *aHTEndOffset)
        *aHTEndOffset = endHTOffset;

    } else { 
      PRInt32 startHTOffset = 0;
      nsresult rv = RangeBoundToHypertextOffset(range, true, true,
                                                &startHTOffset);
      NS_ENSURE_SUCCESS(rv, rv);

      PRInt32 endHTOffset = 0;
      rv = RangeBoundToHypertextOffset(range, false, false,
                                       &endHTOffset);
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
  }

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
