





































#include "nsAccessibilityService.h"
#include "nsCaretAccessible.h"
#include "nsIAccessibleEvent.h"
#include "nsCaret.h"
#include "nsIDOMDocument.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsIFrame.h"
#include "nsIPresShell.h"
#include "nsRootAccessible.h"
#include "nsISelectionPrivate.h"
#include "nsISelection2.h"
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
  mRootAccessible = nsnull;
}

nsresult nsCaretAccessible::ClearControlSelectionListener()
{
  nsCOMPtr<nsISelectionController> controller =
    GetSelectionControllerForNode(mCurrentControl);

  mCurrentControl = nsnull;

  if (!controller)
    return NS_OK;

  
  nsCOMPtr<nsISelection> normalSel;
  controller->GetSelection(nsISelectionController::SELECTION_NORMAL,
                           getter_AddRefs(normalSel));
  nsCOMPtr<nsISelectionPrivate> selPrivate(do_QueryInterface(normalSel));
  NS_ENSURE_TRUE(selPrivate, NS_ERROR_FAILURE);

  nsresult rv = selPrivate->RemoveSelectionListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCOMPtr<nsISelection> spellcheckSel;
  controller->GetSelection(nsISelectionController::SELECTION_SPELLCHECK,
                           getter_AddRefs(spellcheckSel));
  selPrivate = do_QueryInterface(spellcheckSel);
  NS_ENSURE_TRUE(selPrivate, NS_ERROR_FAILURE);

  return selPrivate->RemoveSelectionListener(this);
}

nsresult nsCaretAccessible::SetControlSelectionListener(nsIDOMNode *aCurrentNode)
{
  NS_ENSURE_TRUE(mRootAccessible, NS_ERROR_FAILURE);

  ClearControlSelectionListener();

  mCurrentControl = aCurrentNode;
  mLastTextAccessible = nsnull;

  
  
  
  nsCOMPtr<nsISelectionController> controller =
    GetSelectionControllerForNode(mCurrentControl);
  NS_ENSURE_TRUE(controller, NS_ERROR_FAILURE);

  
  nsCOMPtr<nsISelection> normalSel;
  controller->GetSelection(nsISelectionController::SELECTION_NORMAL,
                           getter_AddRefs(normalSel));
  nsCOMPtr<nsISelectionPrivate> selPrivate(do_QueryInterface(normalSel));
  NS_ENSURE_TRUE(selPrivate, NS_ERROR_FAILURE);

  nsresult rv = selPrivate->AddSelectionListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsISelection> spellcheckSel;
  controller->GetSelection(nsISelectionController::SELECTION_SPELLCHECK,
                           getter_AddRefs(spellcheckSel));
  selPrivate = do_QueryInterface(spellcheckSel);
  NS_ENSURE_TRUE(selPrivate, NS_ERROR_FAILURE);
  
  return selPrivate->AddSelectionListener(this);
}

nsresult
nsCaretAccessible::AddDocSelectionListener(nsIPresShell *aShell)
{
  NS_ENSURE_TRUE(mRootAccessible, NS_ERROR_FAILURE);

  nsCOMPtr<nsISelectionController> selCon = do_QueryInterface(aShell);
  NS_ENSURE_TRUE(selCon, NS_ERROR_FAILURE);

  nsCOMPtr<nsISelection> domSel;
  selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(domSel));
  nsCOMPtr<nsISelectionPrivate> selPrivate = do_QueryInterface(domSel);
  NS_ENSURE_TRUE(selPrivate, NS_ERROR_FAILURE);

  nsresult rv = selPrivate->AddSelectionListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISelection> spellcheckSel;
  selCon->GetSelection(nsISelectionController::SELECTION_SPELLCHECK,
                       getter_AddRefs(spellcheckSel));
  selPrivate = do_QueryInterface(spellcheckSel);
  NS_ENSURE_TRUE(selPrivate, NS_ERROR_FAILURE);
  
  return selPrivate->AddSelectionListener(this);
}

nsresult
nsCaretAccessible::RemoveDocSelectionListener(nsIPresShell *aShell)
{
  nsCOMPtr<nsISelectionController> selCon = do_QueryInterface(aShell);
  NS_ENSURE_TRUE(selCon, NS_ERROR_FAILURE);

  nsCOMPtr<nsISelection> domSel;
  selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(domSel));
  nsCOMPtr<nsISelectionPrivate> selPrivate = do_QueryInterface(domSel);
  NS_ENSURE_TRUE(selPrivate, NS_ERROR_FAILURE);

  selPrivate->RemoveSelectionListener(this);

  nsCOMPtr<nsISelection> spellcheckSel;
  selCon->GetSelection(nsISelectionController::SELECTION_SPELLCHECK,
                       getter_AddRefs(spellcheckSel));
  selPrivate = do_QueryInterface(spellcheckSel);
  NS_ENSURE_TRUE(selPrivate, NS_ERROR_FAILURE);

  return selPrivate->RemoveSelectionListener(this);
}

NS_IMETHODIMP
nsCaretAccessible::NotifySelectionChanged(nsIDOMDocument *aDoc,
                                          nsISelection *aSel,
                                          PRInt16 aReason)
{
  nsCOMPtr<nsISelection2> sel2(do_QueryInterface(aSel));

  PRInt16 type = 0;
  sel2->GetType(&type);

  if (type == nsISelectionController::SELECTION_NORMAL)
    return NormalSelectionChanged(aDoc, aSel);

  if (type == nsISelectionController::SELECTION_SPELLCHECK)
    return SpellcheckSelectionChanged(aDoc, aSel);

  return NS_OK;
}

