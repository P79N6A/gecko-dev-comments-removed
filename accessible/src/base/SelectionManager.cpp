




#include "mozilla/a11y/SelectionManager.h"

#include "DocAccessible-inl.h"
#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "nsCoreUtils.h"
#include "nsEventShell.h"

#include "nsIAccessibleTypes.h"
#include "nsIDOMDocument.h"
#include "nsIPresShell.h"
#include "nsISelectionPrivate.h"
#include "mozilla/Selection.h"
#include "mozilla/dom/Element.h"

using namespace mozilla;
using namespace mozilla::a11y;

void
SelectionManager::ClearControlSelectionListener()
{
  if (!mCurrCtrlFrame)
    return;

  const nsFrameSelection* frameSel = mCurrCtrlFrame->GetConstFrameSelection();
  NS_ASSERTION(frameSel, "No frame selection for the element!");

  mCurrCtrlFrame = nullptr;
  if (!frameSel)
    return;

  
  Selection* normalSel =
    frameSel->GetSelection(nsISelectionController::SELECTION_NORMAL);
  normalSel->RemoveSelectionListener(this);

  
  
  Selection* spellSel =
    frameSel->GetSelection(nsISelectionController::SELECTION_SPELLCHECK);
  spellSel->RemoveSelectionListener(this);
}

void
SelectionManager::SetControlSelectionListener(dom::Element* aFocusedElm)
{
  
  
  
  ClearControlSelectionListener();

  mCurrCtrlFrame = aFocusedElm->GetPrimaryFrame();
  if (!mCurrCtrlFrame)
    return;

  const nsFrameSelection* frameSel = mCurrCtrlFrame->GetConstFrameSelection();
  NS_ASSERTION(frameSel, "No frame selection for focused element!");
  if (!frameSel)
    return;

  
  Selection* normalSel =
    frameSel->GetSelection(nsISelectionController::SELECTION_NORMAL);
  normalSel->AddSelectionListener(this);

  
  Selection* spellSel =
    frameSel->GetSelection(nsISelectionController::SELECTION_SPELLCHECK);
  spellSel->AddSelectionListener(this);
}

void
SelectionManager::AddDocSelectionListener(nsIPresShell* aPresShell)
{
  const nsFrameSelection* frameSel = aPresShell->ConstFrameSelection();

  
  Selection* normalSel =
    frameSel->GetSelection(nsISelectionController::SELECTION_NORMAL);
  normalSel->AddSelectionListener(this);

  
  Selection* spellSel =
    frameSel->GetSelection(nsISelectionController::SELECTION_SPELLCHECK);
  spellSel->AddSelectionListener(this);
}

void
SelectionManager::RemoveDocSelectionListener(nsIPresShell* aPresShell)
{
  const nsFrameSelection* frameSel = aPresShell->ConstFrameSelection();

  
  Selection* normalSel =
    frameSel->GetSelection(nsISelectionController::SELECTION_NORMAL);
  normalSel->RemoveSelectionListener(this);

  
  
  Selection* spellSel =
    frameSel->GetSelection(nsISelectionController::SELECTION_SPELLCHECK);
  spellSel->RemoveSelectionListener(this);
}

void
SelectionManager::ProcessTextSelChangeEvent(AccEvent* aEvent)
{
  AccTextSelChangeEvent* event = downcast_accEvent(aEvent);
  Selection* sel = static_cast<Selection*>(event->mSel.get());

  
  if (sel->GetRangeCount() != 1 || !sel->IsCollapsed())
    nsEventShell::FireEvent(aEvent);

  
  nsINode* caretCntrNode =
    nsCoreUtils::GetDOMNodeFromDOMPoint(sel->GetFocusNode(),
                                        sel->GetFocusOffset());
  if (!caretCntrNode)
    return;

  HyperTextAccessible* caretCntr = nsAccUtils::GetTextContainer(caretCntrNode);
  NS_ASSERTION(caretCntr,
               "No text container for focus while there's one for common ancestor?!");
  if (!caretCntr)
    return;

  int32_t caretOffset = -1;
  if (NS_SUCCEEDED(caretCntr->GetCaretOffset(&caretOffset)) && caretOffset != -1) {
    nsRefPtr<AccCaretMoveEvent> caretMoveEvent =
      new AccCaretMoveEvent(caretCntr, caretOffset, aEvent->FromUserInput());
    nsEventShell::FireEvent(caretMoveEvent);
  }
}

NS_IMETHODIMP
SelectionManager::NotifySelectionChanged(nsIDOMDocument* aDOMDocument,
                                         nsISelection* aSelection,
                                         int16_t aReason)
{
  NS_ENSURE_ARG(aDOMDocument);

  nsCOMPtr<nsIDocument> documentNode(do_QueryInterface(aDOMDocument));
  DocAccessible* document = GetAccService()->GetDocAccessible(documentNode);

#ifdef A11Y_LOG
  if (logging::IsEnabled(logging::eSelection))
    logging::SelChange(aSelection, document);
#endif

  
  if (document && document->IsContentLoaded()) {
    
    
    
    document->HandleNotification<SelectionManager, nsISelection>
      (this, &SelectionManager::ProcessSelectionChanged, aSelection);
  }

  return NS_OK;
}

void
SelectionManager::ProcessSelectionChanged(nsISelection* aSelection)
{
  Selection* selection = static_cast<Selection*>(aSelection);
  const nsRange* range = selection->GetAnchorFocusRange();
  nsINode* cntrNode = nullptr;
  if (range)
    cntrNode = range->GetCommonAncestor();
  if (!cntrNode) {
    cntrNode = selection->GetFrameSelection()->GetAncestorLimiter();
    if (!cntrNode) {
      cntrNode = selection->GetPresShell()->GetDocument();
      NS_ASSERTION(selection->GetPresShell()->ConstFrameSelection() == selection->GetFrameSelection(),
                   "Wrong selection container was used!");
    }
  }

  HyperTextAccessible* text = nsAccUtils::GetTextContainer(cntrNode);
  if (!text) {
    NS_NOTREACHED("We must reach document accessible implementing text interface!");
    return;
  }

  if (selection->GetType() == nsISelectionController::SELECTION_NORMAL) {
    nsRefPtr<AccEvent> event = new AccTextSelChangeEvent(text, aSelection);
    text->Document()->FireDelayedEvent(event);

  } else if (selection->GetType() == nsISelectionController::SELECTION_SPELLCHECK) {
    
    
    text->Document()->FireDelayedEvent(nsIAccessibleEvent::EVENT_TEXT_ATTRIBUTE_CHANGED,
                                       text);
  }
}
