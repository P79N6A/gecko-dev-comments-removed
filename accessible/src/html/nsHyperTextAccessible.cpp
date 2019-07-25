






































#include "nsHyperTextAccessible.h"

#include "nsAccessibilityAtoms.h"
#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "nsTextAttrs.h"

#include "nsIClipboard.h"
#include "nsContentCID.h"
#include "nsIDOMAbstractView.h"
#include "nsIDOMCharacterData.h"
#include "nsIDOMDocument.h"
#include "nsPIDOMWindow.h"        
#include "nsIDOMDocumentView.h"
#include "nsIDOMRange.h"
#include "nsIDOMNSRange.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDOMXULDocument.h"
#include "nsIEditingSession.h"
#include "nsIEditor.h"
#include "nsIFontMetrics.h"
#include "nsIFrame.h"
#include "nsFrameSelection.h"
#include "nsILineIterator.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIPlaintextEditor.h"
#include "nsIScrollableFrame.h"
#include "nsISelection2.h"
#include "nsISelectionPrivate.h"
#include "nsIServiceManager.h"
#include "nsTextFragment.h"
#include "gfxSkipChars.h"

static NS_DEFINE_IID(kRangeCID, NS_RANGE_CID);





nsHyperTextAccessible::
  nsHyperTextAccessible(nsIContent *aNode, nsIWeakReference *aShell) :
  nsAccessibleWrap(aNode, aShell)
{
}

NS_IMPL_ADDREF_INHERITED(nsHyperTextAccessible, nsAccessibleWrap)
NS_IMPL_RELEASE_INHERITED(nsHyperTextAccessible, nsAccessibleWrap)

nsresult nsHyperTextAccessible::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  *aInstancePtr = nsnull;

  if (aIID.Equals(NS_GET_IID(nsHyperTextAccessible))) {
    *aInstancePtr = static_cast<nsHyperTextAccessible*>(this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  if (mRoleMapEntry &&
      (mRoleMapEntry->role == nsIAccessibleRole::ROLE_GRAPHIC ||
       mRoleMapEntry->role == nsIAccessibleRole::ROLE_IMAGE_MAP ||
       mRoleMapEntry->role == nsIAccessibleRole::ROLE_SLIDER ||
       mRoleMapEntry->role == nsIAccessibleRole::ROLE_PROGRESSBAR ||
       mRoleMapEntry->role == nsIAccessibleRole::ROLE_SEPARATOR)) {
    
    return nsAccessible::QueryInterface(aIID, aInstancePtr);
  }

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

  return nsAccessible::QueryInterface(aIID, aInstancePtr);
}

PRUint32
nsHyperTextAccessible::NativeRole()
{
  nsIAtom *tag = mContent->Tag();

  if (tag == nsAccessibilityAtoms::form)
    return nsIAccessibleRole::ROLE_FORM;

  if (tag == nsAccessibilityAtoms::div ||
      tag == nsAccessibilityAtoms::blockquote)
    return nsIAccessibleRole::ROLE_SECTION;

  if (tag == nsAccessibilityAtoms::h1 ||
      tag == nsAccessibilityAtoms::h2 ||
      tag == nsAccessibilityAtoms::h3 ||
      tag == nsAccessibilityAtoms::h4 ||
      tag == nsAccessibilityAtoms::h5 ||
      tag == nsAccessibilityAtoms::h6)
    return nsIAccessibleRole::ROLE_HEADING;

  nsIFrame *frame = GetFrame();
  if (frame && frame->GetType() == nsAccessibilityAtoms::blockFrame &&
      frame->GetContent()->Tag() != nsAccessibilityAtoms::input) {
    
    
    return nsIAccessibleRole::ROLE_PARAGRAPH;
  }

  return nsIAccessibleRole::ROLE_TEXT_CONTAINER; 
}

nsresult
nsHyperTextAccessible::GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsAccessibleWrap::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  if (!aExtraState)
    return NS_OK;

  nsCOMPtr<nsIEditor> editor;
  GetAssociatedEditor(getter_AddRefs(editor));
  if (editor) {
    PRUint32 flags;
    editor->GetFlags(&flags);
    if (0 == (flags & nsIPlaintextEditor::eEditorReadonlyMask)) {
      *aExtraState |= nsIAccessibleStates::EXT_STATE_EDITABLE;
    }
  }

  PRInt32 childCount;
  GetChildCount(&childCount);
  if (childCount > 0) {
    *aExtraState |= nsIAccessibleStates::EXT_STATE_SELECTABLE_TEXT;
  }

  return NS_OK;
}


