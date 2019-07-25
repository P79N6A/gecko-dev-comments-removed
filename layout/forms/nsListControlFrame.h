




































#ifndef nsListControlFrame_h___
#define nsListControlFrame_h___

#ifdef DEBUG_evaughan

#endif

#ifdef DEBUG_rods




#endif

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
                                 nsFrameList&    aChildList);

  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext);
  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext);

  NS_IMETHOD Reflow(nsPresContext*          aCX,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  NS_IMETHOD Init(nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIFrame*        aPrevInFlow);

  NS_IMETHOD DidReflow(nsPresContext*           aPresContext, 
                       const nsHTMLReflowState*  aReflowState, 
                       nsDidReflowStatus         aStatus);
  virtual void DestroyFrom(nsIFrame* aDestructRoot);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  virtual nsIFrame* GetContentInsertionFrame();

  




  virtual nsIAtom* GetType() const;

  virtual bool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsHTMLScrollFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eReplaced | nsIFrame::eReplacedContainsBlock));
  }

  virtual bool IsContainingBlock() const;

  virtual void InvalidateInternal(const nsRect& aDamageRect,
                                  nscoord aX, nscoord aY, nsIFrame* aForChild,
                                  PRUint32 aFlags);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

    
  virtual nsresult SetFormProperty(nsIAtom* aName, const nsAString& aValue);
  virtual nsresult GetFormProperty(nsIAtom* aName, nsAString& aValue) const; 
  virtual void SetFocus(bool aOn = true, bool aRepaint = false);

  virtual nsGfxScrollFrameInner::ScrollbarStyles GetScrollbarStyles() const;
  virtual bool ShouldPropagateComputedHeightToScrolledContent() const;

    
#ifdef ACCESSIBILITY
  virtual already_AddRefed<nsAccessible> CreateAccessible();
