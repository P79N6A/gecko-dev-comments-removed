






































#include "nsHyperTextAccessible.h"
#include "nsAccessibilityAtoms.h"
#include "nsAccessibilityService.h"
#include "nsAccessibleTreeWalker.h"
#include "nsPIAccessNode.h"
#include "nsIClipboard.h"
#include "nsContentCID.h"
#include "nsIDOMAbstractView.h"
#include "nsIDOMCharacterData.h"
#include "nsIDOMDocument.h"
#include "nsPIDOMWindow.h"        
#include "nsIDOMDocumentView.h"
#include "nsIDOMRange.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDOMXULDocument.h"
#include "nsIEditingSession.h"
#include "nsIEditor.h"
#include "nsIFontMetrics.h"
#include "nsIFrame.h"
#include "nsFrameSelection.h"
#include "nsIScrollableFrame.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIPlaintextEditor.h"
#include "nsISelection2.h"
#include "nsISelectionPrivate.h"
#include "nsIServiceManager.h"
#include "nsTextFragment.h"
#include "gfxSkipChars.h"

static NS_DEFINE_IID(kRangeCID, NS_RANGE_CID);





NS_IMPL_ADDREF_INHERITED(nsHyperTextAccessible, nsAccessibleWrap)
NS_IMPL_RELEASE_INHERITED(nsHyperTextAccessible, nsAccessibleWrap)

nsresult nsHyperTextAccessible::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  *aInstancePtr = nsnull;

  nsCOMPtr<nsIDOMXULDocument> xulDoc(do_QueryInterface(mDOMNode));
  if (mDOMNode && !xulDoc) {
    
    
    
    
    

    if (aIID.Equals(NS_GET_IID(nsHyperTextAccessible))) {
      *aInstancePtr = static_cast<nsHyperTextAccessible*>(this);
      NS_ADDREF_THIS();
      return NS_OK;
    }

    if (mRoleMapEntry &&
        (mRoleMapEntry->role == nsIAccessibleRole::ROLE_GRAPHIC ||
         mRoleMapEntry->role == nsIAccessibleRole::ROLE_IMAGE_MAP)) {
      
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
  }

  return nsAccessible::QueryInterface(aIID, aInstancePtr);
}

nsHyperTextAccessible::nsHyperTextAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsAccessibleWrap(aNode, aShell)
{
}

