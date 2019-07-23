







































#include "nsGkAtoms.h"
#include "nsPopupSetFrame.h"
#include "nsIMenuParent.h"
#include "nsMenuFrame.h"
#include "nsBoxFrame.h"
#include "nsIContent.h"
#include "prtypes.h"
#include "nsIAtom.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsCSSRendering.h"
#include "nsINameSpaceManager.h"
#include "nsMenuPopupFrame.h"
#include "nsMenuBarFrame.h"
#include "nsIView.h"
#include "nsIWidget.h"
#include "nsIDocument.h"
#include "nsIDOMNSDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMXULDocument.h"
#include "nsIDOMElement.h"
#include "nsISupportsArray.h"
#include "nsIDOMText.h"
#include "nsBoxLayoutState.h"
#include "nsIScrollableFrame.h"
#include "nsCSSFrameConstructor.h"
#include "nsGUIEvent.h"
#include "nsIRootBox.h"

#define NS_MENU_POPUP_LIST_INDEX   0

nsPopupFrameList::nsPopupFrameList(nsIContent* aPopupContent, nsPopupFrameList* aNext)
:mNextPopup(aNext), 
 mPopupFrame(nsnull),
 mPopupContent(aPopupContent)
{
}






nsIFrame*
NS_NewPopupSetFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsPopupSetFrame (aPresShell, aContext);
}

NS_IMETHODIMP
nsPopupSetFrame::Init(nsIContent*      aContent,
                      nsIFrame*        aParent,
                      nsIFrame*        aPrevInFlow)
{
  nsresult  rv = nsBoxFrame::Init(aContent, aParent, aPrevInFlow);

  nsIRootBox *rootBox;
  nsresult res = CallQueryInterface(aParent->GetParent(), &rootBox);
  NS_ASSERTION(NS_SUCCEEDED(res), "grandparent should be root box");
  if (NS_SUCCEEDED(res)) {
    rootBox->SetPopupSetFrame(this);
  }

  return rv;
}

NS_IMETHODIMP
nsPopupSetFrame::AppendFrames(nsIAtom*        aListName,
                              nsIFrame*       aFrameList)
{
  if (aListName == nsGkAtoms::popupList) {
    return AddPopupFrameList(aFrameList);
  }
  return nsBoxFrame::AppendFrames(aListName, aFrameList);
}

NS_IMETHODIMP
nsPopupSetFrame::RemoveFrame(nsIAtom*        aListName,
                             nsIFrame*       aOldFrame)
{
  if (aListName == nsGkAtoms::popupList) {
    return RemovePopupFrame(aOldFrame);
  }
  return nsBoxFrame::RemoveFrame(aListName, aOldFrame);
}

NS_IMETHODIMP
nsPopupSetFrame::InsertFrames(nsIAtom*        aListName,
                              nsIFrame*       aPrevFrame,
                              nsIFrame*       aFrameList)
{
  if (aListName == nsGkAtoms::popupList) {
    return AddPopupFrameList(aFrameList);
  }
  return nsBoxFrame::InsertFrames(aListName, aPrevFrame, aFrameList);
}

NS_IMETHODIMP
nsPopupSetFrame::SetInitialChildList(nsIAtom*        aListName,
                                     nsIFrame*       aChildList)
{
  if (aListName == nsGkAtoms::popupList) {
    return AddPopupFrameList(aChildList);
  }
  return nsBoxFrame::SetInitialChildList(aListName, aChildList);
}

void
nsPopupSetFrame::Destroy()
{
  
  while (mPopupList) {
    if (mPopupList->mPopupFrame)
      mPopupList->mPopupFrame->Destroy();

    nsPopupFrameList* temp = mPopupList;
    mPopupList = mPopupList->mNextPopup;
    delete temp;
  }

  nsIRootBox *rootBox;
  nsresult res = CallQueryInterface(mParent->GetParent(), &rootBox);
  NS_ASSERTION(NS_SUCCEEDED(res), "grandparent should be root box");
  if (NS_SUCCEEDED(res)) {
    rootBox->SetPopupSetFrame(nsnull);
  }

  nsBoxFrame::Destroy();
}

