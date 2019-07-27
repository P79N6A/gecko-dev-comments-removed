




#include "mozilla/a11y/SelectionManager.h"

#include "DocAccessible-inl.h"
#include "HyperTextAccessible.h"
#include "HyperTextAccessible-inl.h"
#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "nsCoreUtils.h"
#include "nsEventShell.h"
#include "nsFrameSelection.h"

#include "nsIAccessibleTypes.h"
#include "nsIDOMDocument.h"
#include "nsIPresShell.h"
#include "mozilla/dom/Selection.h"
#include "mozilla/dom/Element.h"

using namespace mozilla;
using namespace mozilla::a11y;
using mozilla::dom::Selection;

struct mozilla::a11y::SelData MOZ_FINAL
{
  SelData(Selection* aSel, int32_t aReason) :
    mSel(aSel), mReason(aReason) {}

  nsRefPtr<Selection> mSel;
  int16_t mReason;

  NS_INLINE_DECL_REFCOUNTING(SelData)

private:
  
  ~SelData() {}
};

SelectionManager::SelectionManager() :
  mCaretOffset(-1), mAccWithCaret(nullptr)
{

}

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
  if (!event->IsCaretMoveOnly())
    nsEventShell::FireEvent(aEvent);

  
  nsINode* caretCntrNode =
    nsCoreUtils::GetDOMNodeFromDOMPoint(event->mSel->GetFocusNode(),
                                        event->mSel->FocusOffset());
  if (!caretCntrNode)
    return;

  HyperTextAccessible* caretCntr = nsAccUtils::GetTextContainer(caretCntrNode);
  NS_ASSERTION(caretCntr,
               "No text container for focus while there's one for common ancestor?!");
  if (!caretCntr)
    return;

  Selection* selection = caretCntr->DOMSelection();
  mCaretOffset = caretCntr->DOMPointToOffset(selection->GetFocusNode(),
                                             selection->FocusOffset());
  mAccWithCaret = caretCntr;
  if (mCaretOffset != -1) {
    nsRefPtr<AccCaretMoveEvent> caretMoveEvent =
      new AccCaretMoveEvent(caretCntr, mCaretOffset, aEvent->FromUserInput());
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
    logging::SelChange(aSelection, document, aReason);
#endif

  if (document) {
    
    
    
    nsRefPtr<SelData> selData =
      new SelData(static_cast<Selection*>(aSelection), aReason);
    document->HandleNotification<SelectionManager, SelData>
      (this, &SelectionManager::ProcessSelectionChanged, selData);
  }

  return NS_OK;
}

void
SelectionManager::ProcessSelectionChanged(SelData* aSelData)
{
  Selection* selection = aSelData->mSel;
  if (!selection->GetPresShell())
    return;

  const nsRange* range = selection->GetAnchorFocusRange();
  nsINode* cntrNode = nullptr;
  if (range)
    cntrNode = range->GetCommonAncestor();

  if (!cntrNode) {
    cntrNode = selection->GetFrameSelection()->GetAncestorLimiter();
    if (!cntrNode) {
      cntrNode = selection->GetPresShell()->GetDocument();
      NS_ASSERTION(aSelData->mSel->GetPresShell()->ConstFrameSelection() == selection->GetFrameSelection(),
                   "Wrong selection container was used!");
    }
  }

  HyperTextAccessible* text = nsAccUtils::GetTextContainer(cntrNode);
  if (!text) {
    NS_NOTREACHED("We must reach document accessible implementing text interface!");
    return;
  }

  if (selection->GetType() == nsISelectionController::SELECTION_NORMAL) {
    nsRefPtr<AccEvent> event =
      new AccTextSelChangeEvent(text, selection, aSelData->mReason);
    text->Document()->FireDelayedEvent(event);

  } else if (selection->GetType() == nsISelectionController::SELECTION_SPELLCHECK) {
    
    
    text->Document()->FireDelayedEvent(nsIAccessibleEvent::EVENT_TEXT_ATTRIBUTE_CHANGED,
                                       text);
  }
}
