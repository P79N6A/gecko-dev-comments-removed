












































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

NS_IMETHODIMP
nsBoxLayout::GetFlex(nsIBox* aBox, nsBoxLayoutState& aState, nscoord& aFlex)
{
  aFlex = aBox->GetFlex(aState);
  return NS_OK;
}


NS_IMETHODIMP
nsBoxLayout::IsCollapsed(nsIBox* aBox, nsBoxLayoutState& aState, PRBool& aCollapsed)
{
  aCollapsed = aBox->IsCollapsed(aState);
  return NS_OK;
}

NS_IMETHODIMP
nsBoxLayout::GetPrefSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nsSize& aSize)
{
  aSize.width = 0;
  aSize.height = 0;
  AddBorderAndPadding(aBox, aSize);

  return NS_OK;
}

NS_IMETHODIMP
nsBoxLayout::GetMinSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nsSize& aSize)
{
  aSize.width = 0;
  aSize.height = 0;
  AddBorderAndPadding(aBox, aSize);
  return NS_OK;
}

NS_IMETHODIMP
nsBoxLayout::GetMaxSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nsSize& aSize)
{
  aSize.width = NS_INTRINSICSIZE;
  aSize.height = NS_INTRINSICSIZE;
  AddBorderAndPadding(aBox, aSize);
  return NS_OK;
}


NS_IMETHODIMP
nsBoxLayout::GetAscent(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nscoord& aAscent)
{
  aAscent = 0;
  return NS_OK;
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

NS_IMETHODIMP
nsBoxLayout::ChildrenInserted(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aPrevBox, nsIBox* aChildList)
{
  return NS_OK;
}

NS_IMETHODIMP
nsBoxLayout::ChildrenAppended(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aChildList)
{
  return NS_OK;
}

NS_IMETHODIMP
nsBoxLayout::ChildrenRemoved(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aChildList)
{
  return NS_OK;
}

NS_IMETHODIMP
nsBoxLayout::ChildrenSet(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aChildList)
{
  return NS_OK;
}

NS_IMETHODIMP
nsBoxLayout::IntrinsicWidthsDirty(nsIBox* aBox, nsBoxLayoutState& aState)
{
  return NS_OK;
}


NS_IMPL_ADDREF(nsBoxLayout)
NS_IMPL_RELEASE(nsBoxLayout)




NS_INTERFACE_MAP_BEGIN(nsBoxLayout)
  NS_INTERFACE_MAP_ENTRY(nsIBoxLayout)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END
