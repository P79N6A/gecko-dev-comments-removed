



#ifndef nsListControlFrame_h___
#define nsListControlFrame_h___

#ifdef DEBUG_evaughan

#endif

#ifdef DEBUG_rods




#endif

#include "mozilla/Attributes.h"
#include "nsGfxScrollFrame.h"
#include "nsIFormControlFrame.h"
#include "nsIListControlFrame.h"
#include "nsISelectControlFrame.h"
#include "nsAutoPtr.h"
#include "nsSelectsAreaFrame.h"


#ifdef KeyPress
#undef KeyPress
#endif

class nsIComboboxControlFrame;
class nsPresContext;
class nsListEventListener;

namespace mozilla {
namespace dom {
class HTMLOptionElement;
class HTMLOptionsCollection;
} 
} 





class nsListControlFrame final : public nsHTMLScrollFrame,
                                 public nsIFormControlFrame,
                                 public nsIListControlFrame,
                                 public nsISelectControlFrame
{
public:
  friend nsContainerFrame* NS_NewListControlFrame(nsIPresShell* aPresShell,
                                                  nsStyleContext* aContext);

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

    
  virtual nsresult HandleEvent(nsPresContext* aPresContext,
                               mozilla::WidgetGUIEvent* aEvent,
                               nsEventStatus* aEventStatus) override;
  
  virtual void SetInitialChildList(ChildListID     aListID,
                                   nsFrameList&    aChildList) override;

  virtual nscoord GetPrefISize(nsRenderingContext *aRenderingContext) override;
  virtual nscoord GetMinISize(nsRenderingContext *aRenderingContext) override;

  virtual void Reflow(nsPresContext*           aCX,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) override;

  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) override;

  virtual void DidReflow(nsPresContext*            aPresContext, 
                         const nsHTMLReflowState*  aReflowState, 
                         nsDidReflowStatus         aStatus) override;
  virtual void DestroyFrom(nsIFrame* aDestructRoot) override;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) override;

  virtual nsContainerFrame* GetContentInsertionFrame() override;

  




  virtual nsIAtom* GetType() const override;

  virtual bool IsFrameOfType(uint32_t aFlags) const override
  {
    return nsHTMLScrollFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eReplaced | nsIFrame::eReplacedContainsBlock));
  }

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override;
#endif

    
  virtual nsresult SetFormProperty(nsIAtom* aName, const nsAString& aValue) override;
  virtual void SetFocus(bool aOn = true, bool aRepaint = false) override;

  virtual mozilla::ScrollbarStyles GetScrollbarStyles() const override;
  virtual bool ShouldPropagateComputedHeightToScrolledContent() const override;

    
#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() override;
#endif

    
  virtual void SetComboboxFrame(nsIFrame* aComboboxFrame) override;
  virtual int32_t GetSelectedIndex() override;
  virtual mozilla::dom::HTMLOptionElement* GetCurrentOption() override;

  




  virtual void GetOptionText(uint32_t aIndex, nsAString& aStr) override;

  virtual void CaptureMouseEvents(bool aGrabMouseEvents) override;
  virtual nscoord GetHeightOfARow() override;
  virtual uint32_t GetNumberOfOptions() override;
  virtual void AboutToDropDown() override;

  


  virtual void AboutToRollup() override;

  



  virtual void FireOnChange() override;

  



  virtual void ComboboxFinish(int32_t aIndex) override;
  virtual void OnContentReset() override;

  
  NS_IMETHOD AddOption(int32_t index) override;
  NS_IMETHOD RemoveOption(int32_t index) override;
  NS_IMETHOD DoneAddingChildren(bool aIsDone) override;

  



  NS_IMETHOD OnOptionSelected(int32_t aIndex, bool aSelected) override;
  NS_IMETHOD OnSetSelectedIndex(int32_t aOldIndex, int32_t aNewIndex) override;

  



  nsresult MouseDown(nsIDOMEvent* aMouseEvent);
  nsresult MouseUp(nsIDOMEvent* aMouseEvent);
  nsresult MouseMove(nsIDOMEvent* aMouseEvent);
  nsresult DragMove(nsIDOMEvent* aMouseEvent);
  nsresult KeyDown(nsIDOMEvent* aKeyEvent);
  nsresult KeyPress(nsIDOMEvent* aKeyEvent);

  


  mozilla::dom::HTMLOptionsCollection* GetOptions() const;
  


  mozilla::dom::HTMLOptionElement* GetOption(uint32_t aIndex) const;

  static void ComboboxFocusSet();

  
  bool IsFocused() { return this == mFocused; }

  




  void PaintFocus(nsRenderingContext& aRC, nsPoint aPt);
  





  void InvalidateFocus();

  



  nscoord CalcHeightOfARow();

  



  bool MightNeedSecondPass() const {
    return mMightNeedSecondPass;
  }

  void SetSuppressScrollbarUpdate(bool aSuppress) {
    nsHTMLScrollFrame::SetSuppressScrollbarUpdate(aSuppress);
  }

  


  bool IsInDropDownMode() const;

  


  uint32_t GetNumDisplayRows() const { return mNumDisplayRows; }

  



  bool GetDropdownCanGrow() const { return mDropdownCanGrow; }

  


  virtual bool NeedsView() override { return IsInDropDownMode(); }

  


  static void Shutdown();

