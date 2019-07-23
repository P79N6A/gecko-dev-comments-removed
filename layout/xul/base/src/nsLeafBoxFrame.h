



































#ifndef nsLeafBoxFrame_h___
#define nsLeafBoxFrame_h___

#include "nsLeafFrame.h"
#include "nsBox.h"

class nsAccessKeyInfo;

class nsLeafBoxFrame : public nsLeafFrame
{
public:

  friend nsIFrame* NS_NewLeafBoxFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  
  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);

  virtual nsSize GetPrefSize(nsBoxLayoutState& aState);
  virtual nsSize GetMinSize(nsBoxLayoutState& aState);
  virtual nsSize GetMaxSize(nsBoxLayoutState& aState);
  virtual nscoord GetFlex(nsBoxLayoutState& aState);
  virtual nscoord GetBoxAscent(nsBoxLayoutState& aState);

  virtual nsIAtom* GetType() const;
  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    
    
    return nsLeafFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eReplaced | nsIFrame::eReplacedContainsBlock | nsIFrame::eXULBox));
  }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  

  virtual void MarkIntrinsicWidthsDirty();
  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsIRenderingContext *aRenderingContext);

  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  NS_IMETHOD CharacterDataChanged(nsPresContext* aPresContext,
                                  nsIContent*     aChild,
                                  PRBool          aAppend);

  NS_IMETHOD  Init(
               nsIContent*      aContent,
               nsIFrame*        aParent,
               nsIFrame*        asPrevInFlow);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  NS_IMETHOD AttributeChanged(PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aModType);

  virtual PRBool GetMouseThrough() const;
  virtual PRBool ComputesOwnOverflowArea() { return PR_FALSE; }

protected:

  virtual PRBool GetWasCollapsed(nsBoxLayoutState& aState);
  virtual void SetWasCollapsed(nsBoxLayoutState& aState, PRBool aWas);

  NS_IMETHOD DoLayout(nsBoxLayoutState& aState);

#ifdef DEBUG_LAYOUT
  virtual void GetBoxName(nsAutoString& aName);
#endif

  virtual nscoord GetIntrinsicWidth();

 nsLeafBoxFrame(nsIPresShell* aShell, nsStyleContext* aContext);

protected:
  eMouseThrough mMouseThrough;

private:

 void UpdateMouseThrough();


}; 

#endif 
