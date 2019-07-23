






































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
#include "nsIDOMDocumentView.h"
#include "nsIDOMRange.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDOMXULDocument.h"
#include "nsIFontMetrics.h"
#include "nsIFrame.h"
#include "nsIPlaintextEditor.h"
#include "nsIServiceManager.h"
#include "nsTextFragment.h"

static NS_DEFINE_IID(kRangeCID, NS_RANGE_CID);





NS_IMPL_ADDREF_INHERITED(nsHyperTextAccessible, nsAccessible)
NS_IMPL_RELEASE_INHERITED(nsHyperTextAccessible, nsAccessible)

nsresult nsHyperTextAccessible::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  *aInstancePtr = nsnull;

  if (aIID.Equals(NS_GET_IID(nsHyperTextAccessible))) {
    *aInstancePtr = static_cast<nsHyperTextAccessible*>(this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  nsCOMPtr<nsIDOMXULDocument> xulDoc(do_QueryInterface(mDOMNode));
  if (mDOMNode && !xulDoc) {
    
    
    
    
    if (aIID.Equals(NS_GET_IID(nsIAccessibleText))) {
      
      PRInt32 numChildren;
      GetChildCount(&numChildren);
      if (numChildren > 0) {
        *aInstancePtr = static_cast<nsIAccessibleText*>(this);
        NS_ADDREF_THIS();
        return NS_OK;
      }
      return NS_ERROR_NO_INTERFACE;
    }

    if (aIID.Equals(NS_GET_IID(nsIAccessibleHyperText))) {
      if (IsHyperText()) {
        
        *aInstancePtr = static_cast<nsIAccessibleHyperText*>(this);
        NS_ADDREF_THIS();
        return NS_OK;
      }
      return NS_ERROR_NO_INTERFACE;
    }

    if (aIID.Equals(NS_GET_IID(nsIAccessibleEditableText))) {
      
      PRUint32 state, extState;
      GetState(&state, &extState);
      if (extState & nsIAccessibleStates::EXT_STATE_EDITABLE) {
        *aInstancePtr = static_cast<nsIAccessibleEditableText*>(this);
        NS_ADDREF_THIS();
        return NS_OK;
      }
      return NS_ERROR_NO_INTERFACE;
    }
  }

  return nsAccessible::QueryInterface(aIID, aInstancePtr);
}

nsHyperTextAccessible::nsHyperTextAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsAccessibleWrap(aNode, aShell)
{
}

PRBool nsHyperTextAccessible::IsHyperText()
{
  nsCOMPtr<nsIAccessible> accessible;
  while (NextChild(accessible)) {
    if (IsEmbeddedObject(accessible)) {
      return PR_TRUE;
    }
  }

  return PR_FALSE;
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

  if (!aExtraState)
    return NS_OK;

  nsCOMPtr<nsIEditor> editor = GetEditor();
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
    nsCOMPtr<nsIEditor> editor = GetEditor();
    if (!editor) {
      nsAccessible::CacheChildren();
      return;
    }
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


nsIntRect nsHyperTextAccessible::GetBoundsForString(nsIFrame *aFrame, PRInt32 aStartOffset, PRInt32 aLength)
{
  nsIntRect screenRect;
  nsIFrame *frame;
  PRInt32 startOffsetInFrame;
  nsresult rv = aFrame->GetChildFrameContainingOffset(aStartOffset, PR_FALSE,
                                                      &startOffsetInFrame, &frame);
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

  while (frame && aLength > 0) {
    
    
    
    
    nsIntRect frameScreenRect = frame->GetScreenRectExternal();

    
    PRInt32 startFrameTextOffset, endFrameTextOffset;
    frame->GetOffsets(startFrameTextOffset, endFrameTextOffset);
    PRInt32 frameTotalTextLength = endFrameTextOffset - startFrameTextOffset;
    PRInt32 frameSubStringLength = PR_MIN(frameTotalTextLength - startOffsetInFrame, aLength);

    
    nsPoint frameTextStartPoint;
    rv = frame->GetPointFromOffset(context, rc, aStartOffset, &frameTextStartPoint);
    NS_ENSURE_SUCCESS(rv, nsRect());   
    frameScreenRect.x += context->AppUnitsToDevPixels(frameTextStartPoint.x);

    
    nsPoint frameTextEndPoint;
    rv = frame->GetPointFromOffset(context, rc, aStartOffset + frameSubStringLength, &frameTextEndPoint);
    NS_ENSURE_SUCCESS(rv, nsRect());   
    frameScreenRect.width = context->AppUnitsToDevPixels(frameTextEndPoint.x - frameTextStartPoint.x);

    screenRect.UnionRect(frameScreenRect, screenRect);

    
    aStartOffset += frameSubStringLength;
    startOffsetInFrame = 0;
    aLength -= frameSubStringLength;
    frame = frame->GetNextContinuation();
  }

  return screenRect;
}




nsIFrame* nsHyperTextAccessible::GetPosAndText(PRInt32& aStartOffset, PRInt32& aEndOffset, nsAString *aText,
                                               nsIFrame **aEndFrame, nsIntRect *aBoundsRect)
{
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

  nsIntRect unionRect;
  nsCOMPtr<nsIAccessible> accessible;

  
  
  while (NextChild(accessible)) {
    nsCOMPtr<nsPIAccessNode> accessNode(do_QueryInterface(accessible));
    nsIFrame *frame = accessNode->GetFrame();
    if (!frame) {
      continue;
    }
    if (IsText(accessible)) {
      nsCOMPtr<nsPIAccessible> pAcc(do_QueryInterface(accessible));
      nsAutoString newText;
      pAcc->GetContentText(newText);

      PRInt32 substringEndOffset = newText.Length();
      if (startOffset < substringEndOffset) {
        
        
        
        

        if (startOffset > 0 || endOffset < substringEndOffset) {
          
          
          PRInt32 outStartLineUnused;
          frame->GetChildFrameContainingOffset(startOffset, PR_TRUE, &outStartLineUnused, &frame);
          if (endOffset < substringEndOffset) {
            
            substringEndOffset = endOffset;
          }
          if (aText) {
            newText = Substring(newText, startOffset,
                                substringEndOffset - startOffset);
          }
          if (aEndFrame) {
            *aEndFrame = frame; 
          }
          aEndOffset = endOffset;
        }
        if (aText) {
          if (!frame->GetStyleText()->WhiteSpaceIsSignificant()) {
            
            
            newText.ReplaceChar("\r\n\t", ' ');
          }
          *aText += newText;
        }
        if (aBoundsRect) {
          aBoundsRect->UnionRect(*aBoundsRect, GetBoundsForString(frame, startOffset,
                                                                  substringEndOffset - startOffset));
        }
        if (!startFrame) {
          startFrame = frame;
          aStartOffset = startOffset;
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
            *aText += (frame->GetType() == nsAccessibilityAtoms::brFrame) ?
                      kForcedNewLineChar : kEmbeddedObjectChar;
          }
          if (aBoundsRect) {
            aBoundsRect->UnionRect(*aBoundsRect, frame->GetScreenRectExternal());
          }
        }
        if (!startFrame) {
          startFrame = frame;
          aStartOffset = 0;
        }
      }
      -- endOffset;
    }
    if (endOffset <= 0 && startFrame) {
      break; 
    }
  }

  if (aEndFrame && !*aEndFrame) {
    *aEndFrame = startFrame;
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
    *aCharacterCount += TextLength(accessible);
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
  NS_ENSURE_SUCCESS(rv, rv);
  *aCharacter = text.First();
  return NS_OK;
}

nsresult nsHyperTextAccessible::DOMPointToOffset(nsIDOMNode* aNode, PRInt32 aNodeOffset, PRInt32* aResult,
                                                 nsIAccessible **aFinalAccessible)
{
  
  
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = 0;
  NS_ENSURE_ARG_POINTER(aNode);
  NS_ENSURE_TRUE(aNodeOffset >= 0, NS_ERROR_INVALID_ARG);
  if (aFinalAccessible) {
    *aFinalAccessible = nsnull;
  }

  PRInt32 addTextOffset = 0;
  nsCOMPtr<nsIDOMNode> findNode;

  unsigned short nodeType;
  aNode->GetNodeType(&nodeType);
  if (nodeType == nsIDOMNode::TEXT_NODE) {
    
    
    addTextOffset = aNodeOffset;
    
    findNode = aNode;
  }
  else {
    
    nsCOMPtr<nsIContent> parentContent(do_QueryInterface(aNode));
    
    NS_ENSURE_TRUE(parentContent, NS_ERROR_FAILURE);
    
    
    
    
     
    findNode = do_QueryInterface(parentContent->GetChildAt(aNodeOffset));
    if (!findNode && !aNodeOffset) {
      NS_ASSERTION(!SameCOMIdentity(parentContent, mDOMNode), "Cannot find child for DOMPointToOffset search");
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
    
    
    
    
    
    
    
    
    addTextOffset = addTextOffset > 0;
    descendantAccessible = parentAccessible;
  }  

  
  
  
  
  nsCOMPtr<nsIAccessible> accessible;
  while (NextChild(accessible) && accessible != childAccessible) {
    *aResult += TextLength(accessible);
  }
  if (accessible) {
    *aResult += addTextOffset;
    NS_ASSERTION(accessible == childAccessible, "These should be equal whenever we exit loop and accessible != nsnull");
    if (aFinalAccessible && (NextChild(accessible) || addTextOffset < TextLength(childAccessible))) {  
      
      NS_ADDREF(*aFinalAccessible = childAccessible);
    }
  }
  return NS_OK;
}

PRInt32 nsHyperTextAccessible::GetRelativeOffset(nsIPresShell *aPresShell, nsIFrame *aFromFrame, PRInt32 aFromOffset,
                                                 nsSelectionAmount aAmount, nsDirection aDirection, PRBool aNeedsStart)
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
  pos.SetData(aAmount, aDirection, aFromOffset, 0, kIsJumpLinesOk,
              kIsScrollViewAStop, kIsKeyboardSelect, kIsVisualBidi,
              wordMovementType);
  nsresult rv = aFromFrame->PeekOffset(&pos);
  if (NS_FAILED(rv)) {
    if (aDirection == eDirPrevious) {
      
      
      
      
      pos.mResultContent = aFromFrame->GetContent();
      PRInt32 endOffsetUnused;
      aFromFrame->GetOffsets(pos.mContentOffset, endOffsetUnused);
    }
    else {
      return rv;
    }
  }

  
  PRInt32 hyperTextOffset;
  nsCOMPtr<nsIDOMNode> resultNode = do_QueryInterface(pos.mResultContent);
  NS_ENSURE_TRUE(resultNode, -1);

  nsCOMPtr<nsIAccessible> finalAccessible;
  rv = DOMPointToOffset(resultNode, pos.mContentOffset, &hyperTextOffset, getter_AddRefs(finalAccessible));
  
  
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

  PRInt32 startOffset = aOffset;
  PRInt32 endOffset = aOffset;

  if (aBoundaryType == BOUNDARY_LINE_END) {
    
    ++ startOffset;
    ++ endOffset;
  }
  
  nsIFrame *startFrame = GetPosAndText(startOffset, endOffset);
  if (!startFrame) {
    PRInt32 textLength;
    GetCharacterCount(&textLength);
    return (aOffset < 0 || aOffset > textLength) ? NS_ERROR_FAILURE : NS_OK;
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
    finalStartOffset = GetRelativeOffset(presShell, startFrame,  startOffset,
                                         amount, eDirPrevious, needsStart);
    NS_ENSURE_TRUE(finalStartOffset >= 0, NS_ERROR_FAILURE);
  }

  if (aType == eGetBefore) {
    endOffset = aOffset;
  }
  else {
    
    
    
    startOffset = endOffset = finalStartOffset;
    nsIFrame *endFrame = GetPosAndText(startOffset, endOffset);
    if (!endFrame) {
      return NS_ERROR_FAILURE;
    }
    finalEndOffset = GetRelativeOffset(presShell, endFrame, endOffset, amount,
                                       eDirNext, needsStart);
    NS_ENSURE_TRUE(endOffset >= 0, NS_ERROR_FAILURE);
    if (finalEndOffset == aOffset) {
      
      
      
      ++ finalEndOffset;
    }
  }

  
  
  
  if (aType == eGetAt && amount == eSelectWord && aOffset == endOffset) { 
    return GetTextHelper(eGetAfter, aBoundaryType, aOffset, aStartOffset, aEndOffset, aText);
  }

  *aStartOffset = finalStartOffset;
  *aEndOffset = finalEndOffset;

  NS_ASSERTION((finalStartOffset < aOffset && finalEndOffset >= aOffset) || aType != eGetBefore, "Incorrect results for GetTextHelper");
  NS_ASSERTION((finalStartOffset <= aOffset && finalEndOffset > aOffset) || aType == eGetBefore, "Incorrect results for GetTextHelper");

  return GetPosAndText(finalStartOffset, finalEndOffset, &aText) ? NS_OK : NS_ERROR_FAILURE;
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

  if (aCoordType == nsIAccessibleCoordinateType::COORDTYPE_WINDOW_RELATIVE) {
    
    nsCOMPtr<nsIPresShell> shell = GetPresShell();
    NS_ENSURE_TRUE(shell, NS_ERROR_FAILURE);
    nsCOMPtr<nsIDocument> doc = shell->GetDocument();
    nsCOMPtr<nsIDOMDocumentView> docView(do_QueryInterface(doc));
    NS_ENSURE_TRUE(docView, NS_ERROR_FAILURE);

    nsCOMPtr<nsIDOMAbstractView> abstractView;
    docView->GetDefaultView(getter_AddRefs(abstractView));
    NS_ENSURE_TRUE(abstractView, NS_ERROR_FAILURE);

    nsCOMPtr<nsIDOMWindowInternal> windowInter(do_QueryInterface(abstractView));
    NS_ENSURE_TRUE(windowInter, NS_ERROR_FAILURE);

    PRInt32 screenX, screenY;
    if (NS_FAILED(windowInter->GetScreenX(&screenX)) ||
        NS_FAILED(windowInter->GetScreenY(&screenY))) {
      return NS_ERROR_FAILURE;
    }
    *aX -= screenX;
    *aY -= screenY;
  }
  

  return NS_OK;
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

  if (aCoordType == nsIAccessibleCoordinateType::COORDTYPE_WINDOW_RELATIVE) {
    nsCOMPtr<nsIDocument> doc = shell->GetDocument();
    nsCOMPtr<nsIDOMDocumentView> docView(do_QueryInterface(doc));
    NS_ENSURE_TRUE(docView, NS_ERROR_FAILURE);

    nsCOMPtr<nsIDOMAbstractView> abstractView;
    docView->GetDefaultView(getter_AddRefs(abstractView));
    NS_ENSURE_TRUE(abstractView, NS_ERROR_FAILURE);

    nsCOMPtr<nsIDOMWindowInternal> windowInter(do_QueryInterface(abstractView));
    NS_ENSURE_TRUE(windowInter, NS_ERROR_FAILURE);

    PRInt32 windowX, windowY;
    if (NS_FAILED(windowInter->GetScreenX(&windowX)) ||
        NS_FAILED(windowInter->GetScreenY(&windowY))) {
      return NS_ERROR_FAILURE;
    }
    aX += windowX;
    aY += windowY;
  }
  
  
  if (!frameScreenRect.Contains(aX, aY)) {
    return NS_OK;   
  }
  nsPoint pointInHyperText(aX - frameScreenRect.x, aY - frameScreenRect.y);
  nsPresContext *context = GetPresContext();
  NS_ENSURE_TRUE(context, NS_ERROR_FAILURE);
  pointInHyperText.x = context->DevPixelsToAppUnits(pointInHyperText.x);
  pointInHyperText.y = context->DevPixelsToAppUnits(pointInHyperText.y);

  
  

  
  
  nsCOMPtr<nsIAccessible> accessible;
  PRInt32 offset = 0;

  while (NextChild(accessible)) {
    nsCOMPtr<nsPIAccessNode> accessNode(do_QueryInterface(accessible));
    nsIFrame *frame = accessNode->GetFrame();
    NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);
    while (frame) {
      nsIContent *content = frame->GetContent();
      NS_ENSURE_TRUE(content, NS_ERROR_FAILURE);
      nsPoint pointInFrame = pointInHyperText - frame->GetOffsetToExternal(hyperFrame);
      nsSize frameSize = frame->GetSize();
      if (pointInFrame.x < frameSize.width && pointInFrame.y < frameSize.height) {
        
        if (IsText(accessible)) {
          nsIFrame::ContentOffsets contentOffsets = frame->GetContentOffsetsFromPointExternal(pointInFrame, PR_TRUE);
          if (contentOffsets.IsNull() || contentOffsets.content != content) {
            return NS_OK; 
          }
          offset += contentOffsets.offset;
        }
        *aOffset = offset;
        return NS_OK;
      }
      frame = frame->GetNextContinuation();
    }
    offset += TextLength(accessible);
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
      characterCount += TextLength(accessible);
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
    nsCOMPtr<nsIEditor> editor = GetEditor();
    nsCOMPtr<nsIPlaintextEditor> peditor(do_QueryInterface(editor));
    return peditor ? peditor->InsertText(aText) : NS_ERROR_FAILURE;
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsHyperTextAccessible::CopyText(PRInt32 aStartPos, PRInt32 aEndPos)
{
  nsCOMPtr<nsIEditor> editor = GetEditor();
  if (editor && NS_SUCCEEDED(SetSelectionRange(aStartPos, aEndPos)))
    return editor->Copy();

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsHyperTextAccessible::CutText(PRInt32 aStartPos, PRInt32 aEndPos)
{
  nsCOMPtr<nsIEditor> editor = GetEditor();
  if (editor && NS_SUCCEEDED(SetSelectionRange(aStartPos, aEndPos)))
    return editor->Cut();

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsHyperTextAccessible::DeleteText(PRInt32 aStartPos, PRInt32 aEndPos)
{
  nsCOMPtr<nsIEditor> editor = GetEditor();
  if (editor && NS_SUCCEEDED(SetSelectionRange(aStartPos, aEndPos)))
    return editor->DeleteSelection(nsIEditor::eNone);

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsHyperTextAccessible::PasteText(PRInt32 aPosition)
{
  nsCOMPtr<nsIEditor> editor = GetEditor();
  if (editor && NS_SUCCEEDED(SetCaretOffset(aPosition)))
    return editor->Paste(nsIClipboard::kGlobalClipboard);

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsHyperTextAccessible::GetAssociatedEditor(nsIEditor **aEditor)
{
  NS_ENSURE_ARG_POINTER(aEditor);

  nsCOMPtr<nsIEditor> editor(GetEditor());
  NS_IF_ADDREF(*aEditor = editor);

  return NS_OK;
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

  return DOMPointToOffset(caretNode, caretOffset, aCaretOffset);
}

nsresult nsHyperTextAccessible::GetSelections(nsISelectionController **aSelCon, nsISelection **aDomSel)
{
  if (aSelCon) {
    *aSelCon = nsnull;
  }
  if (aDomSel) {
    *aDomSel = nsnull;
  }
  
  nsCOMPtr<nsIEditor> editor = GetEditor();
  if (editor) {
    if (aSelCon) {
      editor->GetSelectionController(aSelCon);
      NS_ENSURE_TRUE(*aSelCon, NS_ERROR_FAILURE);
    }

    if (aDomSel) {
      editor->GetSelection(aDomSel);
      NS_ENSURE_TRUE(*aDomSel, NS_ERROR_FAILURE);
    }

    return NS_OK;
  }

  nsIFrame *frame = GetFrame();
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  
  nsCOMPtr<nsISelectionController> selCon;
  frame->GetSelectionController(GetPresContext(),
                                getter_AddRefs(selCon));
  NS_ENSURE_TRUE(selCon, NS_ERROR_FAILURE);
  if (aSelCon) {
    NS_ADDREF(*aSelCon = selCon);
  }

  if (aDomSel) {
    nsCOMPtr<nsISelection> domSel;
    selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(domSel));
    NS_ENSURE_TRUE(domSel, NS_ERROR_FAILURE);
    NS_ADDREF(*aDomSel = domSel);
  }

  return NS_OK;
}




NS_IMETHODIMP nsHyperTextAccessible::GetSelectionCount(PRInt32 *aSelectionCount)
{
  nsCOMPtr<nsISelection> domSel;
  nsresult rv = GetSelections(nsnull, getter_AddRefs(domSel));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isSelectionCollapsed;
  rv = domSel->GetIsCollapsed(&isSelectionCollapsed);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isSelectionCollapsed) {
    *aSelectionCount = 0;
    return NS_OK;
  }
  return domSel->GetRangeCount(aSelectionCount);
}




NS_IMETHODIMP nsHyperTextAccessible::GetSelectionBounds(PRInt32 aSelectionNum, PRInt32 *aStartOffset, PRInt32 *aEndOffset)
{
  *aStartOffset = *aEndOffset = 0;

  nsCOMPtr<nsISelection> domSel;
  nsresult rv = GetSelections(nsnull, getter_AddRefs(domSel));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 rangeCount;
  domSel->GetRangeCount(&rangeCount);
  if (aSelectionNum < 0 || aSelectionNum >= rangeCount)
    return NS_ERROR_INVALID_ARG;

  nsCOMPtr<nsIDOMRange> range;
  rv = domSel->GetRangeAt(aSelectionNum, getter_AddRefs(range));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> startNode;
  range->GetStartContainer(getter_AddRefs(startNode));
  PRInt32 startOffset;
  range->GetStartOffset(&startOffset);
  rv = DOMPointToOffset(startNode, startOffset, aStartOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> endNode;
  range->GetEndContainer(getter_AddRefs(endNode));
  PRInt32 endOffset;
  range->GetEndOffset(&endOffset);
  if (startNode == endNode && startOffset == endOffset) {
    
    *aEndOffset = *aStartOffset;
    return NS_OK;
  }
  return DOMPointToOffset(endNode, endOffset, aEndOffset);
}




NS_IMETHODIMP nsHyperTextAccessible::SetSelectionBounds(PRInt32 aSelectionNum, PRInt32 aStartOffset, PRInt32 aEndOffset)
{
  nsCOMPtr<nsISelection> domSel;
  nsresult rv = GetSelections(nsnull, getter_AddRefs(domSel));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 isOnlyCaret = (aStartOffset == aEndOffset); 

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

  nsIFrame *endFrame;
  nsIFrame *startFrame = GetPosAndText(aStartOffset, aEndOffset, nsnull, &endFrame);
  NS_ENSURE_TRUE(startFrame, NS_ERROR_FAILURE);

  nsIContent *startParentContent = startFrame->GetContent();
  if (startFrame->GetType() != nsAccessibilityAtoms::textFrame) {
    nsIContent *newParent = startParentContent->GetParent();
    aStartOffset = newParent->IndexOf(startParentContent);
    startParentContent = newParent;
  }
  nsCOMPtr<nsIDOMNode> startParentNode(do_QueryInterface(startParentContent));
  NS_ENSURE_TRUE(startParentNode, NS_ERROR_FAILURE);
  rv = range->SetStart(startParentNode, aStartOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isOnlyCaret) { 
    rv = range->Collapse(PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    nsIContent *endParentContent = endFrame->GetContent();
    if (endFrame->GetType() != nsAccessibilityAtoms::textFrame) {
      nsIContent *newParent = endParentContent->GetParent();
      aEndOffset = newParent->IndexOf(endParentContent);
      endParentContent = newParent;
    }
    nsCOMPtr<nsIDOMNode> endParentNode(do_QueryInterface(endParentContent));
    NS_ENSURE_TRUE(endParentNode, NS_ERROR_FAILURE);
    rv = range->SetEnd(endParentNode, aEndOffset);
    NS_ENSURE_SUCCESS(rv, rv);
  }

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