#ifdef ACCESSIBILITY
  




  void FireMenuItemActiveEvent(); 
#endif

protected:
  




  bool       UpdateSelection();

  


  bool       GetMultiple() const {
    return mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::multiple);
  }


  



  void       DropDownToggleKey(nsIDOMEvent* aKeyEvent);

  nsresult   IsOptionDisabled(int32_t anIndex, bool &aIsDisabled);
  


  void ScrollToFrame(mozilla::dom::HTMLOptionElement& aOptElement);
  


  void ScrollToIndex(int32_t anIndex);

  









  bool       IgnoreMouseEventForSelection(nsIDOMEvent* aEvent);

  



  void       UpdateInListState(nsIDOMEvent* aEvent);
  void       AdjustIndexForDisabledOpt(int32_t aStartIndex, int32_t &anNewIndex,
                                       int32_t aNumOptions, int32_t aDoAdjustInc, int32_t aDoAdjustIncNext);

  



  virtual void ResetList(bool aAllowScrolling);

  explicit nsListControlFrame(nsStyleContext* aContext);
  virtual ~nsListControlFrame();

  





  nsresult GetIndexFromDOMEvent(nsIDOMEvent* aMouseEvent, int32_t& aCurIndex);

  bool     CheckIfAllFramesHere();
  bool     IsLeftButton(nsIDOMEvent* aMouseEvent);

  
  nscoord  CalcFallbackRowHeight(float aFontSizeInflation);

  
  
  nscoord CalcIntrinsicBSize(nscoord aHeightOfARow, int32_t aNumberOfOptions);

  
  void     SetComboboxItem(int32_t aIndex);

  




  void ReflowAsDropdown(nsPresContext*           aPresContext,
                        nsHTMLReflowMetrics&     aDesiredSize,
                        const nsHTMLReflowState& aReflowState,
                        nsReflowStatus&          aStatus);

  
  bool     SetOptionsSelectedFromFrame(int32_t aStartIndex,
                                       int32_t aEndIndex,
                                       bool aValue,
                                       bool aClearAll);
  bool     ToggleOptionSelectedFromFrame(int32_t aIndex);
  


  bool     SingleSelection(int32_t aClickedIndex, bool aDoToggle);
  bool     ExtendedSelection(int32_t aStartIndex, int32_t aEndIndex,
                             bool aClearAll);
  


  bool     PerformSelection(int32_t aClickedIndex, bool aIsShift,
                            bool aIsControl);
  


  bool     HandleListSelection(nsIDOMEvent * aDOMEvent, int32_t selectedIndex);
  void     InitSelectionRange(int32_t aClickedIndex);
  void     PostHandleKeyEvent(int32_t aNewIndex, uint32_t aCharCode,
                              bool aIsShift, bool aIsControlOrMeta);

public:
  nsSelectsAreaFrame* GetOptionsContainer() const {
    return static_cast<nsSelectsAreaFrame*>(GetScrolledFrame());
  }

protected:
  nscoord HeightOfARow() {
    return GetOptionsContainer()->HeightOfARow();
  }

  


  uint32_t GetNumberOfRows();
  
  
  int32_t      mStartSelectionIndex;
  int32_t      mEndSelectionIndex;

  nsIComboboxControlFrame *mComboboxFrame;
  uint32_t     mNumDisplayRows;
  bool mChangesSinceDragStart:1;
  bool mButtonDown:1;
  
  
  bool mItemSelectionStarted:1;

  bool mIsAllContentHere:1;
  bool mIsAllFramesHere:1;
  bool mHasBeenInitialized:1;
  bool mNeedToReset:1;
  bool mPostChildrenLoadedReset:1;

  
  bool mControlSelectMode:1;

  
  
  bool mMightNeedSecondPass:1;

  



  bool mHasPendingInterruptAtStartOfReflow:1;

  
  
  bool mDropdownCanGrow:1;

  
  bool mForceSelection:1;
  
  
  
  
  nscoord mLastDropdownComputedHeight;

  
  
  nscolor mLastDropdownBackstopColor;
  
  nsRefPtr<nsListEventListener> mEventListener;

  static nsListControlFrame * mFocused;
  static nsString * sIncrementalString;

#ifdef DO_REFLOW_COUNTER
  int32_t mReflowId;
#endif

private:
  
  static nsAString& GetIncrementalString ();
  static DOMTimeStamp gLastKeyTime;

  class MOZ_STACK_CLASS AutoIncrementalSearchResetter
  {
  public:
    AutoIncrementalSearchResetter() :
      mCancelled(false)
    {
    }
    ~AutoIncrementalSearchResetter()
    {
      if (!mCancelled) {
        nsListControlFrame::GetIncrementalString().Truncate();
      }
    }
    void Cancel()
    {
      mCancelled = true;
    }
  private:
    bool mCancelled;
  };
};

#endif 

