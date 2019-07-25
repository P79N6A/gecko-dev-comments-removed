






































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

  virtual bool IsFrameOfType(PRUint32 aFlags) const
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
  NS_IMETHOD SetInitialChildList(ChildListID     aListID,
                                 nsFrameList&    aChildList);
  virtual nsFrameList GetChildList(ChildListID aListID) const;
  virtual void GetChildLists(nsTArray<ChildList>* aLists) const;

  virtual nsIFrame* GetContentInsertionFrame();

  
  virtual nsresult SetFormProperty(nsIAtom* aName, const nsAString& aValue);
  virtual nsresult GetFormProperty(nsIAtom* aName, nsAString& aValue) const; 
  







  virtual void SetFocus(bool aOn, bool aRepaint);

  
  virtual bool IsDroppedDown() { return mDroppedDown; }
  


  virtual void ShowDropDown(bool aDoDropDown);
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
  NS_IMETHOD DoneAddingChildren(bool aIsDone);
  NS_IMETHOD OnOptionSelected(PRInt32 aIndex, bool aSelected);
  NS_IMETHOD OnSetSelectedIndex(PRInt32 aOldIndex, PRInt32 aNewIndex);

  
  



  NS_IMETHOD Rollup(PRUint32 aCount, nsIContent** aLastRolledUp);
  



  NS_IMETHOD ShouldRollupOnMouseWheelEvent(bool *aShouldRollup)
    { *aShouldRollup = true; return NS_OK;}

  



  NS_IMETHOD ShouldRollupOnMouseActivate(bool *aShouldRollup)
    { *aShouldRollup = false; return NS_OK;}

  
  NS_IMETHOD SaveState(SpecialStateID aStateID, nsPresState** aState);
  NS_IMETHOD RestoreState(nsPresState* aState);

  static bool ToolkitHasNativePopup();

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
  
  



  void ShowPopup(bool aShowPopup);

  





  bool ShowList(bool aShowList);
  void CheckFireOnChange();
  void FireValueChangeEvent();
  nsresult RedisplayText(PRInt32 aIndex);
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
  
  bool                  mDroppedDown;             
  bool                  mInRedisplayText;

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
