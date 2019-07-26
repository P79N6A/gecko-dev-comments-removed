












#ifndef nsBoxFrame_h___
#define nsBoxFrame_h___

#include "mozilla/Attributes.h"
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

  NS_IMETHOD RelayoutChildAtOrdinal(nsBoxLayoutState& aState, nsIFrame* aChild);

  virtual nsSize GetPrefSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMinSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMaxSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nscoord GetFlex(nsBoxLayoutState& aBoxLayoutState);
  virtual nscoord GetBoxAscent(nsBoxLayoutState& aBoxLayoutState);
#ifdef DEBUG_LAYOUT
  NS_IMETHOD SetDebug(nsBoxLayoutState& aBoxLayoutState, bool aDebug);
  NS_IMETHOD GetDebug(bool& aDebug) MOZ_OVERRIDE;
#endif
  virtual Valignment GetVAlign() const MOZ_OVERRIDE { return mValign; }
  virtual Halignment GetHAlign() const MOZ_OVERRIDE { return mHalign; }
  NS_IMETHOD DoLayout(nsBoxLayoutState& aBoxLayoutState);

  virtual bool ComputesOwnOverflowArea() MOZ_OVERRIDE { return false; }

  

  
  
  virtual void Init(nsIContent*      aContent,
                    nsIFrame*        aParent,
                    nsIFrame*        asPrevInFlow) MOZ_OVERRIDE;

 
  NS_IMETHOD AttributeChanged(int32_t         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              int32_t         aModType) MOZ_OVERRIDE;

  virtual void MarkIntrinsicWidthsDirty() MOZ_OVERRIDE;
  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;

  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  NS_IMETHOD  AppendFrames(ChildListID     aListID,
                           nsFrameList&    aFrameList) MOZ_OVERRIDE;

  NS_IMETHOD  InsertFrames(ChildListID     aListID,
                           nsIFrame*       aPrevFrame,
                           nsFrameList&    aFrameList) MOZ_OVERRIDE;

  NS_IMETHOD  RemoveFrame(ChildListID     aListID,
                          nsIFrame*       aOldFrame) MOZ_OVERRIDE;

  virtual nsIFrame* GetContentInsertionFrame();

  NS_IMETHOD  SetInitialChildList(ChildListID     aListID,
                                  nsFrameList&    aChildList) MOZ_OVERRIDE;

  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext) MOZ_OVERRIDE;

  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

  virtual bool IsFrameOfType(uint32_t aFlags) const
  {
    
    
    
    
    

    
    
    
    
    return nsContainerFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eReplaced | nsIFrame::eReplacedContainsBlock | eXULBox |
        nsIFrame::eExcludesIgnorableWhitespace));
  }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

  NS_IMETHOD DidReflow(nsPresContext*           aPresContext,
                       const nsHTMLReflowState*  aReflowState,
                       nsDidReflowStatus         aStatus) MOZ_OVERRIDE;

  virtual bool HonorPrintBackgroundSettings();

  virtual ~nsBoxFrame();
  
  nsBoxFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, bool aIsRoot = false, nsBoxLayout* aLayoutManager = nullptr);

  
  
  virtual void BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                           const nsRect&           aDirtyRect,
                                           const nsDisplayListSet& aLists);

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;
  
#ifdef DEBUG_LAYOUT
    virtual void SetDebugOnChildList(nsBoxLayoutState& aState, nsIFrame* aChild, bool aDebug);
    nsresult DisplayDebugInfoFor(nsIFrame*  aBox, 
                                 nsPoint& aPoint);
#endif

  static nsresult LayoutChildAt(nsBoxLayoutState& aState, nsIFrame* aBox, const nsRect& aRect);

  




  void WrapListsInRedirector(nsDisplayListBuilder*   aBuilder,
                             const nsDisplayListSet& aIn,
                             const nsDisplayListSet& aOut);

  



  virtual bool SupportsOrdinalsInChildren();

protected:
#ifdef DEBUG_LAYOUT
    virtual void GetBoxName(nsAutoString& aName) MOZ_OVERRIDE;
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
  
    virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

    nsSize mPrefSize;
    nsSize mMinSize;
    nsSize mMaxSize;
    nscoord mFlex;
    nscoord mAscent;

    nsCOMPtr<nsBoxLayout> mLayoutManager;

    
    
    bool GetEventPoint(nsGUIEvent *aEvent, nsPoint &aPoint);
    
    
    bool GetEventPoint(nsGUIEvent *aEvent, nsIntPoint &aPoint);

protected:
    void RegUnregAccessKey(bool aDoReg);

  NS_HIDDEN_(void) CheckBoxOrder();

private: 

#ifdef DEBUG_LAYOUT
    nsresult SetDebug(nsPresContext* aPresContext, bool aDebug);
    bool GetInitialDebug(bool& aDebug);
    void GetDebugPref(nsPresContext* aPresContext);

    void GetDebugBorder(nsMargin& aInset);
    void GetDebugPadding(nsMargin& aInset);
    void GetDebugMargin(nsMargin& aInset);

    nsresult GetFrameSizeWithMargin(nsIFrame* aBox, nsSize& aSize);

    void PixelMarginToTwips(nsPresContext* aPresContext, nsMargin& aMarginPixels);

    void GetValue(nsPresContext* aPresContext, const nsSize& a, const nsSize& b, char* value);
    void GetValue(nsPresContext* aPresContext, int32_t a, int32_t b, char* value);
    void DrawSpacer(nsPresContext* aPresContext, nsRenderingContext& aRenderingContext, bool aHorizontal, int32_t flex, nscoord x, nscoord y, nscoord size, nscoord spacerSize);
    void DrawLine(nsRenderingContext& aRenderingContext,  bool aHorizontal, nscoord x1, nscoord y1, nscoord x2, nscoord y2);
    void FillRect(nsRenderingContext& aRenderingContext,  bool aHorizontal, nscoord x, nscoord y, nscoord width, nscoord height);
#endif
    virtual void UpdateMouseThrough();

    void CacheAttributes();

    
    Halignment mHalign;
    Valignment mValign;

#ifdef DEBUG_LAYOUT
    static bool gDebug;
    static nsIFrame* mDebugChild;
#endif

}; 

#endif

