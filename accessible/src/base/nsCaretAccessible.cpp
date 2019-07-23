





































#include "nsAccessibilityService.h"
#include "nsCaretAccessible.h"
#include "nsIAccessibleEvent.h"
#include "nsICaret.h"
#include "nsIDOMDocument.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsIFrame.h"
#include "nsIPresShell.h"
#include "nsRootAccessible.h"
#include "nsISelectionController.h"
#include "nsISelectionPrivate.h"
#include "nsServiceManagerUtils.h"
#include "nsIViewManager.h"
#include "nsIWidget.h"

NS_IMPL_ISUPPORTS1(nsCaretAccessible, nsISelectionListener)
  
nsCaretAccessible::nsCaretAccessible( nsRootAccessible *aRootAccessible):
mLastCaretOffset(-1), mRootAccessible(aRootAccessible)
{
}

nsCaretAccessible::~nsCaretAccessible()
{
}

void nsCaretAccessible::Shutdown()
{
  
  
  

  ClearControlSelectionListener(); 
  mLastTextAccessible = nsnull;
  mLastUsedSelection = nsnull;
}

nsresult nsCaretAccessible::ClearControlSelectionListener()
{
  nsCOMPtr<nsISelectionPrivate> selPrivate(do_QueryReferent(mCurrentControlSelection));
  NS_ENSURE_TRUE(selPrivate, NS_ERROR_FAILURE);

  mCurrentControlSelection = nsnull;
  mCurrentControl = nsnull;
  return selPrivate->RemoveSelectionListener(this);
}

nsresult nsCaretAccessible::SetControlSelectionListener(nsIDOMNode *aCurrentNode)
{
  mCurrentControl = aCurrentNode;
  mLastTextAccessible = nsnull;

  
  
  nsCOMPtr<nsIPresShell> presShell = 
    mRootAccessible->GetPresShellFor(aCurrentNode);
  if (!presShell)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDocument> doc = presShell->GetDocument();
  NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

  nsCOMPtr<nsIContent> content(do_QueryInterface(aCurrentNode));
  
  
  if (!content) {
    return NS_OK;
  }

  nsIFrame *frame = presShell->GetPrimaryFrameFor(content);
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  nsPresContext *presContext = presShell->GetPresContext();
  NS_ENSURE_TRUE(presContext, NS_ERROR_FAILURE);

  nsCOMPtr<nsISelectionController> selCon;
  frame->GetSelectionController(presContext, getter_AddRefs(selCon));
  NS_ENSURE_TRUE(selCon, NS_ERROR_FAILURE);
  
  nsCOMPtr<nsISelection> domSel;
  selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(domSel));

  ClearControlSelectionListener();
  nsCOMPtr<nsISelectionPrivate> selPrivate(do_QueryInterface(domSel));
  NS_ENSURE_TRUE(selPrivate, NS_ERROR_FAILURE);

  mCurrentControlSelection = do_GetWeakReference(domSel);
  return selPrivate->AddSelectionListener(this);
}

nsresult nsCaretAccessible::AddDocSelectionListener(nsIDOMDocument *aDoc)
{
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(aDoc);
  NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);
  nsCOMPtr<nsISelectionController> selCon = do_QueryInterface(doc->GetPrimaryShell());
  NS_ENSURE_TRUE(selCon, NS_ERROR_FAILURE);

  nsCOMPtr<nsISelection> domSel;
  selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(domSel));
  nsCOMPtr<nsISelectionPrivate> selPrivate = do_QueryInterface(domSel);
  NS_ENSURE_TRUE(selPrivate, NS_ERROR_FAILURE);

  return selPrivate->AddSelectionListener(this);
}

nsresult nsCaretAccessible::RemoveDocSelectionListener(nsIDOMDocument *aDoc)
{
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(aDoc);
  NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

  nsCOMPtr<nsISelectionController> selCon = do_QueryInterface(doc->GetPrimaryShell());
  NS_ENSURE_TRUE(selCon, NS_ERROR_FAILURE);

  nsCOMPtr<nsISelection> domSel;
  selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(domSel));
  nsCOMPtr<nsISelectionPrivate> selPrivate = do_QueryInterface(domSel);
  NS_ENSURE_TRUE(selPrivate, NS_ERROR_FAILURE);

  return selPrivate->RemoveSelectionListener(this);
}