NS_IMETHODIMP
nsPopupSetFrame::DoLayout(nsBoxLayoutState& aState)
{
  
  nsresult rv = nsBoxFrame::DoLayout(aState);

  
  nsPopupFrameList* currEntry = mPopupList;
  while (currEntry) {
    nsMenuPopupFrame* popupChild = currEntry->mPopupFrame;
    if (popupChild && popupChild->IsOpen()) {
      
      nsSize prefSize = popupChild->GetPrefSize(aState);
      nsSize minSize = popupChild->GetMinSize(aState);
      nsSize maxSize = popupChild->GetMaxSize(aState);

      BoundsCheck(minSize, prefSize, maxSize);

      popupChild->SetBounds(aState, nsRect(0,0,prefSize.width, prefSize.height));
      popupChild->SetPopupPosition(nsnull);

      
      nsIBox* child = popupChild->GetChildBox();

      nsRect bounds(popupChild->GetRect());

      nsCOMPtr<nsIScrollableFrame> scrollframe = do_QueryInterface(child);
      if (scrollframe &&
          scrollframe->GetScrollbarStyles().mVertical == NS_STYLE_OVERFLOW_AUTO) {
        
        if (bounds.height < prefSize.height) {
          
          popupChild->Layout(aState);

          nsMargin scrollbars = scrollframe->GetActualScrollbarSizes();
          if (bounds.width < prefSize.width + scrollbars.left + scrollbars.right)
          {
            bounds.width += scrollbars.left + scrollbars.right;
            popupChild->SetBounds(aState, bounds);
          }
        }
      }

      
      popupChild->Layout(aState);
      
      
      
      
      if (popupChild->GetRect().width > bounds.width ||
          popupChild->GetRect().height > bounds.height)
        popupChild->SetPopupPosition(nsnull);
      popupChild->AdjustView();
    }

    currEntry = currEntry->mNextPopup;
  }

  return rv;
}

nsresult
nsPopupSetFrame::RemovePopupFrame(nsIFrame* aPopup)
{
  
  
  nsPopupFrameList* currEntry = mPopupList;
  nsPopupFrameList* temp = nsnull;
  while (currEntry) {
    if (currEntry->mPopupFrame == aPopup) {
      
      if (temp)
        temp->mNextPopup = currEntry->mNextPopup;
      else
        mPopupList = currEntry->mNextPopup;
      
      
      currEntry->mPopupFrame->Destroy();

      
      currEntry->mNextPopup = nsnull;
      delete currEntry;

      
      break;
    }

    temp = currEntry;
    currEntry = currEntry->mNextPopup;
  }

  return NS_OK;
}

nsresult
nsPopupSetFrame::AddPopupFrameList(nsIFrame* aPopupFrameList)
{
  for (nsIFrame* kid = aPopupFrameList; kid; kid = kid->GetNextSibling()) {
    nsresult rv = AddPopupFrame(kid);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

nsresult
nsPopupSetFrame::AddPopupFrame(nsIFrame* aPopup)
{
  NS_ASSERTION(aPopup->GetType() == nsGkAtoms::menuPopupFrame,
               "expected a menupopup frame to be added to a popupset");
  if (aPopup->GetType() != nsGkAtoms::menuPopupFrame)
    return NS_ERROR_UNEXPECTED;

  
  
  
  nsIContent* content = aPopup->GetContent();
  nsPopupFrameList* entry = mPopupList;
  while (entry && entry->mPopupContent != content)
    entry = entry->mNextPopup;
  if (!entry) {
    entry = new nsPopupFrameList(content, mPopupList);
    if (!entry)
      return NS_ERROR_OUT_OF_MEMORY;
    mPopupList = entry;
  }
  else {
    NS_ASSERTION(!entry->mPopupFrame, "Leaking a popup frame");
  }

  
  entry->mPopupFrame = static_cast<nsMenuPopupFrame *>(aPopup);
  
  return NS_OK;
}
