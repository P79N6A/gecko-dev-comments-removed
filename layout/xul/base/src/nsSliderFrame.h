




































#ifndef nsSliderFrame_h__
#define nsSliderFrame_h__


#include "nsBoxFrame.h"
#include "prtypes.h"
#include "nsIAtom.h"
#include "nsCOMPtr.h"
#include "nsITimer.h"
#include "nsIDOMMouseListener.h"

class nsString;
class nsIScrollbarListener;
class nsISupportsArray;
class nsITimer;
class nsSliderFrame;

nsIFrame* NS_NewSliderFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

class nsSliderMediator : public nsIDOMMouseListener, 
                         public nsITimerCallback
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

  NS_DECL_NSITIMERCALLBACK


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
                           nsIFrame*       aFrameList);

  NS_IMETHOD  InsertFrames(nsIAtom*        aListName,
                           nsIFrame*       aPrevFrame,
                           nsIFrame*       aFrameList);

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

  virtual nsresult CurrentPositionChanged(nsPresContext* aPresContext);

  NS_IMETHOD  Init(nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIFrame*        asPrevInFlow);


  NS_IMETHOD HandleEvent(nsPresContext* aPresContext, 
                         nsGUIEvent* aEvent,
                         nsEventStatus* aEventStatus);

  NS_IMETHOD SetInitialChildList(nsIAtom*        aListName,
                                 nsIFrame*       aChildList);

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

  void SetScrollbarListener(nsIScrollbarListener* aListener);

  virtual nsIView* GetMouseCapturer() const { return GetView(); }

  NS_IMETHOD HandlePress(nsPresContext* aPresContext,
                         nsGUIEvent *    aEvent,
                         nsEventStatus*  aEventStatus);

  NS_IMETHOD HandleMultiplePress(nsPresContext* aPresContext,
                         nsGUIEvent *    aEvent,
                         nsEventStatus*  aEventStatus)  { return NS_OK; }

  NS_IMETHOD HandleDrag(nsPresContext* aPresContext,
                        nsGUIEvent *    aEvent,
                        nsEventStatus*  aEventStatus)  { return NS_OK; }

  NS_IMETHOD HandleRelease(nsPresContext* aPresContext,
                           nsGUIEvent *    aEvent,
                           nsEventStatus*  aEventStatus);

  NS_IMETHOD_(void) Notify(nsITimer *timer);
 
private:

  nsIBox* GetScrollbar();

  void PageUpDown(nscoord change);
  void SetCurrentPosition(nsIContent* scrollbar, nscoord pos, PRBool aIsSmooth);
  void SetCurrentPositionInternal(nsIContent* scrollbar, nscoord pos, PRBool aIsSmooth);
  void DragThumb(PRBool aGrabMouseEvents);
  void AddListener();
  void RemoveListener();
  PRBool isDraggingThumb();

  float mRatio;

  nscoord mDragStart;
  nscoord mThumbStart;

  PRInt32 mCurPos;

  nsIScrollbarListener* mScrollbarListener;

  nscoord mChange;
  nsPoint mDestinationPoint;
  nsSliderMediator* mMediator;

  PRPackedBool mRedrawImmediate;

  static PRBool gMiddlePref;
  static PRInt32 gSnapMultiplier;
}; 

#endif
