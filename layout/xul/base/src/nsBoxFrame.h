












































#ifndef nsBoxFrame_h___
#define nsBoxFrame_h___

#include "nsCOMPtr.h"
#include "nsContainerFrame.h"
class nsBoxLayoutState;


#define NS_STATE_BOX_CHILD_RESERVED      0x00100000
#define NS_STATE_STACK_NOT_POSITIONED    0x00200000

#define NS_STATE_AUTO_STRETCH            0x00800000

#define NS_STATE_CURRENTLY_IN_DEBUG      0x02000000



#define NS_STATE_BOX_WRAPS_KIDS_IN_BLOCK 0x20000000
#define NS_STATE_EQUAL_SIZE              0x40000000


nsIFrame* NS_NewBoxFrame(nsIPresShell* aPresShell,
                         nsStyleContext* aContext,
                         PRBool aIsRoot,
                         nsIBoxLayout* aLayoutManager);
nsIFrame* NS_NewBoxFrame(nsIPresShell* aPresShell,
                         nsStyleContext* aContext);

class nsBoxFrame : public nsContainerFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewBoxFrame(nsIPresShell* aPresShell, 
                                  nsStyleContext* aContext,
                                  PRBool aIsRoot,
                                  nsIBoxLayout* aLayoutManager);
  friend nsIFrame* NS_NewBoxFrame(nsIPresShell* aPresShell,
                                  nsStyleContext* aContext);

  
  

  
  NS_IMETHOD SetLayoutManager(nsIBoxLayout* aLayout);
  NS_IMETHOD GetLayoutManager(nsIBoxLayout** aLayout);
  NS_IMETHOD RelayoutChildAtOrdinal(nsBoxLayoutState& aState, nsIBox* aChild);

  virtual nsSize GetPrefSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMinSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMaxSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nscoord GetFlex(nsBoxLayoutState& aBoxLayoutState);
  virtual nscoord GetBoxAscent(nsBoxLayoutState& aBoxLayoutState);
#ifdef DEBUG_LAYOUT
  NS_IMETHOD SetDebug(nsBoxLayoutState& aBoxLayoutState, PRBool aDebug);
  NS_IMETHOD GetDebug(PRBool& aDebug);
#endif
  virtual Valignment GetVAlign() const { return mValign; }
  virtual Halignment GetHAlign() const { return mHalign; }
  NS_IMETHOD DoLayout(nsBoxLayoutState& aBoxLayoutState);

  virtual PRBool GetMouseThrough() const;
  virtual PRBool ComputesOwnOverflowArea() { return PR_FALSE; }

  

  
  
  NS_IMETHOD  Init(nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIFrame*        asPrevInFlow);

 
  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

  virtual void MarkIntrinsicWidthsDirty();
  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsIRenderingContext *aRenderingContext);

  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  NS_IMETHOD  AppendFrames(nsIAtom*        aListName,
                           nsFrameList&    aFrameList);

  NS_IMETHOD  InsertFrames(nsIAtom*        aListName,
                           nsIFrame*       aPrevFrame,
                           nsFrameList&    aFrameList);

  NS_IMETHOD  RemoveFrame(nsIAtom*        aListName,
                          nsIFrame*       aOldFrame);

  virtual nsIFrame* GetContentInsertionFrame();

  NS_IMETHOD  SetInitialChildList(nsIAtom*        aListName,
                                  nsFrameList&    aChildList);

  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);

  virtual nsIAtom* GetType() const;

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    
    
    
    
    

    
    
    
    
    return nsContainerFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eReplaced | nsIFrame::eReplacedContainsBlock | eXULBox |
        nsIFrame::eExcludesIgnorableWhitespace));
  }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  NS_IMETHOD DidReflow(nsPresContext*           aPresContext,
                       const nsHTMLReflowState*  aReflowState,
                       nsDidReflowStatus         aStatus);

  virtual PRBool HonorPrintBackgroundSettings();

  virtual ~nsBoxFrame();
  
  nsBoxFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, PRBool aIsRoot = PR_FALSE, nsIBoxLayout* aLayoutManager = nsnull);

  
  
  static nsresult CreateViewForFrame(nsPresContext* aPresContext,
                                     nsIFrame* aChild,
                                     nsStyleContext* aStyleContext,
                                     PRBool aForce,
                                     PRBool aIsPopup = PR_FALSE);

  
  
  NS_IMETHOD BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);
  
