



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
#include "nsIDOMEventListener.h"
#include "nsIContent.h"
#include "nsAutoPtr.h"
#include "nsSelectsAreaFrame.h"

class nsIDOMHTMLSelectElement;
class nsIDOMHTMLOptionsCollection;
class nsIDOMHTMLOptionElement;
class nsIComboboxControlFrame;
class nsPresContext;
class nsListEventListener;





class nsListControlFrame : public nsHTMLScrollFrame,
                           public nsIFormControlFrame, 
                           public nsIListControlFrame,
                           public nsISelectControlFrame
{
public:
  friend nsIFrame* NS_NewListControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

    
  NS_IMETHOD HandleEvent(nsPresContext* aPresContext,
                         nsGUIEvent* aEvent,
                         nsEventStatus* aEventStatus);
  
  NS_IMETHOD SetInitialChildList(ChildListID     aListID,
                                 nsFrameList&    aChildList) MOZ_OVERRIDE;

  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;

  NS_IMETHOD Reflow(nsPresContext*          aCX,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  NS_IMETHOD Init(nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIFrame*        aPrevInFlow);

  NS_IMETHOD DidReflow(nsPresContext*           aPresContext, 
                       const nsHTMLReflowState*  aReflowState, 
                       nsDidReflowStatus         aStatus);
  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  virtual nsIFrame* GetContentInsertionFrame();

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

  virtual bool IsFrameOfType(uint32_t aFlags) const
  {
    return nsHTMLScrollFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eReplaced | nsIFrame::eReplacedContainsBlock));
  }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

    
  virtual nsresult SetFormProperty(nsIAtom* aName, const nsAString& aValue) MOZ_OVERRIDE;
  virtual nsresult GetFormProperty(nsIAtom* aName, nsAString& aValue) const MOZ_OVERRIDE; 
  virtual void SetFocus(bool aOn = true, bool aRepaint = false);

  virtual nsGfxScrollFrameInner::ScrollbarStyles GetScrollbarStyles() const MOZ_OVERRIDE;
  virtual bool ShouldPropagateComputedHeightToScrolledContent() const MOZ_OVERRIDE;

    
#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() MOZ_OVERRIDE;
#endif

    
  virtual void SetComboboxFrame(nsIFrame* aComboboxFrame);
  virtual int32_t GetSelectedIndex() MOZ_OVERRIDE;
  virtual already_AddRefed<nsIContent> GetCurrentOption() MOZ_OVERRIDE;

  




  virtual void GetOptionText(int32_t aIndex, nsAString & aStr) MOZ_OVERRIDE;

  virtual void CaptureMouseEvents(bool aGrabMouseEvents) MOZ_OVERRIDE;
  virtual nscoord GetHeightOfARow() MOZ_OVERRIDE;
  virtual uint32_t GetNumberOfOptions() MOZ_OVERRIDE;
  virtual void AboutToDropDown() MOZ_OVERRIDE;

  


  virtual void AboutToRollup() MOZ_OVERRIDE;

  



  virtual void FireOnChange() MOZ_OVERRIDE;

  



  virtual void ComboboxFinish(int32_t aIndex) MOZ_OVERRIDE;
  virtual void OnContentReset() MOZ_OVERRIDE;

  
  NS_IMETHOD AddOption(int32_t index);
  NS_IMETHOD RemoveOption(int32_t index) MOZ_OVERRIDE;
  NS_IMETHOD DoneAddingChildren(bool aIsDone) MOZ_OVERRIDE;

  



  NS_IMETHOD OnOptionSelected(int32_t aIndex, bool aSelected) MOZ_OVERRIDE;
  NS_IMETHOD OnSetSelectedIndex(int32_t aOldIndex, int32_t aNewIndex) MOZ_OVERRIDE;

  
  nsresult MouseDown(nsIDOMEvent* aMouseEvent); 
  nsresult MouseUp(nsIDOMEvent* aMouseEvent);   
  nsresult MouseMove(nsIDOMEvent* aMouseEvent);
  nsresult DragMove(nsIDOMEvent* aMouseEvent);
  nsresult KeyPress(nsIDOMEvent* aKeyEvent);    

  


  static already_AddRefed<nsIDOMHTMLOptionsCollection>
    GetOptions(nsIContent * aContent);

  



  static already_AddRefed<nsIDOMHTMLOptionElement>
    GetOption(nsIDOMHTMLOptionsCollection* aOptions, int32_t aIndex);

  



  static already_AddRefed<nsIContent>
    GetOptionAsContent(nsIDOMHTMLOptionsCollection* aCollection,int32_t aIndex);

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

  


  virtual bool NeedsView() { return IsInDropDownMode(); }

  


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
  nsresult   ScrollToFrame(nsIContent * aOptElement);
  nsresult   ScrollToIndex(int32_t anIndex);

  









  bool       IgnoreMouseEventForSelection(nsIDOMEvent* aEvent);

  



  void       UpdateInListState(nsIDOMEvent* aEvent);
  void       AdjustIndexForDisabledOpt(int32_t aStartIndex, int32_t &anNewIndex,
                                       int32_t aNumOptions, int32_t aDoAdjustInc, int32_t aDoAdjustIncNext);

  



  virtual void ResetList(bool aAllowScrolling);

  nsListControlFrame(nsIPresShell* aShell, nsIDocument* aDocument, nsStyleContext* aContext);
  virtual ~nsListControlFrame();

  
  nsresult GetSizeAttribute(uint32_t *aSize);
  nsIContent* GetOptionFromContent(nsIContent *aContent);

  





  nsresult GetIndexFromDOMEvent(nsIDOMEvent* aMouseEvent, int32_t& aCurIndex);

  



  already_AddRefed<nsIContent> GetOptionContent(int32_t aIndex) const;

  




  bool     IsContentSelected(nsIContent* aContent) const;

  


  bool     IsContentSelectedByIndex(int32_t aIndex) const;

  bool     CheckIfAllFramesHere();
  int32_t  GetIndexFromContent(nsIContent *aContent);
  bool     IsLeftButton(nsIDOMEvent* aMouseEvent);

  
  nscoord  CalcFallbackRowHeight(float aFontSizeInflation);

  
  
  nscoord CalcIntrinsicHeight(nscoord aHeightOfARow, int32_t aNumberOfOptions);

  
  void     SetComboboxItem(int32_t aIndex);

  




  nsresult ReflowAsDropdown(nsPresContext*           aPresContext,
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
};

#endif 

