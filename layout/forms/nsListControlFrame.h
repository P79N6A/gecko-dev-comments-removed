




































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

    
  NS_IMETHOD HandleEvent(nsPresContext* aPresContext,
                         nsGUIEvent* aEvent,
                         nsEventStatus* aEventStatus);
  
  NS_IMETHOD SetInitialChildList(nsIAtom*        aListName,
                                 nsFrameList&    aChildList);

  virtual nscoord GetPrefWidth(nsIRenderingContext *aRenderingContext);
  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);

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
  virtual void Destroy();

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  virtual nsIFrame* GetContentInsertionFrame();

  




  virtual nsIAtom* GetType() const;

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsHTMLScrollFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eReplaced | nsIFrame::eReplacedContainsBlock));
  }

  virtual PRBool IsContainingBlock() const;

  virtual void InvalidateInternal(const nsRect& aDamageRect,
                                  nscoord aX, nscoord aY, nsIFrame* aForChild,
                                  PRUint32 aFlags);

#ifdef DEBUG
    
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

    
  virtual nsresult SetFormProperty(nsIAtom* aName, const nsAString& aValue);
  virtual nsresult GetFormProperty(nsIAtom* aName, nsAString& aValue) const; 
  virtual void SetFocus(PRBool aOn = PR_TRUE, PRBool aRepaint = PR_FALSE);

  virtual nsGfxScrollFrameInner::ScrollbarStyles GetScrollbarStyles() const;
  virtual PRBool ShouldPropagateComputedHeightToScrolledContent() const;

    
#ifdef ACCESSIBILITY
  NS_IMETHOD GetAccessible(nsIAccessible** aAccessible);
#endif

    
  virtual PRIntn GetSkipSides() const;

    
  virtual void SetComboboxFrame(nsIFrame* aComboboxFrame);
  virtual PRInt32 GetSelectedIndex(); 

  




  virtual void GetOptionText(PRInt32 aIndex, nsAString & aStr);

  virtual void CaptureMouseEvents(PRBool aGrabMouseEvents);
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
  NS_IMETHOD GetOptionSelected(PRInt32 aIndex, PRBool* aValue);
  NS_IMETHOD DoneAddingChildren(PRBool aIsDone);

  



  NS_IMETHOD OnOptionSelected(PRInt32 aIndex, PRBool aSelected);
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

  
  PRBool IsFocused() { return this == mFocused; }

  




  void PaintFocus(nsIRenderingContext& aRC, nsPoint aPt);
  





  void InvalidateFocus();

  



  nscoord CalcHeightOfARow();

  



  PRBool MightNeedSecondPass() const {
    return mMightNeedSecondPass;
  }

  void SetSuppressScrollbarUpdate(PRBool aSuppress) {
    nsHTMLScrollFrame::SetSuppressScrollbarUpdate(aSuppress);
  }

  


  PRBool IsInDropDownMode() const;

  


  static void Shutdown();

#ifdef ACCESSIBILITY
  




  void FireMenuItemActiveEvent(); 
#endif

protected:
  



  PRBool     UpdateSelection();

  



  PRBool     GetMultiple(nsIDOMHTMLSelectElement* aSelect = nsnull) const;

  



  void       DropDownToggleKey(nsIDOMEvent* aKeyEvent);

  nsresult   IsOptionDisabled(PRInt32 anIndex, PRBool &aIsDisabled);
  nsresult   ScrollToFrame(nsIContent * aOptElement);
  nsresult   ScrollToIndex(PRInt32 anIndex);

  









  PRBool     IgnoreMouseEventForSelection(nsIDOMEvent* aEvent);

  



  void       UpdateInListState(nsIDOMEvent* aEvent);
  void       AdjustIndexForDisabledOpt(PRInt32 aStartIndex, PRInt32 &anNewIndex,
                                       PRInt32 aNumOptions, PRInt32 aDoAdjustInc, PRInt32 aDoAdjustIncNext);

  



  virtual void ResetList(PRBool aAllowScrolling);

  nsListControlFrame(nsIPresShell* aShell, nsIDocument* aDocument, nsStyleContext* aContext);
  virtual ~nsListControlFrame();

  
  nsresult GetSizeAttribute(PRInt32 *aSize);
  nsIContent* GetOptionFromContent(nsIContent *aContent);

  





  nsresult GetIndexFromDOMEvent(nsIDOMEvent* aMouseEvent, PRInt32& aCurIndex);

  



  already_AddRefed<nsIContent> GetOptionContent(PRInt32 aIndex) const;

  




  PRBool   IsContentSelected(nsIContent* aContent) const;

  


  PRBool   IsContentSelectedByIndex(PRInt32 aIndex) const;

  PRBool   IsOptionElement(nsIContent* aContent);
  PRBool   CheckIfAllFramesHere();
  PRInt32  GetIndexFromContent(nsIContent *aContent);
  PRBool   IsLeftButton(nsIDOMEvent* aMouseEvent);

  
  nscoord  CalcFallbackRowHeight();

  
  
  nscoord CalcIntrinsicHeight(nscoord aHeightOfARow, PRInt32 aNumberOfOptions);

  
  void     SetComboboxItem(PRInt32 aIndex);

  




  nsresult ReflowAsDropdown(nsPresContext*           aPresContext,
                            nsHTMLReflowMetrics&     aDesiredSize,
                            const nsHTMLReflowState& aReflowState,
                            nsReflowStatus&          aStatus);

  
  PRBool   SetOptionsSelectedFromFrame(PRInt32 aStartIndex,
                                       PRInt32 aEndIndex,
                                       PRBool aValue,
                                       PRBool aClearAll);
  PRBool   ToggleOptionSelectedFromFrame(PRInt32 aIndex);
  PRBool   SingleSelection(PRInt32 aClickedIndex, PRBool aDoToggle);
  PRBool   ExtendedSelection(PRInt32 aStartIndex, PRInt32 aEndIndex,
                             PRBool aClearAll);
  PRBool   PerformSelection(PRInt32 aClickedIndex, PRBool aIsShift,
                            PRBool aIsControl);
  PRBool   HandleListSelection(nsIDOMEvent * aDOMEvent, PRInt32 selectedIndex);
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
  PRPackedBool mChangesSinceDragStart:1;
  PRPackedBool mButtonDown:1;
  
  
  PRPackedBool mItemSelectionStarted:1;

  PRPackedBool mIsAllContentHere:1;
  PRPackedBool mIsAllFramesHere:1;
  PRPackedBool mHasBeenInitialized:1;
  PRPackedBool mNeedToReset:1;
  PRPackedBool mPostChildrenLoadedReset:1;

  
  PRPackedBool mControlSelectMode:1;

  
  
  PRPackedBool mMightNeedSecondPass:1;

  
  
  
  nscoord mLastDropdownComputedHeight;

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