#ifdef DEBUG_LAYOUT
    virtual void SetDebugOnChildList(nsBoxLayoutState& aState, nsIBox* aChild, PRBool aDebug);
    nsresult DisplayDebugInfoFor(nsIBox*  aBox, 
                                 nsPoint& aPoint);
#endif

  static nsresult LayoutChildAt(nsBoxLayoutState& aState, nsIBox* aBox, const nsRect& aRect);

  
  
  
  void FireDOMEventSynch(const nsAString& aDOMEventName, nsIContent *aContent = nsnull);

  




  nsresult WrapListsInRedirector(nsDisplayListBuilder*   aBuilder,
                                 const nsDisplayListSet& aIn,
                                 const nsDisplayListSet& aOut);

  



  virtual PRBool SupportsOrdinalsInChildren();

protected:
#ifdef DEBUG_LAYOUT
    virtual void GetBoxName(nsAutoString& aName);
    void PaintXULDebugBackground(nsIRenderingContext& aRenderingContext,
                                 nsPoint aPt);
    void PaintXULDebugOverlay(nsIRenderingContext& aRenderingContext,
                              nsPoint aPt);
#endif

    virtual PRBool GetInitialEqualSize(PRBool& aEqualSize); 
    virtual void GetInitialOrientation(PRBool& aIsHorizontal);
    virtual void GetInitialDirection(PRBool& aIsNormal);
    virtual PRBool GetInitialHAlignment(Halignment& aHalign); 
    virtual PRBool GetInitialVAlignment(Valignment& aValign); 
    virtual PRBool GetInitialAutoStretch(PRBool& aStretch); 
  
    virtual void Destroy();

    nsSize mPrefSize;
    nsSize mMinSize;
    nsSize mMaxSize;
    nscoord mFlex;
    nscoord mAscent;

    nsCOMPtr<nsIBoxLayout> mLayoutManager;

protected:
    nsresult RegUnregAccessKey(PRBool aDoReg);

  NS_HIDDEN_(void) CheckBoxOrder(nsBoxLayoutState& aState);

private: 

#ifdef DEBUG_LAYOUT
    nsresult SetDebug(nsPresContext* aPresContext, PRBool aDebug);
    PRBool GetInitialDebug(PRBool& aDebug);
    void GetDebugPref(nsPresContext* aPresContext);

    void GetDebugBorder(nsMargin& aInset);
    void GetDebugPadding(nsMargin& aInset);
    void GetDebugMargin(nsMargin& aInset);

    nsresult GetFrameSizeWithMargin(nsIBox* aBox, nsSize& aSize);

    void PixelMarginToTwips(nsPresContext* aPresContext, nsMargin& aMarginPixels);

    void GetValue(nsPresContext* aPresContext, const nsSize& a, const nsSize& b, char* value);
    void GetValue(nsPresContext* aPresContext, PRInt32 a, PRInt32 b, char* value);
    void DrawSpacer(nsPresContext* aPresContext, nsIRenderingContext& aRenderingContext, PRBool aHorizontal, PRInt32 flex, nscoord x, nscoord y, nscoord size, nscoord spacerSize);
    void DrawLine(nsIRenderingContext& aRenderingContext,  PRBool aHorizontal, nscoord x1, nscoord y1, nscoord x2, nscoord y2);
    void FillRect(nsIRenderingContext& aRenderingContext,  PRBool aHorizontal, nscoord x, nscoord y, nscoord width, nscoord height);
#endif
    void UpdateMouseThrough();

    void CacheAttributes();

    
    Halignment mHalign;
    Valignment mValign;

    eMouseThrough mMouseThrough;

#ifdef DEBUG_LAYOUT
    static PRBool gDebug;
    static nsIBox* mDebugChild;
#endif

}; 

#endif

