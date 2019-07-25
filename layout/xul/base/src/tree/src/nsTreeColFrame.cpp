





































#include "nsCOMPtr.h"
#include "nsTreeColFrame.h"
#include "nsGkAtoms.h"
#include "nsIContent.h"
#include "nsStyleContext.h"
#include "nsINameSpaceManager.h" 
#include "nsIDOMNSDocument.h"
#include "nsIDocument.h"
#include "nsIBoxObject.h"
#include "nsTreeBoxObject.h"
#include "nsIDOMElement.h"
#include "nsITreeBoxObject.h"
#include "nsITreeColumns.h"
#include "nsIDOMXULTreeElement.h"
#include "nsDisplayList.h"
#include "nsTreeBodyFrame.h"






nsIFrame*
NS_NewTreeColFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsTreeColFrame(aPresShell, aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsTreeColFrame)


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
nsTreeColFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  InvalidateColumns(PR_FALSE);
  nsBoxFrame::DestroyFrom(aDestructRoot);
}

class nsDisplayXULTreeColSplitterTarget : public nsDisplayItem {
public:
  nsDisplayXULTreeColSplitterTarget(nsDisplayListBuilder* aBuilder,
                                    nsIFrame* aFrame) :
    nsDisplayItem(aBuilder, aFrame) {
    MOZ_COUNT_CTOR(nsDisplayXULTreeColSplitterTarget);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayXULTreeColSplitterTarget() {
    MOZ_COUNT_DTOR(nsDisplayXULTreeColSplitterTarget);
  }
#endif

  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames);
  NS_DISPLAY_DECL_NAME("XULTreeColSplitterTarget", TYPE_XUL_TREE_COL_SPLITTER_TARGET)
};

void
nsDisplayXULTreeColSplitterTarget::HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                                           HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames)
{
  nsRect rect = aRect - ToReferenceFrame();
  
  
  PRBool left = PR_FALSE;
  PRBool right = PR_FALSE;
  if (mFrame->GetSize().width - nsPresContext::CSSPixelsToAppUnits(4) <= rect.XMost()) {
    right = PR_TRUE;
  } else if (nsPresContext::CSSPixelsToAppUnits(4) > rect.x) {
    left = PR_TRUE;
  }

  
  if (mFrame->GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL) {
    PRBool tmp = left;
    left = right;
    right = tmp;
  }

  if (left || right) {
    
    nsIFrame* child;
    if (left)
      child = mFrame->GetPrevSibling();
    else
      child = mFrame->GetNextSibling();

    if (child && child->GetContent()->NodeInfo()->Equals(nsGkAtoms::splitter,
                                                         kNameSpaceID_XUL)) {
      aOutFrames->AppendElement(child);
    }
  }

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
      nsDisplayXULTreeColSplitterTarget(aBuilder, this));
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
nsTreeColFrame::InvalidateColumns(PRBool aCanWalkFrameTree)
{
  nsITreeBoxObject* treeBoxObject = GetTreeBoxObject();
  if (treeBoxObject) {
    nsCOMPtr<nsITreeColumns> columns;

    if (aCanWalkFrameTree) {
      treeBoxObject->GetColumns(getter_AddRefs(columns));
    } else {
      nsTreeBodyFrame* body = static_cast<nsTreeBoxObject*>(treeBoxObject)->GetCachedTreeBody();
      if (body) {
        body->GetColumns(getter_AddRefs(columns));
      }
    }

    if (columns)
      columns->InvalidateColumns();
  }
}
