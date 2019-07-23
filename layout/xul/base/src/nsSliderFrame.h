




































#ifndef nsSliderFrame_h__
#define nsSliderFrame_h__

#include "nsRepeatService.h"
#include "nsBoxFrame.h"
#include "prtypes.h"
#include "nsIAtom.h"
#include "nsCOMPtr.h"
#include "nsITimer.h"
#include "nsIDOMMouseListener.h"

class nsString;
class nsITimer;
class nsSliderFrame;

nsIFrame* NS_NewSliderFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

class nsSliderMediator : public nsIDOMMouseListener
{
public:

  NS_DECL_ISUPPORTS

  nsSliderFrame* mSlider;

  nsSliderMediator(nsSliderFrame* aSlider) {  mSlider = aSlider; }
  virtual ~nsSliderMediator() {}

  virtual void SetSlider(nsSliderFrame* aSlider) { mSlider = aSlider; }

 




  NS_IMETHOD MouseDown(nsIDOMEvent* aMouseEvent);

  




  NS_IMETHOD MouseUp(nsIDOMEvent* aMouseEvent);

  





  NS_IMETHOD MouseClick(nsIDOMEvent* aMouseEvent) { return NS_OK; }

  





  NS_IMETHOD MouseDblClick(nsIDOMEvent* aMouseEvent) { return NS_OK; }

  




  NS_IMETHOD MouseOver(nsIDOMEvent* aMouseEvent) { return NS_OK; }

  




  NS_IMETHOD MouseOut(nsIDOMEvent* aMouseEvent) { return NS_OK; }

  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent) { return NS_OK; }
};

class nsSliderFrame : public nsBoxFrame
{
public:
  friend class nsSliderMediator;

  nsSliderFrame(nsIPresShell* aShell, nsStyleContext* aContext);
  virtual ~nsSliderFrame();

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const {
    return MakeFrameName(NS_LITERAL_STRING("SliderFrame"), aResult);
  }
#endif

  
  virtual nsSize GetPrefSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMinSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMaxSize(nsBoxLayoutState& aBoxLayoutState);
  NS_IMETHOD DoLayout(nsBoxLayoutState& aBoxLayoutState);

  
  NS_IMETHOD  AppendFrames(nsIAtom*        aListName,
                           nsFrameList&    aFrameList);

  NS_IMETHOD  InsertFrames(nsIAtom*        aListName,
                           nsIFrame*       aPrevFrame,
                           nsFrameList&    aFrameList);

  NS_IMETHOD  RemoveFrame(nsIAtom*        aListName,
                          nsIFrame*       aOldFrame);

  virtual void Destroy();

  NS_IMETHOD BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);
 
  NS_IMETHOD AttributeChanged(PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aModType);

  NS_IMETHOD  Init(nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIFrame*        asPrevInFlow);


  NS_IMETHOD HandleEvent(nsPresContext* aPresContext, 
                         nsGUIEvent* aEvent,
                         nsEventStatus* aEventStatus);

  NS_IMETHOD SetInitialChildList(nsIAtom*        aListName,
                                 nsFrameList&    aChildList);

  virtual nsIAtom* GetType() const;

  NS_IMETHOD MouseDown(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseUp(nsIDOMEvent* aMouseEvent);

  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent) { return NS_OK; }

  static PRInt32 GetCurrentPosition(nsIContent* content);
  static PRInt32 GetMinPosition(nsIContent* content);
  static PRInt32 GetMaxPosition(nsIContent* content);
  static PRInt32 GetIncrement(nsIContent* content);
  static PRInt32 GetPageIncrement(nsIContent* content);
  static PRInt32 GetIntegerAttribute(nsIContent* content, nsIAtom* atom, PRInt32 defaultValue);
  void EnsureOrient();

  NS_IMETHOD HandlePress(nsPresContext* aPresContext,
                         nsGUIEvent *    aEvent,
                         nsEventStatus*  aEventStatus);

  NS_IMETHOD HandleMultiplePress(nsPresContext* aPresContext,
                                 nsGUIEvent *    aEvent,
                                 nsEventStatus*  aEventStatus,
                                 PRBool aControlHeld)  { return NS_OK; }

  NS_IMETHOD HandleDrag(nsPresContext* aPresContext,
                        nsGUIEvent *    aEvent,
                        nsEventStatus*  aEventStatus)  { return NS_OK; }

  NS_IMETHOD HandleRelease(nsPresContext* aPresContext,
                           nsGUIEvent *    aEvent,
                           nsEventStatus*  aEventStatus);

private:

  PRBool GetScrollToClick();
  nsIBox* GetScrollbar();

  void PageUpDown(nscoord change);
  void SetCurrentThumbPosition(nsIContent* aScrollbar, nscoord aNewPos, PRBool aIsSmooth,
                               PRBool aImmediateRedraw, PRBool aMaySnap);
  void SetCurrentPosition(nsIContent* aScrollbar, PRInt32 aNewPos, PRBool aIsSmooth,
                          PRBool aImmediateRedraw);
  void SetCurrentPositionInternal(nsIContent* aScrollbar, PRInt32 pos,
                                  PRBool aIsSmooth, PRBool aImmediateRedraw);
  nsresult CurrentPositionChanged(nsPresContext* aPresContext,
                                  PRBool aImmediateRedraw);
  void DragThumb(PRBool aGrabMouseEvents);
  void AddListener();
  void RemoveListener();
  PRBool isDraggingThumb();

  void StartRepeat() {
    nsRepeatService::GetInstance()->Start(Notify, this);
  }
  void StopRepeat() {
    nsRepeatService::GetInstance()->Stop(Notify, this);
  }
  void Notify();
  static void Notify(void* aData) {
    (static_cast<nsSliderFrame*>(aData))->Notify();
  }
 
  nsPoint mDestinationPoint;
  nsRefPtr<nsSliderMediator> mMediator;

  float mRatio;

  nscoord mDragStart;
  nscoord mThumbStart;

  PRInt32 mCurPos;

  nscoord mChange;

  
  
  
  PRBool mUserChanged;

  static PRBool gMiddlePref;
  static PRInt32 gSnapMultiplier;
}; 

#endif
