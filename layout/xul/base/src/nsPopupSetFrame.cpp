








































#include "nsPopupSetFrame.h"
#include "nsGkAtoms.h"
#include "nsCOMPtr.h"
#include "nsIContent.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsBoxLayoutState.h"
#include "nsIScrollableFrame.h"
#include "nsIRootBox.h"

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

NS_IMPL_FRAMEARENA_HELPERS(nsPopupSetFrame)

NS_IMETHODIMP
nsPopupSetFrame::Init(nsIContent*      aContent,
                      nsIFrame*        aParent,
                      nsIFrame*        aPrevInFlow)
{
  nsresult  rv = nsBoxFrame::Init(aContent, aParent, aPrevInFlow);

  
  
  nsIRootBox *rootBox = nsIRootBox::GetRootBox(PresContext()->GetPresShell());
  if (rootBox) {
    rootBox->SetPopupSetFrame(this);
  }

  return rv;
}

nsIAtom*
nsPopupSetFrame::GetType() const
{
  return nsGkAtoms::popupSetFrame;
}

NS_IMETHODIMP
nsPopupSetFrame::AppendFrames(nsIAtom*        aListName,
                              nsFrameList&    aFrameList)
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
                              nsFrameList&    aFrameList)
{
  if (aListName == nsGkAtoms::popupList) {
    return AddPopupFrameList(aFrameList);
  }
  return nsBoxFrame::InsertFrames(aListName, aPrevFrame, aFrameList);
}

NS_IMETHODIMP
nsPopupSetFrame::SetInitialChildList(nsIAtom*        aListName,
                                     nsFrameList&    aChildList)
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
    if (mPopupList->mPopupFrame) {
      mPopupList->mPopupFrame->Destroy();
    }
    nsPopupFrameList* temp = mPopupList;
    mPopupList = mPopupList->mNextPopup;
    delete temp;
  }

  
  
  nsIRootBox *rootBox = nsIRootBox::GetRootBox(PresContext()->GetPresShell());
  if (rootBox) {
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

      prefSize = BoundsCheck(minSize, prefSize, maxSize);

      popupChild->SetPreferredBounds(aState, nsRect(0,0,prefSize.width, prefSize.height));
      popupChild->SetPopupPosition(nsnull);

      
      nsIBox* child = popupChild->GetChildBox();

      nsRect bounds(popupChild->GetRect());

      nsIScrollableFrame *scrollframe = do_QueryFrame(child);
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
          popupChild->GetRect().height > bounds.height) {
        
        
        popupChild->SetPreferredSize(popupChild->GetSize());
        popupChild->SetPopupPosition(nsnull);
      }
      popupChild->AdjustView();
    }

    currEntry = currEntry->mNextPopup;
  }

  return rv;
}

nsresult
nsPopupSetFrame::RemovePopupFrame(nsIFrame* aPopup)
{
  
  
#ifdef DEBUG
  PRBool found = PR_FALSE;
#endif
  nsPopupFrameList* currEntry = mPopupList;
  nsPopupFrameList* temp = nsnull;
  while (currEntry) {
    if (currEntry->mPopupFrame == aPopup) {
      
      if (temp)
        temp->mNextPopup = currEntry->mNextPopup;
      else
        mPopupList = currEntry->mNextPopup;
      
      NS_ASSERTION((aPopup->GetStateBits() & NS_FRAME_OUT_OF_FLOW) &&
                   aPopup->GetType() == nsGkAtoms::menuPopupFrame,
                   "found wrong type of frame in popupset's ::popupList");
      
      currEntry->mPopupFrame->Destroy();

      
      currEntry->mNextPopup = nsnull;
      delete currEntry;
#ifdef DEBUG
      found = PR_TRUE;
#endif

      
      break;
    }

    temp = currEntry;
    currEntry = currEntry->mNextPopup;
  }

  NS_ASSERTION(found, "frame to remove is not in our ::popupList");
  return NS_OK;
}

