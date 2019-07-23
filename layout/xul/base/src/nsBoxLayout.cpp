












































#include "nsBox.h"
#include "nsPresContext.h"
#include "nsCOMPtr.h"
#include "nsIContent.h"
#include "nsIPresShell.h"
#include "nsHTMLContainerFrame.h"
#include "nsIFrame.h"
#include "nsBoxLayout.h"

nsBoxLayout::nsBoxLayout()
{
}

void
nsBoxLayout::GetParentLayout(nsIBox* aBox, nsIBoxLayout** aParent)
{
  nsIBox* parent = aBox->GetParentBox();
  if (parent)
  {
    parent->GetLayoutManager(aParent);
    return;
  }

  *aParent = nsnull;
}

void
nsBoxLayout::AddBorderAndPadding(nsIBox* aBox, nsSize& aSize)
{
  nsBox::AddBorderAndPadding(aBox, aSize);
}

void
nsBoxLayout::AddMargin(nsIBox* aBox, nsSize& aSize)
{
  nsBox::AddMargin(aBox, aSize);
}

void
nsBoxLayout::AddMargin(nsSize& aSize, const nsMargin& aMargin)
{
  nsBox::AddMargin(aSize, aMargin);
}

nsSize
nsBoxLayout::GetPrefSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState)
{
  nsSize pref (0, 0);
  AddBorderAndPadding(aBox, pref);

  return pref;
}

nsSize
nsBoxLayout::GetMinSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState)
{
  nsSize minSize (0,0);
  AddBorderAndPadding(aBox, minSize);
  return minSize;
}

nsSize
nsBoxLayout::GetMaxSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState)
{
  
  
  return nsSize (NS_INTRINSICSIZE,NS_INTRINSICSIZE);
}


nscoord
nsBoxLayout::GetAscent(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState)
{
  return 0;
}

NS_IMETHODIMP
nsBoxLayout::Layout(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState)
{
  return NS_OK;
}

void
nsBoxLayout::AddLargestSize(nsSize& aSize, const nsSize& aSize2)
{
  if (aSize2.width > aSize.width)
     aSize.width = aSize2.width;

  if (aSize2.height > aSize.height)
     aSize.height = aSize2.height;
}

void
nsBoxLayout::AddSmallestSize(nsSize& aSize, const nsSize& aSize2)
{
  if (aSize2.width < aSize.width)
     aSize.width = aSize2.width;

  if (aSize2.height < aSize.height)
     aSize.height = aSize2.height;
}

void
nsBoxLayout::ChildrenInserted(nsIBox* aBox, nsBoxLayoutState& aState,
                              nsIBox* aPrevBox,
                              const nsFrameList::Slice& aNewChildren)
{
}

void
nsBoxLayout::ChildrenAppended(nsIBox* aBox, nsBoxLayoutState& aState,
                              const nsFrameList::Slice& aNewChildren)
{
}

void
nsBoxLayout::ChildrenRemoved(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aChildList)
{
}

void
nsBoxLayout::ChildrenSet(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aChildList)
{
}

void
nsBoxLayout::IntrinsicWidthsDirty(nsIBox* aBox, nsBoxLayoutState& aState)
{
}


NS_IMPL_ADDREF(nsBoxLayout)
NS_IMPL_RELEASE(nsBoxLayout)




NS_INTERFACE_MAP_BEGIN(nsBoxLayout)
  NS_INTERFACE_MAP_ENTRY(nsIBoxLayout)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END
