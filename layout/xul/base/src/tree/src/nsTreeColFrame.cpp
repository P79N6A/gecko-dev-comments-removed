





































#include "nsCOMPtr.h"
#include "nsTreeColFrame.h"
#include "nsGkAtoms.h"
#include "nsIContent.h"
#include "nsStyleContext.h"
#include "nsINameSpaceManager.h" 
#include "nsIDOMNSDocument.h"
#include "nsIDocument.h"
#include "nsIBoxObject.h"
#include "nsIDOMElement.h"
#include "nsITreeBoxObject.h"
#include "nsITreeColumns.h"
#include "nsIDOMXULTreeElement.h"
#include "nsDisplayList.h"






nsIFrame*
NS_NewTreeColFrame(nsIPresShell* aPresShell, nsStyleContext* aContext,
                   PRBool aIsRoot, nsIBoxLayout* aLayoutManager)
{
  return new (aPresShell) nsTreeColFrame(aPresShell, aContext, aIsRoot, aLayoutManager);
} 

NS_IMETHODIMP_(nsrefcnt) 
nsTreeColFrame::AddRef(void)
{
  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt)
nsTreeColFrame::Release(void)
{
  return NS_OK;
}




NS_INTERFACE_MAP_BEGIN(nsTreeColFrame)
NS_INTERFACE_MAP_END_INHERITING(nsBoxFrame)


nsTreeColFrame::~nsTreeColFrame()
{
}

NS_IMETHODIMP
nsTreeColFrame::Init(nsIContent*      aContent,
                     nsIFrame*        aParent,
                     nsIFrame*        aPrevInFlow)
{
  nsresult rv = nsBoxFrame::Init(aContent, aParent, aPrevInFlow);
  InvalidateColumns();
  return rv;
}

void                                                                
nsTreeColFrame::Destroy()                          
{
  InvalidateColumns();
  nsBoxFrame::Destroy();
}

class nsDisplayXULTreeColSplitterTarget : public nsDisplayItem {
public:
  nsDisplayXULTreeColSplitterTarget(nsIFrame* aFrame) : nsDisplayItem(aFrame) {
    MOZ_COUNT_CTOR(nsDisplayXULTreeColSplitterTarget);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayXULTreeColSplitterTarget() {
    MOZ_COUNT_DTOR(nsDisplayXULTreeColSplitterTarget);
  }
#endif

  virtual nsIFrame* HitTest(nsDisplayListBuilder* aBuilder, nsPoint aPt);
  NS_DISPLAY_DECL_NAME("XULTreeColSplitterTarget")
};

nsIFrame* 
nsDisplayXULTreeColSplitterTarget::HitTest(nsDisplayListBuilder* aBuilder,
                                           nsPoint aPt)
{
  nsPoint pt = aPt - aBuilder->ToReferenceFrame(mFrame);
  
  
  PRBool left = PR_FALSE;
  PRBool right = PR_FALSE;
  if (mFrame->GetSize().width - nsPresContext::CSSPixelsToAppUnits(4) <= pt.x)
    right = PR_TRUE;
  else if (nsPresContext::CSSPixelsToAppUnits(4) > pt.x)
    left = PR_TRUE;

  if (left || right) {
    
    nsFrameList frames(mFrame->GetParent()->GetFirstChild(nsnull));
    nsIFrame* child;
    if (left)
      child = frames.GetPrevSiblingFor(mFrame);
    else
      child = mFrame->GetNextSibling();

    if (child && child->GetContent()->NodeInfo()->Equals(nsGkAtoms::splitter,
                                                         kNameSpaceID_XUL)) {
      return child;
    }
  }
  
  return nsnull;
}

nsresult
nsTreeColFrame::BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                            const nsRect&           aDirtyRect,
                                            const nsDisplayListSet& aLists)
{
  if (!aBuilder->IsForEventDelivery())
    return nsBoxFrame::BuildDisplayListForChildren(aBuilder, aDirtyRect, aLists);
  
  nsDisplayListCollection set;
  nsresult rv = nsBoxFrame::BuildDisplayListForChildren(aBuilder, aDirtyRect, set);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = WrapListsInRedirector(aBuilder, set, aLists);
  NS_ENSURE_SUCCESS(rv, rv);

  return aLists.Content()->AppendNewToTop(new (aBuilder)
      nsDisplayXULTreeColSplitterTarget(this));
}

NS_IMETHODIMP
nsTreeColFrame::AttributeChanged(PRInt32 aNameSpaceID,
                                 nsIAtom* aAttribute,
                                 PRInt32 aModType)
{
  nsresult rv = nsBoxFrame::AttributeChanged(aNameSpaceID, aAttribute,
                                             aModType);

  if (aAttribute == nsGkAtoms::ordinal || aAttribute == nsGkAtoms::primary) {
    InvalidateColumns();
  }

  return rv;
}

void
nsTreeColFrame::SetBounds(nsBoxLayoutState& aBoxLayoutState,
                          const nsRect& aRect,
                          PRBool aRemoveOverflowArea) {
  nscoord oldWidth = mRect.width;

  nsBoxFrame::SetBounds(aBoxLayoutState, aRect, aRemoveOverflowArea);
  if (mRect.width != oldWidth) {
    nsITreeBoxObject* treeBoxObject = GetTreeBoxObject();
    if (treeBoxObject) {
      treeBoxObject->Invalidate();
    }
  }
}

nsITreeBoxObject*
nsTreeColFrame::GetTreeBoxObject()
{
  nsITreeBoxObject* result = nsnull;

  nsIContent* parent = mContent->GetParent();
  if (parent) {
    nsIContent* grandParent = parent->GetParent();
    nsCOMPtr<nsIDOMXULElement> treeElement = do_QueryInterface(grandParent);
    if (treeElement) {
      nsCOMPtr<nsIBoxObject> boxObject;
      treeElement->GetBoxObject(getter_AddRefs(boxObject));

      nsCOMPtr<nsITreeBoxObject> treeBoxObject = do_QueryInterface(boxObject);
      result = treeBoxObject.get();
    }
  }
  return result;
}

void
nsTreeColFrame::InvalidateColumns()
{
  nsITreeBoxObject* treeBoxObject = GetTreeBoxObject();
  if (treeBoxObject) {
    nsCOMPtr<nsITreeColumns> columns;
    treeBoxObject->GetColumns(getter_AddRefs(columns));

    if (columns)
      columns->InvalidateColumns();
  }
}