nsIntRect nsHyperTextAccessible::GetBoundsForString(nsIFrame *aFrame, PRUint32 aStartRenderedOffset,
                                                    PRUint32 aEndRenderedOffset)
{
  nsIntRect screenRect;
  NS_ENSURE_TRUE(aFrame, screenRect);
  if (aFrame->GetType() != nsAccessibilityAtoms::textFrame) {
    
    
    return aFrame->GetScreenRectExternal();
  }

  PRInt32 startContentOffset, endContentOffset;
  nsresult rv = RenderedToContentOffset(aFrame, aStartRenderedOffset, &startContentOffset);
  NS_ENSURE_SUCCESS(rv, screenRect);
  rv = RenderedToContentOffset(aFrame, aEndRenderedOffset, &endContentOffset);
  NS_ENSURE_SUCCESS(rv, screenRect);

  nsIFrame *frame;
  PRInt32 startContentOffsetInFrame;
  
  
  rv = aFrame->GetChildFrameContainingOffset(startContentOffset, PR_FALSE,
                                             &startContentOffsetInFrame, &frame);
  NS_ENSURE_SUCCESS(rv, screenRect);

  nsCOMPtr<nsIPresShell> shell = GetPresShell();
  NS_ENSURE_TRUE(shell, screenRect);

  nsPresContext *context = shell->GetPresContext();

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
nsHyperTextAccessible::GetPosAndText(PRInt32& aStartOffset, PRInt32& aEndOffset,
                                     nsAString *aText, nsIFrame **aEndFrame,
                                     nsIntRect *aBoundsRect,
                                     nsAccessible **aStartAcc,
                                     nsAccessible **aEndAcc)
{
  if (aStartOffset == nsIAccessibleText::TEXT_OFFSET_END_OF_TEXT) {
    GetCharacterCount(&aStartOffset);
  }
  if (aStartOffset == nsIAccessibleText::TEXT_OFFSET_CARET) {
    GetCaretOffset(&aStartOffset);
  }
  if (aEndOffset == nsIAccessibleText::TEXT_OFFSET_END_OF_TEXT) {
    GetCharacterCount(&aEndOffset);
  }
  if (aEndOffset == nsIAccessibleText::TEXT_OFFSET_CARET) {
    GetCaretOffset(&aEndOffset);
  }

  PRInt32 startOffset = aStartOffset;
  PRInt32 endOffset = aEndOffset;
  
  PRBool isPassword =
    (nsAccUtils::Role(this) == nsIAccessibleRole::ROLE_PASSWORD_TEXT);

  
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
    aBoundsRect->Empty();
  }
  if (aStartAcc)
    *aStartAcc = nsnull;
  if (aEndAcc)
    *aEndAcc = nsnull;

  nsIntRect unionRect;
  nsAccessible *lastAccessible = nsnull;

  gfxSkipChars skipChars;
  gfxSkipCharsIterator iter;

  
  
  PRInt32 childCount = GetChildCount();
  for (PRInt32 childIdx = 0; childIdx < childCount; childIdx++) {
    nsAccessible *childAcc = mChildren[childIdx];
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
      if (frame->GetType() == nsAccessibilityAtoms::textFrame) {
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
          if (frame->GetType() == nsAccessibilityAtoms::textFrame) {
            contentOffset = iter.ConvertSkippedToOriginal(startOffset) +
                            ourRenderedStart - ourContentStart;
          }
          else {
            contentOffset = startOffset;
          }
          frame->GetChildFrameContainingOffset(contentOffset, PR_TRUE,
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
            
            if (frame->GetType() == nsAccessibilityAtoms::brFrame) {
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

NS_IMETHODIMP nsHyperTextAccessible::GetText(PRInt32 aStartOffset, PRInt32 aEndOffset, nsAString &aText)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  return GetPosAndText(aStartOffset, aEndOffset, &aText) ? NS_OK : NS_ERROR_FAILURE;
}




NS_IMETHODIMP nsHyperTextAccessible::GetCharacterCount(PRInt32 *aCharacterCount)
{
  NS_ENSURE_ARG_POINTER(aCharacterCount);
  *aCharacterCount = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  PRInt32 childCount = GetChildCount();
  for (PRInt32 childIdx = 0; childIdx < childCount; childIdx++) {
    nsAccessible *childAcc = mChildren[childIdx];

    PRInt32 textLength = nsAccUtils::TextLength(childAcc);
    NS_ENSURE_TRUE(textLength >= 0, nsnull);
    *aCharacterCount += textLength;
  }
  return NS_OK;
}




NS_IMETHODIMP nsHyperTextAccessible::GetCharacterAtOffset(PRInt32 aOffset, PRUnichar *aCharacter)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsAutoString text;
  nsresult rv = GetText(aOffset, aOffset + 1, text);
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (text.IsEmpty()) {
    return NS_ERROR_FAILURE;
  }
  *aCharacter = text.First();
  return NS_OK;
}

nsAccessible*
nsHyperTextAccessible::DOMPointToHypertextOffset(nsINode *aNode,
                                                 PRInt32 aNodeOffset,
                                                 PRInt32 *aHyperTextOffset,
                                                 PRBool aIsEndOffset)
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
    
    
    
    nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
    NS_ASSERTION(content, "No nsIContent for dom node");
    nsIFrame *frame = content->GetPrimaryFrame();
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

  
  
  nsAccessible *descendantAcc = nsnull;
  if (findNode) {
    nsCOMPtr<nsIContent> findContent(do_QueryInterface(findNode));
    if (findContent && findContent->IsHTML() &&
        findContent->NodeInfo()->Equals(nsAccessibilityAtoms::br) &&
        findContent->AttrValueIs(kNameSpaceID_None,
                                 nsAccessibilityAtoms::mozeditorbogusnode,
                                 nsAccessibilityAtoms::_true,
                                 eIgnoreCase)) {
      
      *aHyperTextOffset = 0;
      return nsnull;
    }
    descendantAcc = GetFirstAvailableAccessible(findNode);
  }

  
  nsAccessible *childAccAtOffset = nsnull;
  while (descendantAcc) {
    nsAccessible *parentAcc = descendantAcc->GetParent();
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

  
  
  
  
  PRInt32 childCount = GetChildCount();

  PRInt32 childIdx = 0;
  nsAccessible *childAcc = nsnull;
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
nsHyperTextAccessible::HypertextOffsetToDOMPoint(PRInt32 aHTOffset,
                                                 nsIDOMNode **aNode,
                                                 PRInt32 *aOffset)
{
  nsCOMPtr<nsIDOMNode> endNode;
  PRInt32 endOffset;

  return HypertextOffsetsToDOMRange(aHTOffset, aHTOffset, aNode, aOffset,
                                    getter_AddRefs(endNode), &endOffset);
}

nsresult
nsHyperTextAccessible::HypertextOffsetsToDOMRange(PRInt32 aStartHTOffset,
                                                  PRInt32 aEndHTOffset,
                                                  nsIDOMNode **aStartNode,
                                                  PRInt32 *aStartOffset,
                                                  nsIDOMNode **aEndNode,
                                                  PRInt32 *aEndOffset)
{
  NS_ENSURE_ARG_POINTER(aStartNode);
  *aStartNode = nsnull;

  NS_ENSURE_ARG_POINTER(aStartOffset);
  *aStartOffset = -1;

  NS_ENSURE_ARG_POINTER(aEndNode);
  *aEndNode = nsnull;

  NS_ENSURE_ARG_POINTER(aEndOffset);
  *aEndOffset = -1;

  
  
  if (aStartHTOffset == 0 && aEndHTOffset == 0) {
    nsCOMPtr<nsIEditor> editor;
    GetAssociatedEditor(getter_AddRefs(editor));
    if (editor) {
      PRBool isEmpty = PR_FALSE;
      editor->GetDocumentIsEmpty(&isEmpty);
      if (isEmpty) {
        nsCOMPtr<nsIDOMElement> editorRootElm;
        editor->GetRootElement(getter_AddRefs(editorRootElm));

        nsCOMPtr<nsIDOMNode> editorRoot(do_QueryInterface(editorRootElm));
        if (editorRoot) {
          *aStartOffset = *aEndOffset = 0;
          NS_ADDREF(*aStartNode = editorRoot);
          NS_ADDREF(*aEndNode = editorRoot);

          return NS_OK;
        }
      }
    }
  }

  nsRefPtr<nsAccessible> startAcc, endAcc;
  PRInt32 startOffset = aStartHTOffset, endOffset = aEndHTOffset;
  nsIFrame *startFrame = nsnull, *endFrame = nsnull;

  startFrame = GetPosAndText(startOffset, endOffset, nsnull, &endFrame, nsnull,
                             getter_AddRefs(startAcc), getter_AddRefs(endAcc));
  if (!startAcc || !endAcc)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNode> startNode, endNode;
  nsresult rv = GetDOMPointByFrameOffset(startFrame, startOffset, startAcc,
                                         getter_AddRefs(startNode),
                                         &startOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aStartHTOffset != aEndHTOffset) {
    rv = GetDOMPointByFrameOffset(endFrame, endOffset, endAcc,
                                  getter_AddRefs(endNode), &endOffset);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    endNode = startNode;
    endOffset = startOffset;
  }

  NS_ADDREF(*aStartNode = startNode);
  *aStartOffset = startOffset;

  NS_ADDREF(*aEndNode = endNode);
  *aEndOffset = endOffset;

  return NS_OK;
}

PRInt32
nsHyperTextAccessible::GetRelativeOffset(nsIPresShell *aPresShell,
                                         nsIFrame *aFromFrame,
                                         PRInt32 aFromOffset,
                                         nsAccessible *aFromAccessible,
                                         nsSelectionAmount aAmount,
                                         nsDirection aDirection,
                                         PRBool aNeedsStart)
{
  const PRBool kIsJumpLinesOk = PR_TRUE;          
  const PRBool kIsScrollViewAStop = PR_FALSE;     
  const PRBool kIsKeyboardSelect = PR_TRUE;       
  const PRBool kIsVisualBidi = PR_FALSE;          

  EWordMovementType wordMovementType = aNeedsStart ? eStartWord : eEndWord;
  if (aAmount == eSelectLine) {
    aAmount = (aDirection == eDirNext) ? eSelectEndLine : eSelectBeginLine;
  }

  
  nsPeekOffsetStruct pos;

  nsresult rv;
  PRInt32 contentOffset = aFromOffset;
  if (nsAccUtils::IsText(aFromAccessible)) {
    nsIFrame *frame = aFromAccessible->GetFrame();
    NS_ENSURE_TRUE(frame, -1);

    if (frame->GetType() == nsAccessibilityAtoms::textFrame) {
      rv = RenderedToContentOffset(frame, aFromOffset, &contentOffset);
      NS_ENSURE_SUCCESS(rv, -1);
    }
  }

  pos.SetData(aAmount, aDirection, contentOffset,
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

  
  
  nsAccessible *finalAccessible =
    DOMPointToHypertextOffset(pos.mResultContent, pos.mContentOffset,
                              &hyperTextOffset, aDirection == eDirNext);

  if (!finalAccessible && aDirection == eDirPrevious) {
    
    
    hyperTextOffset = 0;
  }  
  else if (aAmount == eSelectBeginLine) {
    nsAccessible *firstChild = mChildren.SafeElementAt(0, nsnull);
    
    if (pos.mContentOffset == 0 && firstChild &&
        nsAccUtils::Role(firstChild) == nsIAccessibleRole::ROLE_STATICTEXT &&
        static_cast<PRInt32>(nsAccUtils::TextLength(firstChild)) == hyperTextOffset) {
      
      hyperTextOffset = 0;
    }
    if (!aNeedsStart && hyperTextOffset > 0) {
      -- hyperTextOffset;
    }
  }
  else if (aAmount == eSelectEndLine && finalAccessible) { 
    
    
    if (nsAccUtils::Role(finalAccessible) == nsIAccessibleRole::ROLE_WHITESPACE) {  
      
      
      
      
      ++ hyperTextOffset;  
    }
    
    if (!aNeedsStart) {
      -- hyperTextOffset;
    }
  }

  return hyperTextOffset;
}










nsresult nsHyperTextAccessible::GetTextHelper(EGetTextType aType, nsAccessibleTextBoundary aBoundaryType,
                                              PRInt32 aOffset, PRInt32 *aStartOffset, PRInt32 *aEndOffset,
                                              nsAString &aText)
{
  aText.Truncate();

  NS_ENSURE_ARG_POINTER(aStartOffset);
  NS_ENSURE_ARG_POINTER(aEndOffset);
  *aStartOffset = *aEndOffset = 0;

  nsCOMPtr<nsIPresShell> presShell = GetPresShell();
  if (!presShell) {
    return NS_ERROR_FAILURE;
  }

  if (aOffset == nsIAccessibleText::TEXT_OFFSET_END_OF_TEXT) {
    GetCharacterCount(&aOffset);
  }
  if (aOffset == nsIAccessibleText::TEXT_OFFSET_CARET) {
    GetCaretOffset(&aOffset);
    if (aOffset > 0 && (aBoundaryType == BOUNDARY_LINE_START ||
                        aBoundaryType == BOUNDARY_LINE_END)) {
      
      
      
      
      nsCOMPtr<nsISelection> domSel;
      nsresult rv = GetSelections(nsISelectionController::SELECTION_NORMAL,
                                  nsnull, getter_AddRefs(domSel));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsISelectionPrivate> privateSelection(do_QueryInterface(domSel));
      nsCOMPtr<nsFrameSelection> frameSelection;
      rv = privateSelection->GetFrameSelection(getter_AddRefs(frameSelection));
      NS_ENSURE_SUCCESS(rv, rv);

      if (frameSelection->GetHint() == nsFrameSelection::HINTLEFT) {
        -- aOffset;  
      }
    }
  }
  else if (aOffset < 0) {
    return NS_ERROR_FAILURE;
  }

  nsSelectionAmount amount;
  PRBool needsStart = PR_FALSE;
  switch (aBoundaryType) {
    case BOUNDARY_CHAR:
      amount = eSelectCharacter;
      if (aType == eGetAt)
        aType = eGetAfter; 
      break;

    case BOUNDARY_WORD_START:
      needsStart = PR_TRUE;
      amount = eSelectWord;
      break;

    case BOUNDARY_WORD_END:
      amount = eSelectWord;
      break;

    case BOUNDARY_LINE_START:
      
      
      
      needsStart = PR_TRUE;
      amount = eSelectLine;
      break;

    case BOUNDARY_LINE_END:
      
      
      
      amount = eSelectLine;
      break;

    case BOUNDARY_ATTRIBUTE_RANGE:
    {
      nsresult rv = GetTextAttributes(PR_FALSE, aOffset,
                                      aStartOffset, aEndOffset, nsnull);
      NS_ENSURE_SUCCESS(rv, rv);
      
      return GetText(*aStartOffset, *aEndOffset, aText);
    }

    default:  
      return NS_ERROR_INVALID_ARG;
  }

  PRInt32 startOffset = aOffset + (aBoundaryType == BOUNDARY_LINE_END);  
  PRInt32 endOffset = startOffset;

  
  nsRefPtr<nsAccessible> startAcc;
  nsIFrame *startFrame = GetPosAndText(startOffset, endOffset, nsnull, nsnull,
                                       nsnull, getter_AddRefs(startAcc));

  if (!startFrame) {
    PRInt32 textLength;
    GetCharacterCount(&textLength);
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
    endOffset = aOffset;
  }
  else {
    
    
    
    
    startOffset = endOffset = finalStartOffset + (aBoundaryType == BOUNDARY_LINE_END);
    nsRefPtr<nsAccessible> endAcc;
    nsIFrame *endFrame = GetPosAndText(startOffset, endOffset, nsnull, nsnull,
                                       nsnull, getter_AddRefs(endAcc));
    if (nsAccUtils::Role(endAcc) == nsIAccessibleRole::ROLE_STATICTEXT) {
      
      
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
      PRInt32 textLength;
      GetCharacterCount(&textLength);
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




NS_IMETHODIMP nsHyperTextAccessible::GetTextBeforeOffset(PRInt32 aOffset, nsAccessibleTextBoundary aBoundaryType,
                                                         PRInt32 *aStartOffset, PRInt32 *aEndOffset, nsAString & aText)
{
  return GetTextHelper(eGetBefore, aBoundaryType, aOffset, aStartOffset, aEndOffset, aText);
}

NS_IMETHODIMP nsHyperTextAccessible::GetTextAtOffset(PRInt32 aOffset, nsAccessibleTextBoundary aBoundaryType,
                                                     PRInt32 *aStartOffset, PRInt32 *aEndOffset, nsAString & aText)
{
  return GetTextHelper(eGetAt, aBoundaryType, aOffset, aStartOffset, aEndOffset, aText);
}

NS_IMETHODIMP nsHyperTextAccessible::GetTextAfterOffset(PRInt32 aOffset, nsAccessibleTextBoundary aBoundaryType,
                                                        PRInt32 *aStartOffset, PRInt32 *aEndOffset, nsAString & aText)
{
  return GetTextHelper(eGetAfter, aBoundaryType, aOffset, aStartOffset, aEndOffset, aText);
}






NS_IMETHODIMP
nsHyperTextAccessible::GetTextAttributes(PRBool aIncludeDefAttrs,
                                         PRInt32 aOffset,
                                         PRInt32 *aStartOffset,
                                         PRInt32 *aEndOffset,
                                         nsIPersistentProperties **aAttributes)
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

  nsAccessible* accAtOffset = GetChildAtOffset(aOffset);
  if (!accAtOffset) {
    
    
    if (aOffset == 0) {
      if (aIncludeDefAttrs) {
        nsTextAttrsMgr textAttrsMgr(this, PR_TRUE, nsnull, -1);
        return textAttrsMgr.GetAttributes(*aAttributes);
      }
      return NS_OK;
    }
    return NS_ERROR_INVALID_ARG;
  }

  PRInt32 accAtOffsetIdx = accAtOffset->GetIndexInParent();
  PRInt32 startOffset = GetChildOffset(accAtOffsetIdx);
  PRInt32 endOffset = GetChildOffset(accAtOffsetIdx + 1);
  PRInt32 offsetInAcc = aOffset - startOffset;

  nsTextAttrsMgr textAttrsMgr(this, aIncludeDefAttrs, accAtOffset,
                              accAtOffsetIdx);
  nsresult rv = textAttrsMgr.GetAttributes(*aAttributes, &startOffset,
                                           &endOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsIFrame *offsetFrame = accAtOffset->GetFrame();
  if (offsetFrame && offsetFrame->GetType() == nsAccessibilityAtoms::textFrame) {
    nsCOMPtr<nsIDOMNode> node = accAtOffset->GetDOMNode();

    PRInt32 nodeOffset = 0;
    nsresult rv = RenderedToContentOffset(offsetFrame, offsetInAcc,
                                          &nodeOffset);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = GetSpellTextAttribute(node, nodeOffset, &startOffset, &endOffset,
                               aAttributes ? *aAttributes : nsnull);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  *aStartOffset = startOffset;
  *aEndOffset = endOffset;
  return NS_OK;
}



NS_IMETHODIMP
nsHyperTextAccessible::GetDefaultTextAttributes(nsIPersistentProperties **aAttributes)
{
  NS_ENSURE_ARG_POINTER(aAttributes);
  *aAttributes = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIPersistentProperties> attributes =
    do_CreateInstance(NS_PERSISTENTPROPERTIES_CONTRACTID);
  NS_ENSURE_TRUE(attributes, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*aAttributes = attributes);

  nsTextAttrsMgr textAttrsMgr(this, PR_TRUE);
  return textAttrsMgr.GetAttributes(*aAttributes);
}

PRInt32
nsHyperTextAccessible::GetLevelInternal()
{
  nsIAtom *tag = mContent->Tag();
  if (tag == nsAccessibilityAtoms::h1)
    return 1;
  if (tag == nsAccessibilityAtoms::h2)
    return 2;
  if (tag == nsAccessibilityAtoms::h3)
    return 3;
  if (tag == nsAccessibilityAtoms::h4)
    return 4;
  if (tag == nsAccessibilityAtoms::h5)
    return 5;
  if (tag == nsAccessibilityAtoms::h6)
    return 6;

  return nsAccessibleWrap::GetLevelInternal();
}

nsresult
nsHyperTextAccessible::GetAttributesInternal(nsIPersistentProperties *aAttributes)
{
  nsresult rv = nsAccessibleWrap::GetAttributesInternal(aAttributes);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  nsIFrame *frame = GetFrame();
  if (frame && frame->GetType() == nsAccessibilityAtoms::blockFrame) {
    nsAutoString oldValueUnused;
    aAttributes->SetStringProperty(NS_LITERAL_CSTRING("formatting"), NS_LITERAL_STRING("block"),
                                   oldValueUnused);
  }

  if (gLastFocusedNode == GetNode()) {
    PRInt32 lineNumber = GetCaretLineNumber();
    if (lineNumber >= 1) {
      nsAutoString strLineNumber;
      strLineNumber.AppendInt(lineNumber);
      nsAccUtils::SetAccAttr(aAttributes, nsAccessibilityAtoms::lineNumber,
                             strLineNumber);
    }
  }

  return  NS_OK;
}




NS_IMETHODIMP nsHyperTextAccessible::GetCharacterExtents(PRInt32 aOffset, PRInt32 *aX, PRInt32 *aY,
                                                         PRInt32 *aWidth, PRInt32 *aHeight,
                                                         PRUint32 aCoordType)
{
  return GetRangeExtents(aOffset, aOffset + 1, aX, aY, aWidth, aHeight, aCoordType);
}




NS_IMETHODIMP nsHyperTextAccessible::GetRangeExtents(PRInt32 aStartOffset, PRInt32 aEndOffset,
                                                     PRInt32 *aX, PRInt32 *aY,
                                                     PRInt32 *aWidth, PRInt32 *aHeight,
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
nsHyperTextAccessible::GetOffsetAtPoint(PRInt32 aX, PRInt32 aY,
                                        PRUint32 aCoordType, PRInt32 *aOffset)
{
  *aOffset = -1;
  nsCOMPtr<nsIPresShell> shell = GetPresShell();
  if (!shell) {
    return NS_ERROR_FAILURE;
  }
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
  nsPresContext *context = GetPresContext();
  NS_ENSURE_TRUE(context, NS_ERROR_FAILURE);
  nsPoint pointInHyperText(context->DevPixelsToAppUnits(pxInHyperText.x),
                           context->DevPixelsToAppUnits(pxInHyperText.y));

  
  

  
  
  PRInt32 offset = 0;
  PRInt32 childCount = GetChildCount();
  for (PRInt32 childIdx = 0; childIdx < childCount; childIdx++) {
    nsAccessible *childAcc = mChildren[childIdx];

    nsIFrame *primaryFrame = childAcc->GetFrame();
    NS_ENSURE_TRUE(primaryFrame, NS_ERROR_FAILURE);

    nsIFrame *frame = primaryFrame;
    while (frame) {
      nsIContent *content = frame->GetContent();
      NS_ENSURE_TRUE(content, NS_ERROR_FAILURE);
      nsPoint pointInFrame = pointInHyperText - frame->GetOffsetToExternal(hyperFrame);
      nsSize frameSize = frame->GetSize();
      if (pointInFrame.x < frameSize.width && pointInFrame.y < frameSize.height) {
        
        if (frame->GetType() == nsAccessibilityAtoms::textFrame) {
          nsIFrame::ContentOffsets contentOffsets = frame->GetContentOffsetsFromPointExternal(pointInFrame, PR_TRUE);
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
nsHyperTextAccessible::GetLinkCount(PRInt32 *aLinkCount)
{
  NS_ENSURE_ARG_POINTER(aLinkCount);
  *aLinkCount = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  *aLinkCount = GetLinkCount();
  return NS_OK;
}

NS_IMETHODIMP
nsHyperTextAccessible::GetLinkAt(PRInt32 aIndex, nsIAccessibleHyperLink** aLink)
{
  NS_ENSURE_ARG_POINTER(aLink);
  *aLink = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsAccessible* link = GetLinkAt(aIndex);
  if (link)
    CallQueryInterface(link, aLink);

  return NS_OK;
}

NS_IMETHODIMP
nsHyperTextAccessible::GetLinkIndex(nsIAccessibleHyperLink* aLink,
                                    PRInt32* aIndex)
{
  NS_ENSURE_ARG_POINTER(aLink);

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsRefPtr<nsAccessible> link(do_QueryObject(aLink));
  *aIndex = GetLinkIndex(link);
  return NS_OK;
}

NS_IMETHODIMP
nsHyperTextAccessible::GetLinkIndexAtOffset(PRInt32 aOffset,
                                            PRInt32* aLinkIndex)
{
  NS_ENSURE_ARG_POINTER(aLinkIndex);
  *aLinkIndex = -1; 

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  *aLinkIndex = GetLinkIndexAtOffset(aOffset);
  return NS_OK;
}




NS_IMETHODIMP nsHyperTextAccessible::SetAttributes(PRInt32 aStartPos, PRInt32 aEndPos,
                                                   nsISupports *aAttributes)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsHyperTextAccessible::SetTextContents(const nsAString &aText)
{
  PRInt32 numChars;
  GetCharacterCount(&numChars);
  if (numChars == 0 || NS_SUCCEEDED(DeleteText(0, numChars))) {
    return InsertText(aText, 0);
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsHyperTextAccessible::InsertText(const nsAString &aText, PRInt32 aPosition)
{
  nsCOMPtr<nsIEditor> editor;
  GetAssociatedEditor(getter_AddRefs(editor));

  nsCOMPtr<nsIPlaintextEditor> peditor(do_QueryInterface(editor));
  NS_ENSURE_STATE(peditor);

  nsresult rv = SetSelectionRange(aPosition, aPosition);
  NS_ENSURE_SUCCESS(rv, rv);

  return peditor->InsertText(aText);
}

NS_IMETHODIMP
nsHyperTextAccessible::CopyText(PRInt32 aStartPos, PRInt32 aEndPos)
{
  nsCOMPtr<nsIEditor> editor;
  GetAssociatedEditor(getter_AddRefs(editor));
  NS_ENSURE_STATE(editor);

  nsresult rv = SetSelectionRange(aStartPos, aEndPos);
  NS_ENSURE_SUCCESS(rv, rv);

  return editor->Copy();
}

NS_IMETHODIMP
nsHyperTextAccessible::CutText(PRInt32 aStartPos, PRInt32 aEndPos)
{
  nsCOMPtr<nsIEditor> editor;
  GetAssociatedEditor(getter_AddRefs(editor));
  NS_ENSURE_STATE(editor);

  nsresult rv = SetSelectionRange(aStartPos, aEndPos);
  NS_ENSURE_SUCCESS(rv, rv);

  return editor->Cut();
}

NS_IMETHODIMP
nsHyperTextAccessible::DeleteText(PRInt32 aStartPos, PRInt32 aEndPos)
{
  nsCOMPtr<nsIEditor> editor;
  GetAssociatedEditor(getter_AddRefs(editor));
  NS_ENSURE_STATE(editor);

  nsresult rv = SetSelectionRange(aStartPos, aEndPos);
  NS_ENSURE_SUCCESS(rv, rv);

  return editor->DeleteSelection(nsIEditor::eNone);
}

NS_IMETHODIMP
nsHyperTextAccessible::PasteText(PRInt32 aPosition)
{
  nsCOMPtr<nsIEditor> editor;
  GetAssociatedEditor(getter_AddRefs(editor));
  NS_ENSURE_STATE(editor);

  nsresult rv = SetSelectionRange(aPosition, aPosition);
  NS_ENSURE_SUCCESS(rv, rv);

  return editor->Paste(nsIClipboard::kGlobalClipboard);
}

NS_IMETHODIMP
nsHyperTextAccessible::GetAssociatedEditor(nsIEditor **aEditor)
{
  NS_ENSURE_ARG_POINTER(aEditor);
  *aEditor = nsnull;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  if (!mContent->HasFlag(NODE_IS_EDITABLE)) {
    
    nsCOMPtr<nsIAccessible> ancestor, current = this;
    while (NS_SUCCEEDED(current->GetParent(getter_AddRefs(ancestor))) && ancestor) {
      nsRefPtr<nsHyperTextAccessible> ancestorTextAccessible;
      ancestor->QueryInterface(NS_GET_IID(nsHyperTextAccessible),
                               getter_AddRefs(ancestorTextAccessible));
      if (ancestorTextAccessible) {
        
        
        return ancestorTextAccessible->GetAssociatedEditor(aEditor);
      }
      current = ancestor;
    }
    return NS_OK;
  }

  nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem =
    nsCoreUtils::GetDocShellTreeItemFor(mContent);
  nsCOMPtr<nsIEditingSession> editingSession(do_GetInterface(docShellTreeItem));
  if (!editingSession)
    return NS_OK; 

  nsCOMPtr<nsIPresShell> shell = GetPresShell();
  NS_ENSURE_TRUE(shell, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDocument> doc = shell->GetDocument();
  NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

  nsCOMPtr<nsIEditor> editor;
  return editingSession->GetEditorForWindow(doc->GetWindow(), aEditor);
}





nsresult
nsHyperTextAccessible::SetSelectionRange(PRInt32 aStartPos, PRInt32 aEndPos)
{
  nsresult rv = TakeFocus();
  NS_ENSURE_SUCCESS(rv, rv);

  
  SetSelectionBounds(0, aStartPos, aEndPos);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCOMPtr<nsISelection> domSel;
  nsCOMPtr<nsISelectionController> selCon;
  GetSelections(nsISelectionController::SELECTION_NORMAL,
                getter_AddRefs(selCon), getter_AddRefs(domSel));
  if (domSel) {
    PRInt32 numRanges;
    domSel->GetRangeCount(&numRanges);

    for (PRInt32 count = 0; count < numRanges - 1; count ++) {
      nsCOMPtr<nsIDOMRange> range;
      domSel->GetRangeAt(1, getter_AddRefs(range));
      domSel->RemoveRange(range);
    }
  }
  
  if (selCon) {
    
    
    selCon->ScrollSelectionIntoView(nsISelectionController::SELECTION_NORMAL,
      nsISelectionController::SELECTION_FOCUS_REGION, PR_FALSE);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHyperTextAccessible::SetCaretOffset(PRInt32 aCaretOffset)
{
  return SetSelectionRange(aCaretOffset, aCaretOffset);
}




NS_IMETHODIMP
nsHyperTextAccessible::GetCaretOffset(PRInt32 *aCaretOffset)
{
  *aCaretOffset = -1;

  
  

  nsINode* thisNode = GetNode();
  PRBool isInsideOfFocusedNode =
    nsCoreUtils::IsAncestorOf(gLastFocusedNode, thisNode);

  if (!isInsideOfFocusedNode && thisNode != gLastFocusedNode &&
      !nsCoreUtils::IsAncestorOf(thisNode, gLastFocusedNode))
    return NS_OK;

  
  
  nsCOMPtr<nsISelection> domSel;
  nsresult rv = GetSelections(nsISelectionController::SELECTION_NORMAL,
                              nsnull, getter_AddRefs(domSel));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> focusDOMNode;
  rv = domSel->GetFocusNode(getter_AddRefs(focusDOMNode));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 focusOffset;
  rv = domSel->GetFocusOffset(&focusOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCOMPtr<nsINode> focusNode(do_QueryInterface(focusDOMNode));
  if (isInsideOfFocusedNode) {
    nsINode *resultNode =
      nsCoreUtils::GetDOMNodeFromDOMPoint(focusNode, focusOffset);

    if (resultNode != thisNode &&
        !nsCoreUtils::IsAncestorOf(thisNode, resultNode))
      return NS_OK;
  }

  DOMPointToHypertextOffset(focusNode, focusOffset, aCaretOffset);
  return NS_OK;
}

PRInt32 nsHyperTextAccessible::GetCaretLineNumber()
{
  
  
  nsCOMPtr<nsISelection> domSel;
  GetSelections(nsISelectionController::SELECTION_NORMAL, nsnull,
                getter_AddRefs(domSel));
  nsCOMPtr<nsISelectionPrivate> privateSelection(do_QueryInterface(domSel));
  NS_ENSURE_TRUE(privateSelection, -1);
  nsCOMPtr<nsFrameSelection> frameSelection;
  privateSelection->GetFrameSelection(getter_AddRefs(frameSelection));
  NS_ENSURE_TRUE(frameSelection, -1);

  nsCOMPtr<nsIDOMNode> caretNode;
  domSel->GetFocusNode(getter_AddRefs(caretNode));
  nsCOMPtr<nsIContent> caretContent = do_QueryInterface(caretNode);
  if (!caretContent || !nsCoreUtils::IsAncestorOf(GetNode(), caretContent))
    return -1;

  PRInt32 caretOffset, returnOffsetUnused;
  domSel->GetFocusOffset(&caretOffset);
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

    
    nsIFrame *sibling = parentFrame->GetFirstChild(nsnull);
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

nsresult
nsHyperTextAccessible::GetSelections(PRInt16 aType,
                                     nsISelectionController **aSelCon,
                                     nsISelection **aDomSel,
                                     nsCOMArray<nsIDOMRange>* aRanges)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  if (aSelCon) {
    *aSelCon = nsnull;
  }
  if (aDomSel) {
    *aDomSel = nsnull;
  }
  if (aRanges) {
    aRanges->Clear();
  }
  
  nsCOMPtr<nsISelection> domSel;
  nsCOMPtr<nsISelectionController> selCon;

  nsCOMPtr<nsIEditor> editor;
  GetAssociatedEditor(getter_AddRefs(editor));
  nsCOMPtr<nsIPlaintextEditor> peditor(do_QueryInterface(editor));
  if (peditor) {
    
    
    
    
    editor->GetSelectionController(getter_AddRefs(selCon));
  }
  else {
    
    
    nsIFrame *frame = GetFrame();
    NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

    
    frame->GetSelectionController(GetPresContext(),
                                  getter_AddRefs(selCon));
  }
  NS_ENSURE_TRUE(selCon, NS_ERROR_FAILURE);

  selCon->GetSelection(aType, getter_AddRefs(domSel));
  NS_ENSURE_TRUE(domSel, NS_ERROR_FAILURE);

  if (aSelCon) {
    NS_ADDREF(*aSelCon = selCon);
  }
  if (aDomSel) {
    NS_ADDREF(*aDomSel = domSel);
  }

  if (aRanges) {
    nsCOMPtr<nsISelection2> selection2(do_QueryInterface(domSel));
    NS_ENSURE_TRUE(selection2, NS_ERROR_FAILURE);

    nsCOMPtr<nsINode> startNode = GetNode();
    if (peditor) {
      nsCOMPtr<nsIDOMElement> editorRoot;
      editor->GetRootElement(getter_AddRefs(editorRoot));
      startNode = do_QueryInterface(editorRoot);
    }
    NS_ENSURE_STATE(startNode);

    PRUint32 childCount = startNode->GetChildCount();
    nsCOMPtr<nsIDOMNode> startDOMNode(do_QueryInterface(startNode));
    nsresult rv = selection2->
      GetRangesForIntervalCOMArray(startDOMNode, 0, startDOMNode, childCount,
                                   PR_TRUE, aRanges);
    NS_ENSURE_SUCCESS(rv, rv);
    
    PRInt32 numRanges = aRanges->Count();
    for (PRInt32 count = 0; count < numRanges; count ++) {
      PRBool isCollapsed;
      (*aRanges)[count]->GetCollapsed(&isCollapsed);
      if (isCollapsed) {
        aRanges->RemoveObjectAt(count);
        -- numRanges;
        -- count;
      }
    }
  }

  return NS_OK;
}




NS_IMETHODIMP nsHyperTextAccessible::GetSelectionCount(PRInt32 *aSelectionCount)
{
  nsCOMPtr<nsISelection> domSel;
  nsCOMArray<nsIDOMRange> ranges;
  nsresult rv = GetSelections(nsISelectionController::SELECTION_NORMAL,
                              nsnull, nsnull, &ranges);
  NS_ENSURE_SUCCESS(rv, rv);

  *aSelectionCount = ranges.Count();

  return NS_OK;
}




NS_IMETHODIMP nsHyperTextAccessible::GetSelectionBounds(PRInt32 aSelectionNum, PRInt32 *aStartOffset, PRInt32 *aEndOffset)
{
  *aStartOffset = *aEndOffset = 0;

  nsCOMPtr<nsISelection> domSel;
  nsCOMArray<nsIDOMRange> ranges;
  nsresult rv = GetSelections(nsISelectionController::SELECTION_NORMAL,
                              nsnull, getter_AddRefs(domSel), &ranges);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 rangeCount = ranges.Count();
  if (aSelectionNum < 0 || aSelectionNum >= rangeCount)
    return NS_ERROR_INVALID_ARG;

  nsCOMPtr<nsIDOMRange> range = ranges[aSelectionNum];

  
  nsCOMPtr<nsIDOMNode> startDOMNode;
  range->GetStartContainer(getter_AddRefs(startDOMNode));
  nsCOMPtr<nsINode> startNode(do_QueryInterface(startDOMNode));
  PRInt32 startOffset;
  range->GetStartOffset(&startOffset);

  
  nsCOMPtr<nsIDOMNode> endDOMNode;
  range->GetEndContainer(getter_AddRefs(endDOMNode));
  nsCOMPtr<nsINode> endNode(do_QueryInterface(endDOMNode));
  PRInt32 endOffset;
  range->GetEndOffset(&endOffset);

  PRInt16 rangeCompareResult;
  rv = range->CompareBoundaryPoints(nsIDOMRange::START_TO_END, range, &rangeCompareResult);
  NS_ENSURE_SUCCESS(rv, rv);

  if (rangeCompareResult < 0) {
    
    
    startNode.swap(endNode);
    PRInt32 tempOffset = startOffset;
    startOffset = endOffset;
    endOffset = tempOffset;
  }

  nsAccessible *startAccessible =
    DOMPointToHypertextOffset(startNode, startOffset, aStartOffset);
  if (!startAccessible) {
    *aStartOffset = 0; 
  }

  DOMPointToHypertextOffset(endNode, endOffset, aEndOffset, PR_TRUE);
  return NS_OK;
}




NS_IMETHODIMP
nsHyperTextAccessible::SetSelectionBounds(PRInt32 aSelectionNum,
                                          PRInt32 aStartOffset,
                                          PRInt32 aEndOffset)
{
  nsCOMPtr<nsISelection> domSel;
  nsresult rv = GetSelections(nsISelectionController::SELECTION_NORMAL,
                              nsnull, getter_AddRefs(domSel));
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRBool isOnlyCaret = (aStartOffset == aEndOffset);

  PRInt32 rangeCount;
  domSel->GetRangeCount(&rangeCount);
  nsCOMPtr<nsIDOMRange> range;
  if (aSelectionNum == rangeCount) { 
    range = do_CreateInstance(kRangeCID);
    NS_ENSURE_TRUE(range, NS_ERROR_OUT_OF_MEMORY);
  }
  else if (aSelectionNum < 0 || aSelectionNum > rangeCount) {
    return NS_ERROR_INVALID_ARG;
  }
  else {
    domSel->GetRangeAt(aSelectionNum, getter_AddRefs(range));
    NS_ENSURE_TRUE(range, NS_ERROR_FAILURE);
  }

  PRInt32 startOffset, endOffset;
  nsCOMPtr<nsIDOMNode> startNode, endNode;

  rv = HypertextOffsetsToDOMRange(aStartOffset, aEndOffset,
                                  getter_AddRefs(startNode), &startOffset,
                                  getter_AddRefs(endNode), &endOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = range->SetStart(startNode, startOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = isOnlyCaret ? range->Collapse(PR_TRUE) :
                     range->SetEnd(endNode, endOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aSelectionNum == rangeCount) { 
    return domSel->AddRange(range);
  }
  return NS_OK;
}




NS_IMETHODIMP nsHyperTextAccessible::AddSelection(PRInt32 aStartOffset, PRInt32 aEndOffset)
{
  nsCOMPtr<nsISelection> domSel;
  nsresult rv = GetSelections(nsISelectionController::SELECTION_NORMAL,
                              nsnull, getter_AddRefs(domSel));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 rangeCount;
  domSel->GetRangeCount(&rangeCount);

  return SetSelectionBounds(rangeCount, aStartOffset, aEndOffset);
}




NS_IMETHODIMP nsHyperTextAccessible::RemoveSelection(PRInt32 aSelectionNum)
{
  nsCOMPtr<nsISelection> domSel;
  nsresult rv = GetSelections(nsISelectionController::SELECTION_NORMAL,
                              nsnull, getter_AddRefs(domSel));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 rangeCount;
  domSel->GetRangeCount(&rangeCount);
  if (aSelectionNum < 0 || aSelectionNum >= rangeCount)
    return NS_ERROR_INVALID_ARG;

  nsCOMPtr<nsIDOMRange> range;
  domSel->GetRangeAt(aSelectionNum, getter_AddRefs(range));
  return domSel->RemoveRange(range);
}




NS_IMETHODIMP
nsHyperTextAccessible::ScrollSubstringTo(PRInt32 aStartIndex, PRInt32 aEndIndex,
                                         PRUint32 aScrollType)
{
  PRInt32 startOffset, endOffset;
  nsCOMPtr<nsIDOMNode> startNode, endNode;

  nsresult rv = HypertextOffsetsToDOMRange(aStartIndex, aEndIndex,
                                           getter_AddRefs(startNode),
                                           &startOffset,
                                           getter_AddRefs(endNode),
                                           &endOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  return nsCoreUtils::ScrollSubstringTo(GetFrame(), startNode, startOffset,
                                        endNode, endOffset, aScrollType);
}





NS_IMETHODIMP
nsHyperTextAccessible::ScrollSubstringToPoint(PRInt32 aStartIndex,
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

  PRInt32 startOffset, endOffset;
  nsCOMPtr<nsIDOMNode> startNode, endNode;

  rv = HypertextOffsetsToDOMRange(aStartIndex, aEndIndex,
                                  getter_AddRefs(startNode), &startOffset,
                                  getter_AddRefs(endNode), &endOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  nsPresContext *presContext = frame->PresContext();

  PRBool initialScrolled = PR_FALSE;
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

        rv = nsCoreUtils::ScrollSubstringTo(GetFrame(), startNode, startOffset,
                                            endNode, endOffset,
                                            vPercent, hPercent);
        NS_ENSURE_SUCCESS(rv, rv);

        initialScrolled = PR_TRUE;
      } else {
        
        
        
        
        nsCoreUtils::ScrollFrameToPoint(parentFrame, frame, coords);
      }
    }
    frame = parentFrame;
  }

  return NS_OK;
}




void
nsHyperTextAccessible::InvalidateChildren()
{
  mOffsets.Clear();

  nsAccessibleWrap::InvalidateChildren();
}




nsresult nsHyperTextAccessible::ContentToRenderedOffset(nsIFrame *aFrame, PRInt32 aContentOffset,
                                                        PRUint32 *aRenderedOffset)
{
  if (!aFrame) {
    
    
    *aRenderedOffset = 0;
    return NS_OK;
  }
  NS_ASSERTION(aFrame->GetType() == nsAccessibilityAtoms::textFrame,
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

nsresult nsHyperTextAccessible::RenderedToContentOffset(nsIFrame *aFrame, PRUint32 aRenderedOffset,
                                                        PRInt32 *aContentOffset)
{
  *aContentOffset = 0;
  NS_ENSURE_TRUE(aFrame, NS_ERROR_FAILURE);

  NS_ASSERTION(aFrame->GetType() == nsAccessibilityAtoms::textFrame,
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




PRInt32
nsHyperTextAccessible::GetChildOffset(PRUint32 aChildIndex,
                                      PRBool aInvalidateAfter)
{
  if (aChildIndex == 0)
    return aChildIndex;

  PRInt32 count = mOffsets.Length() - aChildIndex;
  if (count > 0) {
    if (aInvalidateAfter)
      mOffsets.RemoveElementsAt(aChildIndex, count);

    return mOffsets[aChildIndex - 1];
  }

  PRUint32 lastOffset = mOffsets.IsEmpty() ?
    0 : mOffsets[mOffsets.Length() - 1];

  EnsureChildren();
  while (mOffsets.Length() < aChildIndex) {
    nsAccessible* child = mChildren[mOffsets.Length()];
    lastOffset += nsAccUtils::TextLength(child);
    mOffsets.AppendElement(lastOffset);
  }

  return mOffsets[aChildIndex - 1];
}

PRInt32
nsHyperTextAccessible::GetChildIndexAtOffset(PRUint32 aOffset)
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

  PRUint32 childCount = GetChildCount();
  while (mOffsets.Length() < childCount) {
    nsAccessible* child = GetChildAt(mOffsets.Length());
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
nsHyperTextAccessible::GetDOMPointByFrameOffset(nsIFrame *aFrame,
                                                PRInt32 aOffset,
                                                nsIAccessible *aAccessible,
                                                nsIDOMNode **aNode,
                                                PRInt32 *aNodeOffset)
{
  NS_ENSURE_ARG(aAccessible);

  nsCOMPtr<nsIDOMNode> node;

  if (!aFrame) {
    
    
    nsCOMPtr<nsIAccessNode> accessNode(do_QueryInterface(aAccessible));

    nsCOMPtr<nsIDOMNode> DOMNode;
    accessNode->GetDOMNode(getter_AddRefs(DOMNode));
    nsCOMPtr<nsIContent> content(do_QueryInterface(DOMNode));
    NS_ENSURE_STATE(content);

    nsCOMPtr<nsIContent> parent(content->GetParent());
    NS_ENSURE_STATE(parent);

    *aNodeOffset = parent->IndexOf(content) + 1;
    node = do_QueryInterface(parent);

  } else if (aFrame->GetType() == nsAccessibilityAtoms::textFrame) {
    nsCOMPtr<nsIContent> content(aFrame->GetContent());
    NS_ENSURE_STATE(content);

    nsIFrame *primaryFrame = content->GetPrimaryFrame();
    nsresult rv = RenderedToContentOffset(primaryFrame, aOffset, aNodeOffset);
    NS_ENSURE_SUCCESS(rv, rv);

    node = do_QueryInterface(content);

  } else {
    nsCOMPtr<nsIContent> content(aFrame->GetContent());
    NS_ENSURE_STATE(content);

    nsCOMPtr<nsIContent> parent(content->GetParent());
    NS_ENSURE_STATE(parent);

    *aNodeOffset = parent->IndexOf(content);
    node = do_QueryInterface(parent);
  }

  NS_IF_ADDREF(*aNode = node);
  return NS_OK;
}


nsresult
nsHyperTextAccessible::DOMRangeBoundToHypertextOffset(nsIDOMRange *aRange,
                                                      PRBool aIsStartBound,
                                                      PRBool aIsStartHTOffset,
                                                      PRInt32 *aHTOffset)
{
  nsCOMPtr<nsIDOMNode> DOMNode;
  PRInt32 nodeOffset = 0;

  nsresult rv;
  if (aIsStartBound) {
    rv = aRange->GetStartContainer(getter_AddRefs(DOMNode));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aRange->GetStartOffset(&nodeOffset);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    rv = aRange->GetEndContainer(getter_AddRefs(DOMNode));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aRange->GetEndOffset(&nodeOffset);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsINode> node(do_QueryInterface(DOMNode));
  nsAccessible *startAcc =
    DOMPointToHypertextOffset(node, nodeOffset, aHTOffset);

  if (aIsStartHTOffset && !startAcc)
    *aHTOffset = 0;

  return NS_OK;
}


nsresult
nsHyperTextAccessible::GetSpellTextAttribute(nsIDOMNode *aNode,
                                             PRInt32 aNodeOffset,
                                             PRInt32 *aHTStartOffset,
                                             PRInt32 *aHTEndOffset,
                                             nsIPersistentProperties *aAttributes)
{
  nsCOMArray<nsIDOMRange> ranges;
  nsresult rv = GetSelections(nsISelectionController::SELECTION_SPELLCHECK,
                              nsnull, nsnull, &ranges);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 rangeCount = ranges.Count();
  if (!rangeCount)
    return NS_OK;

  for (PRInt32 index = 0; index < rangeCount; index++) {
    nsCOMPtr<nsIDOMRange> range = ranges[index];
    nsCOMPtr<nsIDOMNSRange> nsrange(do_QueryInterface(range));
    NS_ENSURE_STATE(nsrange);

    PRInt16 result;
    rv = nsrange->ComparePoint(aNode, aNodeOffset, &result);
    NS_ENSURE_SUCCESS(rv, rv);
    
    
    
    if (result == 0) {
      nsCOMPtr<nsIDOMNode> end;
      rv = range->GetEndContainer(getter_AddRefs(end));
      NS_ENSURE_SUCCESS(rv, rv);
      PRInt32 endOffset;
      rv = range->GetEndOffset(&endOffset);
      NS_ENSURE_SUCCESS(rv, rv);
      if (aNode == end && aNodeOffset == endOffset) {
        result = 1;
      }
    }

    if (result == 1) { 
      PRInt32 startHTOffset = 0;
      rv = DOMRangeBoundToHypertextOffset(range, PR_FALSE, PR_TRUE,
                                          &startHTOffset);
      NS_ENSURE_SUCCESS(rv, rv);

      if (startHTOffset > *aHTStartOffset)
        *aHTStartOffset = startHTOffset;

    } else if (result == -1) { 
      PRInt32 endHTOffset = 0;
      rv = DOMRangeBoundToHypertextOffset(range, PR_TRUE, PR_FALSE,
                                          &endHTOffset);
      NS_ENSURE_SUCCESS(rv, rv);

      if (endHTOffset < *aHTEndOffset)
        *aHTEndOffset = endHTOffset;

    } else { 
      PRInt32 startHTOffset = 0;
      rv = DOMRangeBoundToHypertextOffset(range, PR_TRUE, PR_TRUE,
                                          &startHTOffset);
      NS_ENSURE_SUCCESS(rv, rv);

      PRInt32 endHTOffset = 0;
      rv = DOMRangeBoundToHypertextOffset(range, PR_FALSE, PR_FALSE,
                                          &endHTOffset);
      NS_ENSURE_SUCCESS(rv, rv);

      if (startHTOffset > *aHTStartOffset)
        *aHTStartOffset = startHTOffset;
      if (endHTOffset < *aHTEndOffset)
        *aHTEndOffset = endHTOffset;

      if (aAttributes) {
        nsAccUtils::SetAccAttr(aAttributes, nsAccessibilityAtoms::invalid,
                               NS_LITERAL_STRING("spelling"));
      }

      return NS_OK;
    }
  }

  return NS_OK;
}