NS_IMETHODIMP nsCaretAccessible::NotifySelectionChanged(nsIDOMDocument *aDoc, nsISelection *aSel, PRInt16 aReason)
{
  mLastUsedSelection = do_GetWeakReference(aSel);

  nsCOMPtr<nsIDocument> doc = do_QueryInterface(aDoc);
  nsIPresShell *presShell = doc->GetPrimaryShell();
  NS_ENSURE_TRUE(presShell, NS_OK);

  
  nsCOMPtr<nsIAccessible> accessible;
  nsIAccessibilityService *accService = mRootAccessible->GetAccService();
  NS_ENSURE_TRUE(accService, NS_ERROR_FAILURE);
  
  nsCOMPtr<nsIDOMNode> focusNode;
  aSel->GetFocusNode(getter_AddRefs(focusNode));
  if (!focusNode) {
    mLastTextAccessible = nsnull;
    return NS_OK; 
  }
  nsCOMPtr<nsIDOMNode> nodeWithCaret = focusNode;

  nsCOMPtr<nsIAccessibleText> textAcc;
  while (focusNode) {
    
    nsCOMPtr<nsIDOMNode> relevantNode;
    if (NS_SUCCEEDED(accService->GetRelevantContentNodeFor(focusNode, getter_AddRefs(relevantNode))) && relevantNode) {
      focusNode  = relevantNode;
    }

    nsCOMPtr<nsIContent> content = do_QueryInterface(focusNode);
    if (!content || !content->IsNodeOfType(nsINode::eTEXT)) {
      accService->GetAccessibleInShell(focusNode, presShell,  getter_AddRefs(accessible));
      textAcc = do_QueryInterface(accessible);
      if (textAcc) {
        break;
      }
    }
    nsCOMPtr<nsIDOMNode> parentNode;
    focusNode->GetParentNode(getter_AddRefs(parentNode));
    focusNode.swap(parentNode);
  }
  NS_ASSERTION(textAcc, "No nsIAccessibleText for caret move event!"); 
  NS_ENSURE_TRUE(textAcc, NS_ERROR_FAILURE);

  PRInt32 caretOffset;
  nsresult rv = textAcc->GetCaretOffset(&caretOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  if (textAcc == mLastTextAccessible && caretOffset == mLastCaretOffset) {
    PRInt32 selectionCount;
    textAcc->GetSelectionCount(&selectionCount);   
    if (!selectionCount) {
      return NS_OK;  
    }
  }
  mLastCaretOffset = caretOffset;
  mLastTextAccessible = textAcc;

  nsCOMPtr<nsIAccessibleCaretMoveEvent> event =
    new nsAccCaretMoveEvent(focusNode);
  NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

  return mRootAccessible->FireDelayedAccessibleEvent(event, nsDocAccessible::eRemoveDupes);
}

nsRect
nsCaretAccessible::GetCaretRect(nsIWidget **aOutWidget)
{
  nsRect caretRect;
  NS_ENSURE_TRUE(aOutWidget, caretRect);
  *aOutWidget = nsnull;

  if (!mLastTextAccessible) {
    return caretRect;    
  }

  nsCOMPtr<nsIAccessNode> lastAccessNode(do_QueryInterface(mLastTextAccessible));
  NS_ENSURE_TRUE(lastAccessNode, caretRect);

  nsCOMPtr<nsIDOMNode> lastNodeWithCaret;
  lastAccessNode->GetDOMNode(getter_AddRefs(lastNodeWithCaret));
  NS_ENSURE_TRUE(lastNodeWithCaret, caretRect);

  nsCOMPtr<nsIPresShell> presShell = mRootAccessible->GetPresShellFor(lastNodeWithCaret);
  NS_ENSURE_TRUE(presShell, caretRect);

  nsICaret *caret;
  presShell->GetCaret(&caret);
  NS_ENSURE_TRUE(caret, caretRect);

  PRBool isCollapsed;
  nsIView *view;
  nsCOMPtr<nsISelection> caretSelection(do_QueryReferent(mLastUsedSelection));
  caret->GetCaretCoordinates(nsICaret::eRenderingViewCoordinates, caretSelection,
                             &caretRect, &isCollapsed, &view);
  if (!view || caretRect.IsEmpty()) {
    return nsRect(); 
  }

  PRBool isVisible;
  caret->GetCaretVisible(&isVisible);
  if (!isVisible) {
    return nsRect();  
  }
  nsPoint offsetFromWidget;
  *aOutWidget = view->GetNearestWidget(&offsetFromWidget);
  NS_ENSURE_TRUE(*aOutWidget, nsRect());

  nsPresContext *presContext = presShell->GetPresContext();
  NS_ENSURE_TRUE(presContext, nsRect());

  caretRect.x = presContext->AppUnitsToDevPixels(caretRect.x + offsetFromWidget.x);
  caretRect.y = presContext->AppUnitsToDevPixels(caretRect.y + offsetFromWidget.y);
  caretRect.width = presContext->AppUnitsToDevPixels(caretRect.width);
  caretRect.height = presContext->AppUnitsToDevPixels(caretRect.height);

  (*aOutWidget)->WidgetToScreen(caretRect, caretRect);

  
  
  
  PRInt32 charX, charY, charWidth, charHeight;
  if (NS_SUCCEEDED(mLastTextAccessible->GetCharacterExtents(mLastCaretOffset, &charX, &charY,
                                                            &charWidth, &charHeight,
                                                            nsIAccessibleCoordinateType::COORDTYPE_SCREEN_RELATIVE))) {
    caretRect.height -= charY - caretRect.y;
    caretRect.y = charY;
  }

  return caretRect;
}