nsresult
nsPopupSetFrame::AddPopupFrameList(nsFrameList& aPopupFrameList)
{
  while (!aPopupFrameList.IsEmpty()) {
    nsIFrame* f = aPopupFrameList.FirstChild();
    
    
    
    aPopupFrameList.RemoveFrame(f);
    nsresult rv = AddPopupFrame(f);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

nsresult
nsPopupSetFrame::AddPopupFrame(nsIFrame* aPopup)
{
  NS_ASSERTION((aPopup->GetStateBits() & NS_FRAME_OUT_OF_FLOW) &&
               aPopup->GetType() == nsGkAtoms::menuPopupFrame,
               "adding wrong type of frame in popupset's ::popupList");

  
  
  
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

#ifdef DEBUG
NS_IMETHODIMP
nsPopupSetFrame::List(FILE* out, PRInt32 aIndent) const
{
  IndentBy(out, aIndent);
  ListTag(out);
#ifdef DEBUG_waterson
  fprintf(out, " [parent=%p]", static_cast<void*>(mParent));
#endif
  if (HasView()) {
    fprintf(out, " [view=%p]", static_cast<void*>(GetView()));
  }
  if (GetNextSibling()) {
    fprintf(out, " next=%p", static_cast<void*>(GetNextSibling()));
  }
  if (nsnull != GetPrevContinuation()) {
    fprintf(out, " prev-continuation=%p", static_cast<void*>(GetPrevContinuation()));
  }
  if (nsnull != GetNextContinuation()) {
    fprintf(out, " next-continuation=%p", static_cast<void*>(GetNextContinuation()));
  }
  fprintf(out, " {%d,%d,%d,%d}", mRect.x, mRect.y, mRect.width, mRect.height);
  if (0 != mState) {
    fprintf(out, " [state=%08x]", mState);
  }
  fprintf(out, " [content=%p]", static_cast<void*>(mContent));
  nsPopupSetFrame* f = const_cast<nsPopupSetFrame*>(this);
  if (f->HasOverflowRect()) {
    nsRect overflowArea = f->GetOverflowRect();
    fprintf(out, " [overflow=%d,%d,%d,%d]", overflowArea.x, overflowArea.y,
            overflowArea.width, overflowArea.height);
  }
  fprintf(out, " [sc=%p]", static_cast<void*>(mStyleContext));
  nsIAtom* pseudoTag = mStyleContext->GetPseudo();
  if (pseudoTag) {
    nsAutoString atomString;
    pseudoTag->ToString(atomString);
    fprintf(out, " pst=%s",
            NS_LossyConvertUTF16toASCII(atomString).get());
  }

  
  nsIAtom* listName = nsnull;
  PRInt32 listIndex = 0;
  PRBool outputOneList = PR_FALSE;
  do {
    nsIFrame* kid = GetFirstChild(listName);
    if (nsnull != kid) {
      if (outputOneList) {
        IndentBy(out, aIndent);
      }
      outputOneList = PR_TRUE;
      nsAutoString tmp;
      if (nsnull != listName) {
        listName->ToString(tmp);
        fputs(NS_LossyConvertUTF16toASCII(tmp).get(), out);
      }
      fputs("<\n", out);
      while (nsnull != kid) {
        
        NS_ASSERTION(kid->GetParent() == (nsIFrame*)this, "bad parent frame pointer");

        
        kid->List(out, aIndent + 1);
        kid = kid->GetNextSibling();
      }
      IndentBy(out, aIndent);
      fputs(">\n", out);
    }
    listName = GetAdditionalChildListName(listIndex++);
  } while(nsnull != listName);

  
  

  if (mPopupList) {
    fputs("<\n", out);
    ++aIndent;
    IndentBy(out, aIndent);
    nsAutoString tmp;
    nsGkAtoms::popupList->ToString(tmp);
    fputs(NS_LossyConvertUTF16toASCII(tmp).get(), out);
    fputs(" for ", out);
    ListTag(out);
    fputs(" <\n", out);
    ++aIndent;
    for (nsPopupFrameList* l = mPopupList; l; l = l->mNextPopup) {
      l->mPopupFrame->List(out, aIndent);
    }
    --aIndent;
    IndentBy(out, aIndent);
    fputs(">\n", out);
    --aIndent;
    IndentBy(out, aIndent);
    fputs(">\n", out);
    outputOneList = PR_TRUE;
  }

  if (!outputOneList) {
    fputs("<>\n", out);
  }

  return NS_OK;
}
#endif