NS_IMETHODIMP nsHyperTextAccessible::GetRole(PRUint32 *aRole)
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(mDOMNode);
  if (!content) {
    return NS_ERROR_FAILURE;
  }

  nsIAtom *tag = content->Tag();

  if (tag == nsAccessibilityAtoms::form) {
    *aRole = nsIAccessibleRole::ROLE_FORM;
  }
  else if (tag == nsAccessibilityAtoms::div ||
           tag == nsAccessibilityAtoms::blockquote) {
    *aRole = nsIAccessibleRole::ROLE_SECTION;
  }
  else if (tag == nsAccessibilityAtoms::h1 ||
           tag == nsAccessibilityAtoms::h2 ||
           tag == nsAccessibilityAtoms::h3 ||
           tag == nsAccessibilityAtoms::h4 ||
           tag == nsAccessibilityAtoms::h5 ||
           tag == nsAccessibilityAtoms::h6) {
    *aRole = nsIAccessibleRole::ROLE_HEADING;
  }
  else {
    nsIFrame *frame = GetFrame();
    if (frame && frame->GetType() == nsAccessibilityAtoms::blockFrame) {
      *aRole = nsIAccessibleRole::ROLE_PARAGRAPH;
    }
    else {
      *aRole = nsIAccessibleRole::ROLE_TEXT_CONTAINER; 
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsHyperTextAccessible::GetState(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsAccessibleWrap::GetState(aState, aExtraState);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!mDOMNode || !aExtraState)
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

void nsHyperTextAccessible::CacheChildren()
{
  if (!mWeakShell) {
    
    mAccChildCount = eChildCountUninitialized;
    return;
  }

  
  if (mAccChildCount == eChildCountUninitialized) {
    PRUint32 role;
    GetRole(&role);
    if (role != nsIAccessibleRole::ROLE_ENTRY && role != nsIAccessibleRole::ROLE_PASSWORD_TEXT) {
      nsAccessible::CacheChildren();
      return;
    }
    nsCOMPtr<nsIEditor> editor;
    GetAssociatedEditor(getter_AddRefs(editor));
    if (!editor) {
      nsAccessible::CacheChildren();
      return;
    }
    mAccChildCount = 0;  
    nsCOMPtr<nsIDOMElement> editorRoot;
    editor->GetRootElement(getter_AddRefs(editorRoot));
    nsCOMPtr<nsIDOMNode> editorRootDOMNode = do_QueryInterface(editorRoot);
    if (!editorRootDOMNode) {
      return;
    }
    nsAccessibleTreeWalker walker(mWeakShell, editorRootDOMNode, PR_TRUE);
    nsCOMPtr<nsPIAccessible> privatePrevAccessible;
    PRInt32 childCount = 0;
    walker.GetFirstChild();
    SetFirstChild(walker.mState.accessible);

    while (walker.mState.accessible) {
      ++ childCount;
      privatePrevAccessible = do_QueryInterface(walker.mState.accessible);
      privatePrevAccessible->SetParent(this);
      walker.GetNextSibling();
      privatePrevAccessible->SetNextSibling(walker.mState.accessible);
    }
    mAccChildCount = childCount;
  }
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

  nsCOMPtr<nsIRenderingContext> rc;
  shell->CreateRenderingContext(frame, getter_AddRefs(rc));
  NS_ENSURE_TRUE(rc, screenRect);

  const nsStyleFont *font = frame->GetStyleFont();
  const nsStyleVisibility *visibility = frame->GetStyleVisibility();

  rv = rc->SetFont(font->mFont, visibility->mLangGroup);
  NS_ENSURE_SUCCESS(rv, screenRect);

  nsPresContext *context = shell->GetPresContext();

  while (frame && startContentOffset < endContentOffset) {
    
    
    
    
    nsIntRect frameScreenRect = frame->GetScreenRectExternal();

    
    PRInt32 startFrameTextOffset, endFrameTextOffset;
    frame->GetOffsets(startFrameTextOffset, endFrameTextOffset);
    PRInt32 frameTotalTextLength = endFrameTextOffset - startFrameTextOffset;
    PRInt32 seekLength = endContentOffset - startContentOffset;
    PRInt32 frameSubStringLength = PR_MIN(frameTotalTextLength - startContentOffsetInFrame, seekLength);

    
    nsPoint frameTextStartPoint;
    rv = frame->GetPointFromOffset(startContentOffset, &frameTextStartPoint);
    NS_ENSURE_SUCCESS(rv, nsRect());   
    frameScreenRect.x += context->AppUnitsToDevPixels(frameTextStartPoint.x);

    
    nsPoint frameTextEndPoint;
    rv = frame->GetPointFromOffset(startContentOffset + frameSubStringLength, &frameTextEndPoint);
    NS_ENSURE_SUCCESS(rv, nsRect());   
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
                                     nsIAccessible **aStartAcc,
                                     nsIAccessible **aEndAcc)
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
  nsCOMPtr<nsIAccessible> accessible, lastAccessible;

  gfxSkipChars skipChars;
  gfxSkipCharsIterator iter;

  
  
  while (NextChild(accessible)) {
    lastAccessible = accessible;
    nsCOMPtr<nsPIAccessNode> accessNode(do_QueryInterface(accessible));
    nsIFrame *frame = accessNode->GetFrame();
    if (!frame) {
      continue;
    }
    nsIFrame *primaryFrame = frame;
    if (IsText(accessible)) {
      
      
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
        
        
        substringEndOffset = TextLength(accessible);
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
              NS_ADDREF(*aEndAcc = accessible);
          }
          if (substringEndOffset > endOffset) {
            
            substringEndOffset = endOffset;
          }
          aEndOffset = endOffset;
        }
        if (aText) {
          nsCOMPtr<nsPIAccessible> pAcc(do_QueryInterface(accessible));
          pAcc->AppendTextTo(*aText, startOffset,
                             substringEndOffset - startOffset);
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
            NS_ADDREF(*aStartAcc = accessible);
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
            } else if (MustPrune(this)) {
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
            NS_ADDREF(*aStartAcc = accessible);
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
  if (!mDOMNode) {
    return NS_ERROR_FAILURE;
  }
  return GetPosAndText(aStartOffset, aEndOffset, &aText) ? NS_OK : NS_ERROR_FAILURE;
}




NS_IMETHODIMP nsHyperTextAccessible::GetCharacterCount(PRInt32 *aCharacterCount)
{
  *aCharacterCount = 0;
  if (!mDOMNode) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIAccessible> accessible;

  while (NextChild(accessible)) {
    PRInt32 textLength = TextLength(accessible);
    NS_ENSURE_TRUE(textLength >= 0, nsnull);
    *aCharacterCount += textLength;
  }
  return NS_OK;
}




NS_IMETHODIMP nsHyperTextAccessible::GetCharacterAtOffset(PRInt32 aOffset, PRUnichar *aCharacter)
{
  if (!mDOMNode) {
    return NS_ERROR_FAILURE;
  }
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

nsresult nsHyperTextAccessible::DOMPointToHypertextOffset(nsIDOMNode* aNode, PRInt32 aNodeOffset,
                                                          PRInt32* aHyperTextOffset,
                                                          nsIAccessible **aFinalAccessible,
                                                          PRBool aIsEndOffset)
{
  
  
  NS_ENSURE_ARG_POINTER(aHyperTextOffset);
  *aHyperTextOffset = 0;
  NS_ENSURE_ARG_POINTER(aNode);
  if (aFinalAccessible) {
    *aFinalAccessible = nsnull;
  }

  PRUint32 addTextOffset = 0;
  nsCOMPtr<nsIDOMNode> findNode;

  unsigned short nodeType;
  aNode->GetNodeType(&nodeType);
  if (aNodeOffset == -1) {
    findNode = aNode;
  }
  else if (nodeType == nsIDOMNode::TEXT_NODE) {
    
    
    
    nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
    NS_ASSERTION(content, "No nsIContent for dom node");
    nsCOMPtr<nsIPresShell> presShell = GetPresShell();
    NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);
    nsIFrame *frame = presShell->GetPrimaryFrameFor(content);
    NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);
    nsresult rv = ContentToRenderedOffset(frame, aNodeOffset, &addTextOffset);
    NS_ENSURE_SUCCESS(rv, rv);
    
    findNode = aNode;
  }
  else {
    
    nsCOMPtr<nsIContent> parentContent(do_QueryInterface(aNode));
    
    NS_ENSURE_TRUE(parentContent, NS_ERROR_FAILURE);
    
    
    
    
     
    findNode = do_QueryInterface(parentContent->GetChildAt(aNodeOffset));
    if (!findNode && !aNodeOffset) {
      if (SameCOMIdentity(parentContent, mDOMNode)) {
        
        
        *aHyperTextOffset = 0;
        return NS_OK;
      }
      findNode = do_QueryInterface(parentContent); 
    }
  }

  
  
  nsCOMPtr<nsIAccessible> descendantAccessible;
  if (findNode) {
    descendantAccessible = GetFirstAvailableAccessible(findNode);
  }
  
  nsCOMPtr<nsIAccessible> childAccessible;
  while (descendantAccessible) {
    nsCOMPtr<nsIAccessible> parentAccessible;
    descendantAccessible->GetParent(getter_AddRefs(parentAccessible));
    if (this == parentAccessible) {
      childAccessible = descendantAccessible;
      break;
    }
    
    
    
    
    
    if (aIsEndOffset) {
      
      
    
    
    addTextOffset = addTextOffset > 0;
    }
    else {
      
      
      
      addTextOffset = (TextLength(descendantAccessible) == addTextOffset) ? 1 : 0;
    }
    descendantAccessible = parentAccessible;
  }  

  
  
  
  
  nsCOMPtr<nsIAccessible> accessible;
  while (NextChild(accessible) && accessible != childAccessible) {
    PRInt32 textLength = TextLength(accessible);
    NS_ENSURE_TRUE(textLength >= 0, nsnull);
    *aHyperTextOffset += textLength;
  }
  if (accessible) {
    *aHyperTextOffset += addTextOffset;
    NS_ASSERTION(accessible == childAccessible, "These should be equal whenever we exit loop and accessible != nsnull");
    if (aFinalAccessible && (NextChild(accessible) || static_cast<PRInt32>(addTextOffset) < TextLength(childAccessible))) {  
      
      NS_ADDREF(*aFinalAccessible = childAccessible);
    }
  }

  return NS_OK;
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

  nsCOMPtr<nsIAccessible> startAcc, endAcc;
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
                                         nsIAccessible *aFromAccessible,
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
  if (IsText(aFromAccessible)) {
    nsCOMPtr<nsPIAccessNode> accessNode(do_QueryInterface(aFromAccessible));
    NS_ASSERTION(accessNode, "nsIAccessible doesn't support nsPIAccessNode");

    nsIFrame *frame = accessNode->GetFrame();
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
  nsCOMPtr<nsIDOMNode> resultNode = do_QueryInterface(pos.mResultContent);
  NS_ENSURE_TRUE(resultNode, -1);

  nsCOMPtr<nsIAccessible> finalAccessible;
  rv = DOMPointToHypertextOffset(resultNode, pos.mContentOffset, &hyperTextOffset,
                                 getter_AddRefs(finalAccessible),
                                 aDirection == eDirNext);
  
  
  NS_ENSURE_SUCCESS(rv, -1);

  if (!finalAccessible && aDirection == eDirPrevious) {
    
    
    hyperTextOffset = 0;
  }  
  else if (aAmount == eSelectBeginLine) {
    
    if (pos.mContentOffset == 0 && mFirstChild && 
        Role(mFirstChild) == nsIAccessibleRole::ROLE_STATICTEXT &&
        TextLength(mFirstChild) == hyperTextOffset) {
      
      hyperTextOffset = 0;
    }
    if (!aNeedsStart && hyperTextOffset > 0) {
      -- hyperTextOffset;
    }
  }
  else if (aAmount == eSelectEndLine && finalAccessible) { 
    
    
    if (Role(finalAccessible) == nsIAccessibleRole::ROLE_WHITESPACE) {  
      
      
      
      
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
      nsresult rv = GetSelections(nsnull, getter_AddRefs(domSel));
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

  PRInt32 startOffset = aOffset + (aBoundaryType == BOUNDARY_LINE_END);  
  PRInt32 endOffset = startOffset;

  
  nsCOMPtr<nsIAccessible> startAcc;
  nsIFrame *startFrame = GetPosAndText(startOffset, endOffset, nsnull, nsnull,
                                       nsnull, getter_AddRefs(startAcc));

  if (!startFrame) {
    PRInt32 textLength;
    GetCharacterCount(&textLength);
    if (aBoundaryType == BOUNDARY_LINE_START && aOffset > 0 && aOffset == textLength) {
      
      nsCOMPtr<nsPIAccessNode> startAccessNode = do_QueryInterface(startAcc);
      if (startAccessNode) {
        startFrame = startAccessNode->GetFrame();
      }
    }
    if (!startFrame) {
      return aOffset > textLength ? NS_ERROR_FAILURE : NS_OK;
    }
    else {
      
      startFrame = startFrame->GetLastContinuation();
    }
  }

  nsSelectionAmount amount;
  PRBool needsStart = PR_FALSE;
  switch (aBoundaryType)
  {
  case BOUNDARY_CHAR:
    amount = eSelectCharacter;
    if (aType == eGetAt) {
      aType = eGetAfter; 
    }
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
      
      
      
      nsIContent *textContent = startFrame->GetContent();
      
      
      
      
      PRInt32 textLength = textContent ? textContent->TextLength() : 1;
      if (textLength < 0) {
        return NS_ERROR_FAILURE;
      }
      *aStartOffset = aOffset - startOffset;
      *aEndOffset = *aStartOffset + textLength;
      startOffset = *aStartOffset;
      endOffset = *aEndOffset;
      return GetText(startOffset, endOffset, aText);
    }
  default:  
    return NS_ERROR_INVALID_ARG;
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
    nsCOMPtr<nsIAccessible> endAcc;
    nsIFrame *endFrame = GetPosAndText(startOffset, endOffset, nsnull, nsnull,
                                       nsnull, getter_AddRefs(endAcc));
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

NS_IMETHODIMP nsHyperTextAccessible::GetAttributeRange(PRInt32 aOffset, PRInt32 *aRangeStartOffset, 
                                                       PRInt32 *aRangeEndOffset, nsIAccessible **aAccessibleWithAttrs)
{
  
  *aRangeStartOffset = *aRangeEndOffset = 0;
  *aAccessibleWithAttrs = nsnull;

  if (!mDOMNode) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIAccessible> accessible;
  
  while (NextChild(accessible)) {
    PRInt32 length = TextLength(accessible);
    NS_ENSURE_TRUE(length >= 0, NS_ERROR_FAILURE);
    if (*aRangeStartOffset + length > aOffset) {
      *aRangeEndOffset = *aRangeStartOffset + length;
      NS_ADDREF(*aAccessibleWithAttrs = accessible);
      return NS_OK;
    }
    *aRangeStartOffset += length;
  }

  return NS_ERROR_FAILURE;
}

nsresult
nsHyperTextAccessible::GetAttributesInternal(nsIPersistentProperties *aAttributes)
{
  if (!mDOMNode) {
    return NS_ERROR_FAILURE;  
  }

  nsresult rv = nsAccessibleWrap::GetAttributesInternal(aAttributes);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  NS_ENSURE_TRUE(content, NS_ERROR_UNEXPECTED);
  nsIAtom *tag = content->Tag();

  PRInt32 headLevel = 0;
  if (tag == nsAccessibilityAtoms::h1)
    headLevel = 1;
  else if (tag == nsAccessibilityAtoms::h2)
    headLevel = 2;
  else if (tag == nsAccessibilityAtoms::h3)
    headLevel = 3;
  else if (tag == nsAccessibilityAtoms::h4)
    headLevel = 4;
  else if (tag == nsAccessibilityAtoms::h5)
    headLevel = 5;
  else if (tag == nsAccessibilityAtoms::h6)
    headLevel = 6;

  if (headLevel) {
    nsAutoString strHeadLevel;
    strHeadLevel.AppendInt(headLevel);
    nsAccUtils::SetAccAttr(aAttributes, nsAccessibilityAtoms::level,
                           strHeadLevel);
  }
  
  
  
  nsIFrame *frame = GetFrame();
  if (frame && frame->GetType() == nsAccessibilityAtoms::blockFrame) {
    nsAutoString oldValueUnused;
    aAttributes->SetStringProperty(NS_LITERAL_CSTRING("formatting"), NS_LITERAL_STRING("block"),
                                   oldValueUnused);
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
  nsPoint pointInHyperText(coords.x - frameScreenRect.x,
                           coords.y - frameScreenRect.y);
  nsPresContext *context = GetPresContext();
  NS_ENSURE_TRUE(context, NS_ERROR_FAILURE);
  pointInHyperText.x = context->DevPixelsToAppUnits(pointInHyperText.x);
  pointInHyperText.y = context->DevPixelsToAppUnits(pointInHyperText.y);

  
  

  
  
  nsCOMPtr<nsIAccessible> accessible;
  PRInt32 offset = 0;

  while (NextChild(accessible)) {
    nsCOMPtr<nsPIAccessNode> accessNode(do_QueryInterface(accessible));
    nsIFrame *primaryFrame = accessNode->GetFrame();
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
    PRInt32 textLength = TextLength(accessible);
    NS_ENSURE_TRUE(textLength >= 0, NS_ERROR_FAILURE);
    offset += textLength;
  }

  return NS_OK; 
}


NS_IMETHODIMP nsHyperTextAccessible::GetLinks(PRInt32 *aLinks)
{
  *aLinks = 0;
  if (!mDOMNode) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIAccessible> accessible;

  while (NextChild(accessible)) {
    if (IsEmbeddedObject(accessible)) {
      ++*aLinks;
    }
  }
  return NS_OK;
}


NS_IMETHODIMP nsHyperTextAccessible::GetLink(PRInt32 aIndex, nsIAccessibleHyperLink **aLink)
{
  *aLink = nsnull;
  if (!mDOMNode) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIAccessible> accessible;

  while (NextChild(accessible)) {
    if (IsEmbeddedObject(accessible) && aIndex-- == 0) {
      CallQueryInterface(accessible, aLink);
      return NS_OK;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP nsHyperTextAccessible::GetLinkIndex(PRInt32 aCharIndex, PRInt32 *aLinkIndex)
{
  *aLinkIndex = -1; 

  PRInt32 characterCount = 0;
  PRInt32 linkIndex = 0;
  if (!mDOMNode) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIAccessible> accessible;

  while (NextChild(accessible) && characterCount <= aCharIndex) {
    PRUint32 role = Role(accessible);
    if (role == nsIAccessibleRole::ROLE_TEXT_LEAF ||
        role == nsIAccessibleRole::ROLE_STATICTEXT) {
      PRInt32 textLength = TextLength(accessible);
      NS_ENSURE_TRUE(textLength >= 0, NS_ERROR_FAILURE);
      characterCount += textLength;
    }
    else {
      if (characterCount ++ == aCharIndex) {
        *aLinkIndex = linkIndex;
        break;
      }
      if (role != nsIAccessibleRole::ROLE_WHITESPACE) {
        ++ linkIndex;
      }
    }
  }
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

NS_IMETHODIMP nsHyperTextAccessible::InsertText(const nsAString &aText, PRInt32 aPosition)
{
  if (NS_SUCCEEDED(SetCaretOffset(aPosition))) {
    nsCOMPtr<nsIEditor> editor;
    GetAssociatedEditor(getter_AddRefs(editor));
    nsCOMPtr<nsIPlaintextEditor> peditor(do_QueryInterface(editor));
    return peditor ? peditor->InsertText(aText) : NS_ERROR_FAILURE;
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsHyperTextAccessible::CopyText(PRInt32 aStartPos, PRInt32 aEndPos)
{
  nsCOMPtr<nsIEditor> editor;
  GetAssociatedEditor(getter_AddRefs(editor));
  if (editor && NS_SUCCEEDED(SetSelectionRange(aStartPos, aEndPos)))
    return editor->Copy();

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsHyperTextAccessible::CutText(PRInt32 aStartPos, PRInt32 aEndPos)
{
  nsCOMPtr<nsIEditor> editor;
  GetAssociatedEditor(getter_AddRefs(editor));
  if (editor && NS_SUCCEEDED(SetSelectionRange(aStartPos, aEndPos)))
    return editor->Cut();

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsHyperTextAccessible::DeleteText(PRInt32 aStartPos, PRInt32 aEndPos)
{
  nsCOMPtr<nsIEditor> editor;
  GetAssociatedEditor(getter_AddRefs(editor));
  if (editor && NS_SUCCEEDED(SetSelectionRange(aStartPos, aEndPos)))
    return editor->DeleteSelection(nsIEditor::eNone);

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsHyperTextAccessible::PasteText(PRInt32 aPosition)
{
  nsCOMPtr<nsIEditor> editor;
  GetAssociatedEditor(getter_AddRefs(editor));
  if (editor && NS_SUCCEEDED(SetCaretOffset(aPosition)))
    return editor->Paste(nsIClipboard::kGlobalClipboard);

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsHyperTextAccessible::GetAssociatedEditor(nsIEditor **aEditor)
{
  NS_ENSURE_ARG_POINTER(aEditor);

  *aEditor = nsnull;
  nsCOMPtr<nsIContent> content = do_QueryInterface(mDOMNode);
  NS_ENSURE_TRUE(content, NS_ERROR_FAILURE);

  if (!content->HasFlag(NODE_IS_EDITABLE)) {
    
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
    nsAccUtils::GetDocShellTreeItemFor(mDOMNode);
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





nsresult nsHyperTextAccessible::SetSelectionRange(PRInt32 aStartPos, PRInt32 aEndPos)
{
  
  nsresult rv = SetSelectionBounds(0, aStartPos, aEndPos);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCOMPtr<nsISelection> domSel;
  nsCOMPtr<nsISelectionController> selCon;
  GetSelections(getter_AddRefs(selCon), getter_AddRefs(domSel));
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
       nsISelectionController::SELECTION_FOCUS_REGION, PR_TRUE);
  }

  return NS_OK;
}

NS_IMETHODIMP nsHyperTextAccessible::SetCaretOffset(PRInt32 aCaretOffset)
{
  return SetSelectionRange(aCaretOffset, aCaretOffset);
}




NS_IMETHODIMP nsHyperTextAccessible::GetCaretOffset(PRInt32 *aCaretOffset)
{
  *aCaretOffset = 0;

  nsCOMPtr<nsISelection> domSel;
  nsresult rv = GetSelections(nsnull, getter_AddRefs(domSel));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> caretNode;
  rv = domSel->GetFocusNode(getter_AddRefs(caretNode));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 caretOffset;
  domSel->GetFocusOffset(&caretOffset);

  return DOMPointToHypertextOffset(caretNode, caretOffset, aCaretOffset);
}

nsresult nsHyperTextAccessible::GetSelections(nsISelectionController **aSelCon,
                                              nsISelection **aDomSel,
                                              nsCOMArray<nsIDOMRange>* aRanges)
{
  if (!mDOMNode) {
    return NS_ERROR_FAILURE;
  }
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
    
    
    
    
    if (aSelCon) {
      editor->GetSelectionController(getter_AddRefs(selCon));
      NS_ENSURE_TRUE(*aSelCon, NS_ERROR_FAILURE);
    }

    editor->GetSelection(getter_AddRefs(domSel));
    NS_ENSURE_TRUE(domSel, NS_ERROR_FAILURE);
    }
  else {
    
    
    nsIFrame *frame = GetFrame();
    NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

    
    frame->GetSelectionController(GetPresContext(),
                                  getter_AddRefs(selCon));
    NS_ENSURE_TRUE(selCon, NS_ERROR_FAILURE);
    selCon->GetSelection(nsISelectionController::SELECTION_NORMAL,
                         getter_AddRefs(domSel));
    NS_ENSURE_TRUE(domSel, NS_ERROR_FAILURE);
  }

  if (aSelCon) {
    NS_ADDREF(*aSelCon = selCon);
  }
  if (aDomSel) {
    NS_ADDREF(*aDomSel = domSel);
  }
  if (aRanges) {
    nsCOMPtr<nsISelection2> selection2(do_QueryInterface(domSel));
    NS_ENSURE_TRUE(selection2, NS_ERROR_FAILURE);

    nsCOMPtr<nsIDOMNodeList> childNodes;
    nsresult rv = mDOMNode->GetChildNodes(getter_AddRefs(childNodes));
    NS_ENSURE_SUCCESS(rv, rv);
    PRUint32 numChildren; 
    rv = childNodes->GetLength(&numChildren);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = selection2->GetRangesForIntervalCOMArray(mDOMNode, 0,
                                                  mDOMNode, numChildren,
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
  nsresult rv = GetSelections(nsnull, nsnull, &ranges);
  NS_ENSURE_SUCCESS(rv, rv);

  *aSelectionCount = ranges.Count();

  return NS_OK;
}




NS_IMETHODIMP nsHyperTextAccessible::GetSelectionBounds(PRInt32 aSelectionNum, PRInt32 *aStartOffset, PRInt32 *aEndOffset)
{
  *aStartOffset = *aEndOffset = 0;

  nsCOMPtr<nsISelection> domSel;
  nsCOMArray<nsIDOMRange> ranges;
  nsresult rv = GetSelections(nsnull, getter_AddRefs(domSel), &ranges);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 rangeCount = ranges.Count();
  if (aSelectionNum < 0 || aSelectionNum >= rangeCount)
    return NS_ERROR_INVALID_ARG;

  nsCOMPtr<nsIDOMRange> range = ranges[aSelectionNum];

  
  nsCOMPtr<nsIDOMNode> startNode;
  range->GetStartContainer(getter_AddRefs(startNode));
  PRInt32 startOffset;
  range->GetStartOffset(&startOffset);

  
  nsCOMPtr<nsIDOMNode> endNode;
  range->GetEndContainer(getter_AddRefs(endNode));
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

  nsCOMPtr<nsIAccessible> startAccessible;
  rv = DOMPointToHypertextOffset(startNode, startOffset, aStartOffset, getter_AddRefs(startAccessible));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!startAccessible) {
    *aStartOffset = 0; 
  }

  return DOMPointToHypertextOffset(endNode, endOffset, aEndOffset, nsnull, PR_TRUE);
}




NS_IMETHODIMP
nsHyperTextAccessible::SetSelectionBounds(PRInt32 aSelectionNum,
                                          PRInt32 aStartOffset,
                                          PRInt32 aEndOffset)
{
  nsCOMPtr<nsISelection> domSel;
  nsresult rv = GetSelections(nsnull, getter_AddRefs(domSel));
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
  nsresult rv = GetSelections(nsnull, getter_AddRefs(domSel));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 rangeCount;
  domSel->GetRangeCount(&rangeCount);

  return SetSelectionBounds(rangeCount, aStartOffset, aEndOffset);
}




NS_IMETHODIMP nsHyperTextAccessible::RemoveSelection(PRInt32 aSelectionNum)
{
  nsCOMPtr<nsISelection> domSel;
  nsresult rv = GetSelections(nsnull, getter_AddRefs(domSel));
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

  return nsAccUtils::ScrollSubstringTo(GetFrame(), startNode, startOffset,
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
  while (parentFrame = parentFrame->GetParent()) {
    nsIScrollableFrame *scrollableFrame = nsnull;
    CallQueryInterface(parentFrame, &scrollableFrame);
    if (scrollableFrame) {
      if (!initialScrolled) {
        
        
        nsIntRect frameRect = parentFrame->GetScreenRectExternal();
        PRInt32 devOffsetX = coords.x - frameRect.x;
        PRInt32 devOffsetY = coords.y - frameRect.y;

        nsPoint offsetPoint(presContext->DevPixelsToAppUnits(devOffsetX),
                            presContext->DevPixelsToAppUnits(devOffsetY));

        nsSize size(parentFrame->GetSize());
        PRInt16 hPercent = offsetPoint.x * 100 / size.width;
        PRInt16 vPercent = offsetPoint.y * 100 / size.height;

        rv = nsAccUtils::ScrollSubstringTo(GetFrame(), startNode, startOffset,
                                           endNode, endOffset,
                                           vPercent, hPercent);
        NS_ENSURE_SUCCESS(rv, rv);

        initialScrolled = PR_TRUE;
      } else {
        
        
        
        
        nsAccUtils::ScrollFrameToPoint(parentFrame, frame, coords);
      }
    }
    frame = parentFrame;
  }

  return NS_OK;
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

    nsCOMPtr<nsIPresShell> shell(GetPresShell());
    NS_ENSURE_STATE(shell);

    nsIFrame *primaryFrame = shell->GetPrimaryFrameFor(content);
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

