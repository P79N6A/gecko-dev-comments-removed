




#include "nsPopupSetFrame.h"
#include "nsGkAtoms.h"
#include "nsCOMPtr.h"
#include "nsIContent.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsBoxLayoutState.h"
#include "nsIScrollableFrame.h"
#include "nsIRootBox.h"
#include "nsMenuPopupFrame.h"

nsIFrame*
NS_NewPopupSetFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsPopupSetFrame (aPresShell, aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsPopupSetFrame)

void
nsPopupSetFrame::Init(nsIContent*      aContent,
                      nsIFrame*        aParent,
                      nsIFrame*        aPrevInFlow)
{
  nsBoxFrame::Init(aContent, aParent, aPrevInFlow);

  
  
  nsIRootBox *rootBox = nsIRootBox::GetRootBox(PresContext()->GetPresShell());
  if (rootBox) {
    rootBox->SetPopupSetFrame(this);
  }
}

nsIAtom*
nsPopupSetFrame::GetType() const
{
  return nsGkAtoms::popupSetFrame;
}

NS_IMETHODIMP
nsPopupSetFrame::AppendFrames(ChildListID     aListID,
                              nsFrameList&    aFrameList)
{
  if (aListID == kPopupList) {
    AddPopupFrameList(aFrameList);
    return NS_OK;
  }
  return nsBoxFrame::AppendFrames(aListID, aFrameList);
}

NS_IMETHODIMP
nsPopupSetFrame::RemoveFrame(ChildListID     aListID,
                             nsIFrame*       aOldFrame)
{
  if (aListID == kPopupList) {
    RemovePopupFrame(aOldFrame);
    return NS_OK;
  }
  return nsBoxFrame::RemoveFrame(aListID, aOldFrame);
}

NS_IMETHODIMP
nsPopupSetFrame::InsertFrames(ChildListID     aListID,
                              nsIFrame*       aPrevFrame,
                              nsFrameList&    aFrameList)
{
  if (aListID == kPopupList) {
    AddPopupFrameList(aFrameList);
    return NS_OK;
  }
  return nsBoxFrame::InsertFrames(aListID, aPrevFrame, aFrameList);
}

NS_IMETHODIMP
nsPopupSetFrame::SetInitialChildList(ChildListID     aListID,
                                     nsFrameList&    aChildList)
{
  if (aListID == kPopupList) {
    
    
    
    
    
    AddPopupFrameList(aChildList);
    return NS_OK;
  }
  return nsBoxFrame::SetInitialChildList(aListID, aChildList);
}

void
nsPopupSetFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  mPopupList.DestroyFramesFrom(aDestructRoot);

  
  
  nsIRootBox *rootBox = nsIRootBox::GetRootBox(PresContext()->GetPresShell());
  if (rootBox) {
    rootBox->SetPopupSetFrame(nullptr);
  }

  nsBoxFrame::DestroyFrom(aDestructRoot);
}

NS_IMETHODIMP
nsPopupSetFrame::DoLayout(nsBoxLayoutState& aState)
{
  
  nsresult rv = nsBoxFrame::DoLayout(aState);

  
  for (nsFrameList::Enumerator e(mPopupList); !e.AtEnd(); e.Next()) {
    nsMenuPopupFrame* popupChild = static_cast<nsMenuPopupFrame*>(e.get());
    popupChild->LayoutPopup(aState, nullptr, false);
  }

  return rv;
}

void
nsPopupSetFrame::RemovePopupFrame(nsIFrame* aPopup)
{
  NS_PRECONDITION((aPopup->GetStateBits() & NS_FRAME_OUT_OF_FLOW) &&
                  aPopup->GetType() == nsGkAtoms::menuPopupFrame,
                  "removing wrong type of frame in popupset's ::popupList");

  mPopupList.DestroyFrame(aPopup);
}

void
nsPopupSetFrame::AddPopupFrameList(nsFrameList& aPopupFrameList)
{
#ifdef DEBUG
  for (nsFrameList::Enumerator e(aPopupFrameList); !e.AtEnd(); e.Next()) {
    NS_ASSERTION((e.get()->GetStateBits() & NS_FRAME_OUT_OF_FLOW) &&
                 e.get()->GetType() == nsGkAtoms::menuPopupFrame,
                 "adding wrong type of frame in popupset's ::popupList");
  }
#endif
  mPopupList.InsertFrames(nullptr, nullptr, aPopupFrameList);
}

#ifdef DEBUG
void
nsPopupSetFrame::List(FILE* out, int32_t aIndent, uint32_t aFlags) const
{
  ListGeneric(out, aIndent, aFlags);

  
  bool outputOneList = false;
  ChildListIterator lists(this);
  for (; !lists.IsDone(); lists.Next()) {
    if (outputOneList) {
      IndentBy(out, aIndent);
    }
    outputOneList = true;
    fprintf(out, "%s<\n", mozilla::layout::ChildListName(lists.CurrentID()));
    nsFrameList::Enumerator childFrames(lists.CurrentList());
    for (; !childFrames.AtEnd(); childFrames.Next()) {
      nsIFrame* kid = childFrames.get();
      
      NS_ASSERTION(kid->GetParent() == this, "bad parent frame pointer");

      
      kid->List(out, aIndent + 1, aFlags);
    }
    IndentBy(out, aIndent);
    fputs(">\n", out);
  }

  
  

  if (!mPopupList.IsEmpty()) {
    fputs("<\n", out);
    ++aIndent;
    IndentBy(out, aIndent);
    fputs(mozilla::layout::ChildListName(kPopupList), out);
    fputs(" for ", out);
    ListTag(out);
    fputs(" <\n", out);
    ++aIndent;
    for (nsFrameList::Enumerator e(mPopupList); !e.AtEnd(); e.Next()) {
      e.get()->List(out, aIndent, aFlags);
    }
    --aIndent;
    IndentBy(out, aIndent);
    fputs(">\n", out);
    --aIndent;
    IndentBy(out, aIndent);
    fputs(">\n", out);
    outputOneList = true;
  }

  if (!outputOneList) {
    fputs("<>\n", out);
  }
}
#endif
