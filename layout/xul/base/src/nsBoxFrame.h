












#ifndef nsBoxFrame_h___
#define nsBoxFrame_h___

#include "nsCOMPtr.h"
#include "nsContainerFrame.h"
#include "nsBoxLayout.h"

class nsBoxLayoutState;


#define NS_STATE_BOX_CHILD_RESERVED      NS_FRAME_STATE_BIT(20)
#define NS_STATE_STACK_NOT_POSITIONED    NS_FRAME_STATE_BIT(21)

#define NS_STATE_AUTO_STRETCH            NS_FRAME_STATE_BIT(23)

#define NS_STATE_CURRENTLY_IN_DEBUG      NS_FRAME_STATE_BIT(25)


#define NS_STATE_MENU_HAS_POPUP_LIST       NS_FRAME_STATE_BIT(28) /* used on nsMenuFrame */
#define NS_STATE_BOX_WRAPS_KIDS_IN_BLOCK NS_FRAME_STATE_BIT(29)
#define NS_STATE_EQUAL_SIZE              NS_FRAME_STATE_BIT(30)

#define NS_FRAME_MOUSE_THROUGH_ALWAYS    NS_FRAME_STATE_BIT(60)
#define NS_FRAME_MOUSE_THROUGH_NEVER     NS_FRAME_STATE_BIT(61)

nsIFrame* NS_NewBoxFrame(nsIPresShell* aPresShell,
                         nsStyleContext* aContext,
                         bool aIsRoot,
                         nsBoxLayout* aLayoutManager);
nsIFrame* NS_NewBoxFrame(nsIPresShell* aPresShell,
                         nsStyleContext* aContext);

class nsBoxFrame : public nsContainerFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewBoxFrame(nsIPresShell* aPresShell, 
                                  nsStyleContext* aContext,
                                  bool aIsRoot,
                                  nsBoxLayout* aLayoutManager);
  friend nsIFrame* NS_NewBoxFrame(nsIPresShell* aPresShell,
                                  nsStyleContext* aContext);

  
  

  
  virtual void SetLayoutManager(nsBoxLayout* aLayout) { mLayoutManager = aLayout; }
  virtual nsBoxLayout* GetLayoutManager() { return mLayoutManager; }

  NS_IMETHOD RelayoutChildAtOrdinal(nsBoxLayoutState& aState, nsIBox* aChild);

  virtual nsSize GetPrefSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMinSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMaxSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nscoord GetFlex(nsBoxLayoutState& aBoxLayoutState);
  virtual nscoord GetBoxAscent(nsBoxLayoutState& aBoxLayoutState);
#ifdef DEBUG_LAYOUT
  NS_IMETHOD SetDebug(nsBoxLayoutState& aBoxLayoutState, bool aDebug);
  NS_IMETHOD GetDebug(bool& aDebug);
#endif
  virtual Valignment GetVAlign() const { return mValign; }
  virtual Halignment GetHAlign() const { return mHalign; }
  NS_IMETHOD DoLayout(nsBoxLayoutState& aBoxLayoutState);

  virtual bool ComputesOwnOverflowArea() { return false; }

  

  
  
  NS_IMETHOD  Init(nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIFrame*        asPrevInFlow);

 
  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

  virtual void MarkIntrinsicWidthsDirty();
  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext);

  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  NS_IMETHOD  AppendFrames(ChildListID     aListID,
                           nsFrameList&    aFrameList);

  NS_IMETHOD  InsertFrames(ChildListID     aListID,
                           nsIFrame*       aPrevFrame,
                           nsFrameList&    aFrameList);

  NS_IMETHOD  RemoveFrame(ChildListID     aListID,
                          nsIFrame*       aOldFrame);

  virtual nsIFrame* GetContentInsertionFrame();

  NS_IMETHOD  SetInitialChildList(ChildListID     aListID,
                                  nsFrameList&    aChildList);

  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);

  virtual nsIAtom* GetType() const;

  virtual bool IsFrameOfType(PRUint32 aFlags) const
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

  virtual bool HonorPrintBackgroundSettings();

  virtual ~nsBoxFrame();
  
  nsBoxFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, bool aIsRoot = false, nsBoxLayout* aLayoutManager = nsnull);

  
  
  NS_IMETHOD BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);
  
