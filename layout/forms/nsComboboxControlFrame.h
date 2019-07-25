






































#ifndef nsComboboxControlFrame_h___
#define nsComboboxControlFrame_h___

#ifdef DEBUG_evaughan

#endif

#ifdef DEBUG_rods





#endif


#define NS_SKIP_NOTIFY_INDEX -2

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

class nsIView;
class nsStyleContext;
class nsIListControlFrame;
class nsComboboxDisplayFrame;





#define NS_COMBO_LIST_COUNT   (NS_BLOCK_LIST_COUNT + 1)

class nsComboboxControlFrame : public nsBlockFrame,
                               public nsIFormControlFrame,
                               public nsIComboboxControlFrame,
                               public nsIAnonymousContentCreator,
                               public nsISelectControlFrame,
                               public nsIRollupListener,
                               public nsIStatefulFrame
{
public:
  friend nsIFrame* NS_NewComboboxControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, PRUint32 aFlags);
  friend class nsComboboxDisplayFrame;

  nsComboboxControlFrame(nsStyleContext* aContext);
  ~nsComboboxControlFrame();

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements);
  virtual void AppendAnonymousContentTo(nsBaseContentList& aElements,
                                        PRUint32 aFilter);
  virtual nsIFrame* CreateFrameFor(nsIContent* aContent);

#ifdef ACCESSIBILITY
  virtual already_AddRefed<nsAccessible> CreateAccessible();
#endif

  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext);

  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext);

  NS_IMETHOD Reflow(nsPresContext*          aCX,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  NS_IMETHOD HandleEvent(nsPresContext* aPresContext,
                         nsGUIEvent* aEvent,
                         nsEventStatus* aEventStatus);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  void PaintFocus(nsRenderingContext& aRenderingContext, nsPoint aPt);

  
  
  
  virtual nsIAtom* GetType() const;

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsBlockFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eReplaced | nsIFrame::eReplacedContainsBlock));
  }

  virtual nsIScrollableFrame* GetScrollTargetFrame() {
    return do_QueryFrame(mDropdownFrame);
  }

#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif
  virtual void DestroyFrom(nsIFrame* aDestructRoot);
  virtual nsFrameList GetChildList(nsIAtom* aListName) const;
  NS_IMETHOD SetInitialChildList(nsIAtom*        aListName,
                                 nsFrameList&    aChildList);
  virtual nsIAtom* GetAdditionalChildListName(PRInt32 aIndex) const;

  virtual nsIFrame* GetContentInsertionFrame();

  
  virtual nsresult SetFormProperty(nsIAtom* aName, const nsAString& aValue);
  virtual nsresult GetFormProperty(nsIAtom* aName, nsAString& aValue) const; 
  







  virtual void SetFocus(PRBool aOn, PRBool aRepaint);

  
  virtual PRBool IsDroppedDown() { return mDroppedDown; }
  


  virtual void ShowDropDown(PRBool aDoDropDown);
  virtual nsIFrame* GetDropDown();
  virtual void SetDropDown(nsIFrame* aDropDownFrame);
  


  virtual void RollupFromList();
  virtual void AbsolutelyPositionDropDown();
  virtual PRInt32 GetIndexOfDisplayArea();
  


  NS_IMETHOD RedisplaySelectedText();
  virtual PRInt32 UpdateRecentIndex(PRInt32 aIndex);
  virtual void OnContentReset();

  
  NS_IMETHOD AddOption(PRInt32 index);
  NS_IMETHOD RemoveOption(PRInt32 index);
  NS_IMETHOD DoneAddingChildren(PRBool aIsDone);
  NS_IMETHOD OnOptionSelected(PRInt32 aIndex, PRBool aSelected);
  NS_IMETHOD OnSetSelectedIndex(PRInt32 aOldIndex, PRInt32 aNewIndex);

  
  



  NS_IMETHOD Rollup(PRUint32 aCount, nsIContent** aLastRolledUp);
  



  NS_IMETHOD ShouldRollupOnMouseWheelEvent(PRBool *aShouldRollup)
    { *aShouldRollup = PR_TRUE; return NS_OK;}

  



  NS_IMETHOD ShouldRollupOnMouseActivate(PRBool *aShouldRollup)
    { *aShouldRollup = PR_FALSE; return NS_OK;}

  
  NS_IMETHOD SaveState(SpecialStateID aStateID, nsPresState** aState);
  NS_IMETHOD RestoreState(nsPresState* aState);

  static PRBool ToolkitHasNativePopup();

protected:

  
  nsresult ReflowDropdown(nsPresContext*          aPresContext, 
                          const nsHTMLReflowState& aReflowState);

  
  nscoord GetIntrinsicWidth(nsRenderingContext* aRenderingContext,
                            nsLayoutUtils::IntrinsicWidthType aType);
protected:
  class RedisplayTextEvent;
  friend class RedisplayTextEvent;

  class RedisplayTextEvent : public nsRunnable {
  public:
    NS_DECL_NSIRUNNABLE
    RedisplayTextEvent(nsComboboxControlFrame *c) : mControlFrame(c) {}
    void Revoke() { mControlFrame = nsnull; }
  private:
    nsComboboxControlFrame *mControlFrame;
  };
  
  



  void ShowPopup(PRBool aShowPopup);

  





  PRBool ShowList(PRBool aShowList);
  void CheckFireOnChange();
  void FireValueChangeEvent();
  nsresult RedisplayText(PRInt32 aIndex);
  void HandleRedisplayTextEvent();
  void ActuallyDisplayText(PRBool aNotify);

  nsFrameList              mPopupFrames;             
  nsCOMPtr<nsIContent>     mDisplayContent;          
  nsCOMPtr<nsIContent>     mButtonContent;           
  nsIFrame*                mDisplayFrame;            
  nsIFrame*                mButtonFrame;             
  nsIFrame*                mDropdownFrame;           
  nsIListControlFrame *    mListControlFrame;        

  
  
  nscoord mDisplayWidth;
  
  PRPackedBool          mDroppedDown;             
  PRPackedBool          mInRedisplayText;

  nsRevocableEventPtr<RedisplayTextEvent> mRedisplayTextEvent;

  PRInt32               mRecentSelectedIndex;
  PRInt32               mDisplayedIndex;
  nsString              mDisplayedOptionText;

  
  
  nsCOMPtr<nsIDOMEventListener> mButtonListener;

  
  
  static nsComboboxControlFrame * mFocused;

#ifdef DO_REFLOW_COUNTER
  PRInt32 mReflowId;
#endif
};

#endif
