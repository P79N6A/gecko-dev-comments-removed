




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

namespace mozilla {
namespace gfx {
class DrawTarget;
}
}

class nsComboboxControlFrame final : public nsBlockFrame,
                                     public nsIFormControlFrame,
                                     public nsIComboboxControlFrame,
                                     public nsIAnonymousContentCreator,
                                     public nsISelectControlFrame,
                                     public nsIRollupListener,
                                     public nsIStatefulFrame
{
  typedef mozilla::gfx::DrawTarget DrawTarget;

public:
  friend nsContainerFrame* NS_NewComboboxControlFrame(nsIPresShell* aPresShell,
                                                      nsStyleContext* aContext,
                                                      nsFrameState aFlags);
  friend class nsComboboxDisplayFrame;

  explicit nsComboboxControlFrame(nsStyleContext* aContext);
  ~nsComboboxControlFrame();

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements) override;
  virtual void AppendAnonymousContentTo(nsTArray<nsIContent*>& aElements,
                                        uint32_t aFilter) override;
  virtual nsIFrame* CreateFrameFor(nsIContent* aContent) override;

#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() override;
#endif

  virtual nscoord GetMinISize(nsRenderingContext *aRenderingContext) override;

  virtual nscoord GetPrefISize(nsRenderingContext *aRenderingContext) override;

  virtual void Reflow(nsPresContext*           aCX,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) override;

  virtual nsresult HandleEvent(nsPresContext* aPresContext,
                               mozilla::WidgetGUIEvent* aEvent,
                               nsEventStatus* aEventStatus) override;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) override;

  void PaintFocus(DrawTarget& aDrawTarget, nsPoint aPt);

  
  
  
  virtual nsIAtom* GetType() const override;

  virtual bool IsFrameOfType(uint32_t aFlags) const override
  {
    return nsBlockFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eReplaced | nsIFrame::eReplacedContainsBlock));
  }

  virtual nsIScrollableFrame* GetScrollTargetFrame() override {
    return do_QueryFrame(mDropdownFrame);
  }

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override;
#endif
  virtual void DestroyFrom(nsIFrame* aDestructRoot) override;
  virtual void SetInitialChildList(ChildListID     aListID,
                                   nsFrameList&    aChildList) override;
  virtual const nsFrameList& GetChildList(ChildListID aListID) const override;
  virtual void GetChildLists(nsTArray<ChildList>* aLists) const override;

  virtual nsContainerFrame* GetContentInsertionFrame() override;

  
  virtual nsresult SetFormProperty(nsIAtom* aName, const nsAString& aValue) override;
  







  virtual void SetFocus(bool aOn, bool aRepaint) override;

  
  virtual bool IsDroppedDown() override { return mDroppedDown; }
  


  virtual void ShowDropDown(bool aDoDropDown) override;
  virtual nsIFrame* GetDropDown() override;
  virtual void SetDropDown(nsIFrame* aDropDownFrame) override;
  


  virtual void RollupFromList() override;

  





  void GetAvailableDropdownSpace(mozilla::WritingMode aWM,
                                 nscoord* aBefore,
                                 nscoord* aAfter,
                                 mozilla::LogicalPoint* aTranslation);
  virtual int32_t GetIndexOfDisplayArea() override;
  


  NS_IMETHOD RedisplaySelectedText() override;
  virtual int32_t UpdateRecentIndex(int32_t aIndex) override;
  virtual void OnContentReset() override;

  
  NS_IMETHOD AddOption(int32_t index) override;
  NS_IMETHOD RemoveOption(int32_t index) override;
  NS_IMETHOD DoneAddingChildren(bool aIsDone) override;
  NS_IMETHOD OnOptionSelected(int32_t aIndex, bool aSelected) override;
  NS_IMETHOD OnSetSelectedIndex(int32_t aOldIndex, int32_t aNewIndex) override;

  
  



  virtual bool Rollup(uint32_t aCount, bool aFlush,
                      const nsIntPoint* pos, nsIContent** aLastRolledUp) override;
  virtual void NotifyGeometryChange() override;

  



  virtual bool ShouldRollupOnMouseWheelEvent() override
    { return true; }

  virtual bool ShouldConsumeOnMouseWheelEvent() override
    { return false; }

  



  virtual bool ShouldRollupOnMouseActivate() override
    { return false; }

  virtual uint32_t GetSubmenuWidgetChain(nsTArray<nsIWidget*> *aWidgetChain) override
    { return 0; }

  virtual nsIWidget* GetRollupWidget() override;

  
  NS_IMETHOD SaveState(nsPresState** aState) override;
  NS_IMETHOD RestoreState(nsPresState* aState) override;

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

  
  
  nscoord mDisplayISize;
  
  nsRevocableEventPtr<RedisplayTextEvent> mRedisplayTextEvent;

  int32_t               mRecentSelectedIndex;
  int32_t               mDisplayedIndex;
  nsString              mDisplayedOptionText;

  
  
  nsCOMPtr<nsIDOMEventListener> mButtonListener;

  
  
  
  
  
  nscoord               mLastDropDownBeforeScreenBCoord;
  nscoord               mLastDropDownAfterScreenBCoord;
  
  bool                  mDroppedDown;
  
  bool                  mInRedisplayText;
  
  bool                  mDelayedShowDropDown;

  
  
  static nsComboboxControlFrame* sFocused;

#ifdef DO_REFLOW_COUNTER
  int32_t mReflowId;
#endif
};

#endif
