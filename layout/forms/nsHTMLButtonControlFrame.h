




































#ifndef nsHTMLButtonControlFrame_h___
#define nsHTMLButtonControlFrame_h___

#include "nsCOMPtr.h"
#include "nsHTMLContainerFrame.h"
#include "nsIFormControlFrame.h"
#include "nsHTMLParts.h"

#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsStyleContext.h"
#include "nsLeafFrame.h"
#include "nsCSSRendering.h"
#include "nsISupports.h"
#include "nsStyleConsts.h"
#include "nsIComponentManager.h"
#include "nsButtonFrameRenderer.h"

class nsIRenderingContext;

class nsHTMLButtonControlFrame : public nsHTMLContainerFrame,
                                 public nsIFormControlFrame 
{
public:
  nsHTMLButtonControlFrame(nsStyleContext* aContext);
  ~nsHTMLButtonControlFrame();

  virtual void Destroy();

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);

  virtual nscoord GetPrefWidth(nsIRenderingContext *aRenderingContext);

  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  virtual PRBool IsContainingBlock() const;
  
  NS_IMETHOD HandleEvent(nsPresContext* aPresContext, 
                         nsGUIEvent* aEvent,
                         nsEventStatus* aEventStatus);

  NS_IMETHOD  Init(nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIFrame*        asPrevInFlow);

  virtual nsStyleContext* GetAdditionalStyleContext(PRInt32 aIndex) const;
  virtual void SetAdditionalStyleContext(PRInt32 aIndex, 
                                         nsStyleContext* aStyleContext);
 
  NS_IMETHOD AppendFrames(nsIAtom*        aListName,
                          nsFrameList&    aFrameList);

  NS_IMETHOD InsertFrames(nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsFrameList&    aFrameList);

  NS_IMETHOD RemoveFrame(nsIAtom*        aListName,
                         nsIFrame*       aOldFrame);

#ifdef ACCESSIBILITY
  NS_IMETHOD GetAccessible(nsIAccessible** aAccessible);
#endif

  virtual nsIAtom* GetType() const;
  
#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const {
    return MakeFrameName(NS_LITERAL_STRING("HTMLButtonControl"), aResult);
  }
#endif

  virtual PRBool HonorPrintBackgroundSettings() { return PR_FALSE; }

  
  void SetFocus(PRBool aOn, PRBool aRepaint);
  virtual nsresult SetFormProperty(nsIAtom* aName, const nsAString& aValue);
  virtual nsresult GetFormProperty(nsIAtom* aName, nsAString& aValue) const; 

  
  virtual nsIFrame* GetContentInsertionFrame() {
    return GetFirstChild(nsnull)->GetContentInsertionFrame();
  }

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsHTMLContainerFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eReplaced | nsIFrame::eReplacedContainsBlock));
  }

protected:
  virtual PRBool IsInput() { return PR_FALSE; }
  void ReflowButtonContents(nsPresContext* aPresContext,
                            nsHTMLReflowMetrics& aDesiredSize,
                            const nsHTMLReflowState& aReflowState,
                            nsIFrame* aFirstKid,
                            nsMargin aFocusPadding,
                            nsReflowStatus& aStatus);

  PRIntn GetSkipSides() const;
  nsButtonFrameRenderer mRenderer;
};

#endif




