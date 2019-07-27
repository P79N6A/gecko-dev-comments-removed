




#ifndef nsComboboxControlFrame_h___
#define nsComboboxControlFrame_h___

#ifdef DEBUG_evaughan

#endif

#ifdef DEBUG_rods





#endif


#define NS_SKIP_NOTIFY_INDEX -2

#include "mozilla/Attributes.h"
#include "nsBlockFrame.h"
#include "nsIFormControlFrame.h"
#include "nsIComboboxControlFrame.h"
#include "nsIAnonymousContentCreator.h"
#include "nsISelectControlFrame.h"
#include "nsIRollupListener.h"
#include "nsIStatefulFrame.h"
#include "nsThreadUtils.h"

class nsStyleContext;
class nsIListControlFrame;
class nsComboboxDisplayFrame;
class nsIDOMEventListener;
class nsIScrollableFrame;

class nsComboboxControlFrame MOZ_FINAL : public nsBlockFrame,
                                         public nsIFormControlFrame,
                                         public nsIComboboxControlFrame,
                                         public nsIAnonymousContentCreator,
                                         public nsISelectControlFrame,
                                         public nsIRollupListener,
                                         public nsIStatefulFrame
{
public:
  friend nsContainerFrame* NS_NewComboboxControlFrame(nsIPresShell* aPresShell,
                                                      nsStyleContext* aContext,
                                                      nsFrameState aFlags);
  friend class nsComboboxDisplayFrame;

  explicit nsComboboxControlFrame(nsStyleContext* aContext);
  ~nsComboboxControlFrame();

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements) MOZ_OVERRIDE;
  virtual void AppendAnonymousContentTo(nsTArray<nsIContent*>& aElements,
                                        uint32_t aFilter) MOZ_OVERRIDE;
  virtual nsIFrame* CreateFrameFor(nsIContent* aContent) MOZ_OVERRIDE;

