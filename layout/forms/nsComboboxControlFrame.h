




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
#include "nsPresState.h"
#include "nsCSSFrameConstructor.h"
#include "nsIStatefulFrame.h"
#include "nsIScrollableFrame.h"
#include "nsIDOMEventListener.h"
#include "nsThreadUtils.h"

class nsView;
class nsStyleContext;
class nsIListControlFrame;
class nsComboboxDisplayFrame;

class nsComboboxControlFrame : public nsBlockFrame,
                               public nsIFormControlFrame,
                               public nsIComboboxControlFrame,
                               public nsIAnonymousContentCreator,
                               public nsISelectControlFrame,
                               public nsIRollupListener,
                               public nsIStatefulFrame
{
public:
  friend nsIFrame* NS_NewComboboxControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, uint32_t aFlags);
  friend class nsComboboxDisplayFrame;

  nsComboboxControlFrame(nsStyleContext* aContext);
  ~nsComboboxControlFrame();

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements);
  virtual void AppendAnonymousContentTo(nsBaseContentList& aElements,
                                        uint32_t aFilter) MOZ_OVERRIDE;
  virtual nsIFrame* CreateFrameFor(nsIContent* aContent) MOZ_OVERRIDE;

#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() MOZ_OVERRIDE;
#endif

  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;

  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;

  NS_IMETHOD Reflow(nsPresContext*          aCX,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  NS_IMETHOD HandleEvent(nsPresContext* aPresContext,
                         nsGUIEvent* aEvent,
                         nsEventStatus* aEventStatus);

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

  virtual nsIScrollableFrame* GetScrollTargetFrame() {
    return do_QueryFrame(mDropdownFrame);
  }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif
  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;
  NS_IMETHOD SetInitialChildList(ChildListID     aListID,
                                 nsFrameList&    aChildList) MOZ_OVERRIDE;
  virtual const nsFrameList& GetChildList(ChildListID aListID) const MOZ_OVERRIDE;
  virtual void GetChildLists(nsTArray<ChildList>* aLists) const MOZ_OVERRIDE;

  virtual nsIFrame* GetContentInsertionFrame();

  
  virtual nsresult SetFormProperty(nsIAtom* aName, const nsAString& aValue) MOZ_OVERRIDE;
  virtual nsresult GetFormProperty(nsIAtom* aName, nsAString& aValue) const MOZ_OVERRIDE; 
  







  virtual void SetFocus(bool aOn, bool aRepaint);

  
  virtual bool IsDroppedDown() { return mDroppedDown; }
  


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

  
  NS_IMETHOD AddOption(int32_t index);
  NS_IMETHOD RemoveOption(int32_t index) MOZ_OVERRIDE;
  NS_IMETHOD DoneAddingChildren(bool aIsDone) MOZ_OVERRIDE;
  NS_IMETHOD OnOptionSelected(int32_t aIndex, bool aSelected) MOZ_OVERRIDE;
  NS_IMETHOD OnSetSelectedIndex(int32_t aOldIndex, int32_t aNewIndex) MOZ_OVERRIDE;

  
  



  virtual bool Rollup(uint32_t aCount, nsIContent** aLastRolledUp);
  virtual void NotifyGeometryChange();

  



  virtual bool ShouldRollupOnMouseWheelEvent()
    { return true; }

  



  virtual bool ShouldRollupOnMouseActivate()
    { return false; }

  virtual uint32_t GetSubmenuWidgetChain(nsTArray<nsIWidget*> *aWidgetChain)
    { return 0; }

  virtual nsIWidget* GetRollupWidget();

  
  NS_IMETHOD SaveState(nsPresState** aState) MOZ_OVERRIDE;
  NS_IMETHOD RestoreState(nsPresState* aState) MOZ_OVERRIDE;

  static bool ToolkitHasNativePopup();

protected:
  friend class RedisplayTextEvent;
  friend class nsAsyncResize;
  friend class nsResizeDropdownAtFinalPosition;

  
  nsresult ReflowDropdown(nsPresContext*          aPresContext, 
                          const nsHTMLReflowState& aReflowState);

  enum DropDownPositionState {
    
    eDropDownPositionSuppressed,
    
    eDropDownPositionPendingResize,
    
    eDropDownPositionFinal
  };
  DropDownPositionState AbsolutelyPositionDropDown();

  
  nscoord GetIntrinsicWidth(nsRenderingContext* aRenderingContext,
                            nsLayoutUtils::IntrinsicWidthType aType);

  class RedisplayTextEvent : public nsRunnable {
  public:
    NS_DECL_NSIRUNNABLE
    RedisplayTextEvent(nsComboboxControlFrame *c) : mControlFrame(c) {}
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
  nsIFrame*                mDisplayFrame;            
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