#ifdef DEBUG_LAYOUT
    virtual void SetDebugOnChildList(nsBoxLayoutState& aState, nsIBox* aChild, bool aDebug);
    nsresult DisplayDebugInfoFor(nsIBox*  aBox, 
                                 nsPoint& aPoint);
#endif

  static nsresult LayoutChildAt(nsBoxLayoutState& aState, nsIBox* aBox, const nsRect& aRect);

  




  nsresult WrapListsInRedirector(nsDisplayListBuilder*   aBuilder,
                                 const nsDisplayListSet& aIn,
                                 const nsDisplayListSet& aOut);

  



  virtual bool SupportsOrdinalsInChildren();

protected:
#ifdef DEBUG_LAYOUT
    virtual void GetBoxName(nsAutoString& aName);
    void PaintXULDebugBackground(nsRenderingContext& aRenderingContext,
                                 nsPoint aPt);
    void PaintXULDebugOverlay(nsRenderingContext& aRenderingContext,
                              nsPoint aPt);
#endif

    virtual bool GetInitialEqualSize(bool& aEqualSize); 
    virtual void GetInitialOrientation(bool& aIsHorizontal);
    virtual void GetInitialDirection(bool& aIsNormal);
    virtual bool GetInitialHAlignment(Halignment& aHalign); 
    virtual bool GetInitialVAlignment(Valignment& aValign); 
    virtual bool GetInitialAutoStretch(bool& aStretch); 
  
    virtual void DestroyFrom(nsIFrame* aDestructRoot);

    nsSize mPrefSize;
    nsSize mMinSize;
    nsSize mMaxSize;
    nscoord mFlex;
    nscoord mAscent;

    nsCOMPtr<nsBoxLayout> mLayoutManager;

protected:
    nsresult RegUnregAccessKey(bool aDoReg);

  NS_HIDDEN_(void) CheckBoxOrder(nsBoxLayoutState& aState);

private: 

#ifdef DEBUG_LAYOUT
    nsresult SetDebug(nsPresContext* aPresContext, bool aDebug);
    bool GetInitialDebug(bool& aDebug);
    void GetDebugPref(nsPresContext* aPresContext);

    void GetDebugBorder(nsMargin& aInset);
    void GetDebugPadding(nsMargin& aInset);
    void GetDebugMargin(nsMargin& aInset);

    nsresult GetFrameSizeWithMargin(nsIBox* aBox, nsSize& aSize);

    void PixelMarginToTwips(nsPresContext* aPresContext, nsMargin& aMarginPixels);

    void GetValue(nsPresContext* aPresContext, const nsSize& a, const nsSize& b, char* value);
    void GetValue(nsPresContext* aPresContext, PRInt32 a, PRInt32 b, char* value);
    void DrawSpacer(nsPresContext* aPresContext, nsRenderingContext& aRenderingContext, bool aHorizontal, PRInt32 flex, nscoord x, nscoord y, nscoord size, nscoord spacerSize);
    void DrawLine(nsRenderingContext& aRenderingContext,  bool aHorizontal, nscoord x1, nscoord y1, nscoord x2, nscoord y2);
    void FillRect(nsRenderingContext& aRenderingContext,  bool aHorizontal, nscoord x, nscoord y, nscoord width, nscoord height);
#endif
    virtual void UpdateMouseThrough();

    void CacheAttributes();

    
    Halignment mHalign;
    Valignment mValign;

#ifdef DEBUG_LAYOUT
    static bool gDebug;
    static nsIBox* mDebugChild;
#endif

}; 

#endif