nsresult
nsCaretAccessible::NormalSelectionChanged(nsIDOMDocument *aDoc,
                                          nsISelection *aSel)
{
  NS_ENSURE_TRUE(mRootAccessible, NS_ERROR_FAILURE);

  mLastUsedSelection = do_GetWeakReference(aSel);

  nsCOMPtr<nsIDocument> doc = do_QueryInterface(aDoc);
  NS_ENSURE_TRUE(doc, NS_OK);
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

  nsCOMPtr<nsIAccessibleDocument> docAccessible =
    nsAccessNode::GetDocAccessibleFor(focusNode);
  nsCOMPtr<nsIAccessible> accessibleForDoc =
    do_QueryInterface(docAccessible);
  if (!accessibleForDoc) {
    return NS_OK;
  }
  PRUint32 docState;
  accessibleForDoc->GetFinalState(&docState, nsnull);
  if (docState & nsIAccessibleStates::STATE_BUSY) {
    return NS_OK;  
  }

  
  nsCOMPtr<nsIContent> focusContainer(do_QueryInterface(focusNode));
  if (focusContainer && focusContainer->IsNodeOfType(nsINode::eELEMENT)) {
    PRInt32 focusOffset = 0;
    aSel->GetFocusOffset(&focusOffset);

    nsCOMPtr<nsIContent> focusContent = focusContainer->GetChildAt(focusOffset);
    focusNode = do_QueryInterface(focusContent);
  }

  
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

  return mRootAccessible->FireDelayedAccessibleEvent(event);
}

nsresult
nsCaretAccessible::SpellcheckSelectionChanged(nsIDOMDocument *aDoc,
                                              nsISelection *aSel)
{
  
  
  
  
  
  nsCOMPtr<nsIDOMNode> targetNode;
  aSel->GetFocusNode(getter_AddRefs(targetNode));
  if (!targetNode)
    return NS_OK;

  
  nsCOMPtr<nsIContent> focusContainer(do_QueryInterface(targetNode));
  if (focusContainer && focusContainer->IsNodeOfType(nsINode::eELEMENT)) {
    PRInt32 focusOffset = 0;
    aSel->GetFocusOffset(&focusOffset);
    
    nsCOMPtr<nsIContent> focusContent = focusContainer->GetChildAt(focusOffset);
    targetNode = do_QueryInterface(focusContent);
  }

  nsCOMPtr<nsIAccessibleDocument> docAccessible =
    nsAccessNode::GetDocAccessibleFor(targetNode);
  NS_ENSURE_STATE(docAccessible);

  nsCOMPtr<nsIAccessible> containerAccessible;
  nsresult rv =
    docAccessible->GetAccessibleInParentChain(targetNode, PR_TRUE,
                                              getter_AddRefs(containerAccessible));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIAccessibleEvent> event =
    new nsAccEvent(nsIAccessibleEvent::EVENT_TEXT_ATTRIBUTE_CHANGED,
                   containerAccessible, nsnull);
  NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

  return mRootAccessible->FireAccessibleEvent(event);
}

nsRect
nsCaretAccessible::GetCaretRect(nsIWidget **aOutWidget)
{
  nsRect caretRect;
  NS_ENSURE_TRUE(aOutWidget, caretRect);
  *aOutWidget = nsnull;
  NS_ENSURE_TRUE(mRootAccessible, caretRect);

  if (!mLastTextAccessible) {
    return caretRect;    
  }

  nsCOMPtr<nsIAccessNode> lastAccessNode(do_QueryInterface(mLastTextAccessible));
  NS_ENSURE_TRUE(lastAccessNode, caretRect);

  nsCOMPtr<nsIDOMNode> lastNodeWithCaret;
  lastAccessNode->GetDOMNode(getter_AddRefs(lastNodeWithCaret));
  NS_ENSURE_TRUE(lastNodeWithCaret, caretRect);

  nsCOMPtr<nsIPresShell> presShell =
    nsCoreUtils::GetPresShellFor(lastNodeWithCaret);
  NS_ENSURE_TRUE(presShell, caretRect);

  nsRefPtr<nsCaret> caret;
  presShell->GetCaret(getter_AddRefs(caret));
  NS_ENSURE_TRUE(caret, caretRect);

  PRBool isCollapsed;
  nsIView *view;
  nsCOMPtr<nsISelection> caretSelection(do_QueryReferent(mLastUsedSelection));
  NS_ENSURE_TRUE(caretSelection, caretRect);
  
  caret->GetCaretCoordinates(nsCaret::eRenderingViewCoordinates, caretSelection,
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

already_AddRefed<nsISelectionController>
nsCaretAccessible::GetSelectionControllerForNode(nsIDOMNode *aNode)
{
  if (!aNode)
    return nsnull;

  nsCOMPtr<nsIPresShell> presShell = nsCoreUtils::GetPresShellFor(aNode);
  if (!presShell)
    return nsnull;

  nsCOMPtr<nsIDocument> doc = presShell->GetDocument();
  if (!doc)
    return nsnull;

  
  nsCOMPtr<nsIContent> content(do_QueryInterface(aNode));
  if (!content)
    return nsnull;

  nsIFrame *frame = presShell->GetPrimaryFrameFor(content);
  if (!frame)
    return nsnull;

  nsPresContext *presContext = presShell->GetPresContext();
  if (!presContext)
    return nsnull;

  nsISelectionController *controller = nsnull;
  frame->GetSelectionController(presContext, &controller);
  return controller;
}