#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() MOZ_OVERRIDE;
#endif

  virtual nscoord GetMinISize(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;

  virtual nscoord GetPrefISize(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;

  virtual void Reflow(nsPresContext*           aCX,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  virtual nsresult HandleEvent(nsPresContext* aPresContext,
                               mozilla::WidgetGUIEvent* aEvent,
                               nsEventStatus* aEventStatus) MOZ_OVERRIDE;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  void PaintFocus(nsRenderingContext& aRenderingContext, nsPoint aPt);

  
  
  
  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE
  {
    return nsBlockFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eReplaced | nsIFrame::eReplacedContainsBlock));
  }

  virtual nsIScrollableFrame* GetScrollTargetFrame() MOZ_OVERRIDE {
    return do_QueryFrame(mDropdownFrame);
  }

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif
  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;
  virtual void SetInitialChildList(ChildListID     aListID,
                                   nsFrameList&    aChildList) MOZ_OVERRIDE;
  virtual const nsFrameList& GetChildList(ChildListID aListID) const MOZ_OVERRIDE;
  virtual void GetChildLists(nsTArray<ChildList>* aLists) const MOZ_OVERRIDE;

  virtual nsContainerFrame* GetContentInsertionFrame() MOZ_OVERRIDE;

  
  virtual nsresult SetFormProperty(nsIAtom* aName, const nsAString& aValue) MOZ_OVERRIDE;
  







  virtual void SetFocus(bool aOn, bool aRepaint) MOZ_OVERRIDE;

  
  virtual bool IsDroppedDown() MOZ_OVERRIDE { return mDroppedDown; }
  


  virtual void ShowDropDown(bool aDoDropDown) MOZ_OVERRIDE;
  virtual nsIFrame* GetDropDown() MOZ_OVERRIDE;
  virtual void SetDropDown(nsIFrame* aDropDownFrame) MOZ_OVERRIDE;
  


  virtual void RollupFromList() MOZ_OVERRIDE;

  





  void GetAvailableDropdownSpace(nscoord* aAbove,
                                 nscoord* aBelow,
                                 nsPoint* aTranslation);
  virtual int32_t GetIndexOfDisplayArea() MOZ_OVERRIDE;
  


  NS_IMETHOD RedisplaySelectedText() MOZ_OVERRIDE;
  virtual int32_t UpdateRecentIndex(int32_t aIndex) MOZ_OVERRIDE;
  virtual void OnContentReset() MOZ_OVERRIDE;

  
  NS_IMETHOD AddOption(int32_t index) MOZ_OVERRIDE;
  NS_IMETHOD RemoveOption(int32_t index) MOZ_OVERRIDE;
  NS_IMETHOD DoneAddingChildren(bool aIsDone) MOZ_OVERRIDE;
  NS_IMETHOD OnOptionSelected(int32_t aIndex, bool aSelected) MOZ_OVERRIDE;
  NS_IMETHOD OnSetSelectedIndex(int32_t aOldIndex, int32_t aNewIndex) MOZ_OVERRIDE;

  
  



  virtual bool Rollup(uint32_t aCount, const nsIntPoint* pos, nsIContent** aLastRolledUp) MOZ_OVERRIDE;
  virtual void NotifyGeometryChange() MOZ_OVERRIDE;

  



  virtual bool ShouldRollupOnMouseWheelEvent() MOZ_OVERRIDE
    { return true; }

  virtual bool ShouldConsumeOnMouseWheelEvent() MOZ_OVERRIDE
    { return false; }

  



  virtual bool ShouldRollupOnMouseActivate() MOZ_OVERRIDE
    { return false; }

  virtual uint32_t GetSubmenuWidgetChain(nsTArray<nsIWidget*> *aWidgetChain) MOZ_OVERRIDE
    { return 0; }

  virtual nsIWidget* GetRollupWidget() MOZ_OVERRIDE;

  
  NS_IMETHOD SaveState(nsPresState** aState) MOZ_OVERRIDE;
  NS_IMETHOD RestoreState(nsPresState* aState) MOZ_OVERRIDE;

  static bool ToolkitHasNativePopup();

protected:
  friend class RedisplayTextEvent;
  friend class nsAsyncResize;
  friend class nsResizeDropdownAtFinalPosition;

  
  void ReflowDropdown(nsPresContext*          aPresContext, 
                      const nsHTMLReflowState& aReflowState);

  enum DropDownPositionState {
    
    eDropDownPositionSuppressed,
    
    eDropDownPositionPendingResize,
    
    eDropDownPositionFinal
  };
  DropDownPositionState AbsolutelyPositionDropDown();

  
  nscoord GetIntrinsicISize(nsRenderingContext* aRenderingContext,
                            nsLayoutUtils::IntrinsicISizeType aType);

  class RedisplayTextEvent : public nsRunnable {
  public:
    NS_DECL_NSIRUNNABLE
    explicit RedisplayTextEvent(nsComboboxControlFrame *c) : mControlFrame(c) {}
    void Revoke() { mControlFrame = nullptr; }
  private:
    nsComboboxControlFrame *mControlFrame;
  };
  
  



  void ShowPopup(bool aShowPopup);

  





  bool ShowList(bool aShowList);
  void CheckFireOnChange();
  void FireValueChangeEvent();
  nsresult RedisplayText(int32_t aIndex);
  void HandleRedisplayTextEvent();
  void ActuallyDisplayText(bool aNotify);

private:
  
  
  nsPoint GetCSSTransformTranslation();

protected:
  nsFrameList              mPopupFrames;             
  nsCOMPtr<nsIContent>     mDisplayContent;          
  nsCOMPtr<nsIContent>     mButtonContent;           
  nsContainerFrame*        mDisplayFrame;            
  nsIFrame*                mButtonFrame;             
  nsIFrame*                mDropdownFrame;           
  nsIListControlFrame *    mListControlFrame;        

  
  
  nscoord mDisplayWidth;
  
  nsRevocableEventPtr<RedisplayTextEvent> mRedisplayTextEvent;

  int32_t               mRecentSelectedIndex;
  int32_t               mDisplayedIndex;
  nsString              mDisplayedOptionText;

  
  
  nsCOMPtr<nsIDOMEventListener> mButtonListener;

  
  
  
  
  
  nscoord               mLastDropDownAboveScreenY;
  nscoord               mLastDropDownBelowScreenY;
  
  bool                  mDroppedDown;
  
  bool                  mInRedisplayText;
  
  bool                  mDelayedShowDropDown;

  
  
  static nsComboboxControlFrame* sFocused;

#ifdef DO_REFLOW_COUNTER
  int32_t mReflowId;
#endif
};

#endif