#endif

    
  virtual PRIntn GetSkipSides() const;

    
  virtual void SetComboboxFrame(nsIFrame* aComboboxFrame);
  virtual PRInt32 GetSelectedIndex();
  virtual already_AddRefed<nsIContent> GetCurrentOption();

  




  virtual void GetOptionText(PRInt32 aIndex, nsAString & aStr);

  virtual void CaptureMouseEvents(bool aGrabMouseEvents);
  virtual nscoord GetHeightOfARow();
  virtual PRInt32 GetNumberOfOptions();  
  virtual void SyncViewWithFrame();
  virtual void AboutToDropDown();

  


  virtual void AboutToRollup();

  



  virtual void FireOnChange();

  



  virtual void ComboboxFinish(PRInt32 aIndex);
  virtual void OnContentReset();

  
  NS_IMETHOD AddOption(PRInt32 index);
  NS_IMETHOD RemoveOption(PRInt32 index);
  NS_IMETHOD DoneAddingChildren(bool aIsDone);

  



  NS_IMETHOD OnOptionSelected(PRInt32 aIndex, bool aSelected);
  NS_IMETHOD OnSetSelectedIndex(PRInt32 aOldIndex, PRInt32 aNewIndex);

  
  nsresult MouseDown(nsIDOMEvent* aMouseEvent); 
  nsresult MouseUp(nsIDOMEvent* aMouseEvent);   
  nsresult MouseMove(nsIDOMEvent* aMouseEvent);
  nsresult DragMove(nsIDOMEvent* aMouseEvent);
  nsresult KeyPress(nsIDOMEvent* aKeyEvent);    

  


  static already_AddRefed<nsIDOMHTMLOptionsCollection>
    GetOptions(nsIContent * aContent);

  



  static already_AddRefed<nsIDOMHTMLOptionElement>
    GetOption(nsIDOMHTMLOptionsCollection* aOptions, PRInt32 aIndex);

  



  static already_AddRefed<nsIContent>
    GetOptionAsContent(nsIDOMHTMLOptionsCollection* aCollection,PRInt32 aIndex);

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

  nsresult   IsOptionDisabled(PRInt32 anIndex, bool &aIsDisabled);
  nsresult   ScrollToFrame(nsIContent * aOptElement);
  nsresult   ScrollToIndex(PRInt32 anIndex);

  









  bool       IgnoreMouseEventForSelection(nsIDOMEvent* aEvent);

  



  void       UpdateInListState(nsIDOMEvent* aEvent);
  void       AdjustIndexForDisabledOpt(PRInt32 aStartIndex, PRInt32 &anNewIndex,
                                       PRInt32 aNumOptions, PRInt32 aDoAdjustInc, PRInt32 aDoAdjustIncNext);

  



  virtual void ResetList(bool aAllowScrolling);

  nsListControlFrame(nsIPresShell* aShell, nsIDocument* aDocument, nsStyleContext* aContext);
  virtual ~nsListControlFrame();

  
  nsresult GetSizeAttribute(PRInt32 *aSize);
  nsIContent* GetOptionFromContent(nsIContent *aContent);

  





  nsresult GetIndexFromDOMEvent(nsIDOMEvent* aMouseEvent, PRInt32& aCurIndex);

  



  already_AddRefed<nsIContent> GetOptionContent(PRInt32 aIndex) const;

  




  bool     IsContentSelected(nsIContent* aContent) const;

  


  bool     IsContentSelectedByIndex(PRInt32 aIndex) const;

  bool     CheckIfAllFramesHere();
  PRInt32  GetIndexFromContent(nsIContent *aContent);
  bool     IsLeftButton(nsIDOMEvent* aMouseEvent);

  
  nscoord  CalcFallbackRowHeight();

  
  
  nscoord CalcIntrinsicHeight(nscoord aHeightOfARow, PRInt32 aNumberOfOptions);

  
  void     SetComboboxItem(PRInt32 aIndex);

  




  nsresult ReflowAsDropdown(nsPresContext*           aPresContext,
                            nsHTMLReflowMetrics&     aDesiredSize,
                            const nsHTMLReflowState& aReflowState,
                            nsReflowStatus&          aStatus);

  
  bool     SetOptionsSelectedFromFrame(PRInt32 aStartIndex,
                                       PRInt32 aEndIndex,
                                       bool aValue,
                                       bool aClearAll);
  bool     ToggleOptionSelectedFromFrame(PRInt32 aIndex);
  bool     SingleSelection(PRInt32 aClickedIndex, bool aDoToggle);
  bool     ExtendedSelection(PRInt32 aStartIndex, PRInt32 aEndIndex,
                             bool aClearAll);
  bool     PerformSelection(PRInt32 aClickedIndex, bool aIsShift,
                            bool aIsControl);
  bool     HandleListSelection(nsIDOMEvent * aDOMEvent, PRInt32 selectedIndex);
  void     InitSelectionRange(PRInt32 aClickedIndex);

  nsSelectsAreaFrame* GetOptionsContainer() const {
    return static_cast<nsSelectsAreaFrame*>(GetScrolledFrame());
  }

  nscoord HeightOfARow() {
    return GetOptionsContainer()->HeightOfARow();
  }
  
  
  PRInt32      mStartSelectionIndex;
  PRInt32      mEndSelectionIndex;

  nsIComboboxControlFrame *mComboboxFrame;
  PRInt32      mNumDisplayRows;
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

  
  
  
  nscoord mLastDropdownComputedHeight;

  
  
  nscolor mLastDropdownBackstopColor;
  
  nsRefPtr<nsListEventListener> mEventListener;

  static nsListControlFrame * mFocused;
  static nsString * sIncrementalString;

#ifdef DO_REFLOW_COUNTER
  PRInt32 mReflowId;
#endif

private:
  
  static nsAString& GetIncrementalString ();
  static DOMTimeStamp gLastKeyTime;
};

#endif 

