




































#ifndef nsIEventStateManager_h__
#define nsIEventStateManager_h__

#include "nsEvent.h"
#include "nsISupports.h"
#include "nsEventStates.h"

class nsIContent;
class nsIDocument;
class nsPresContext;
class nsIDOMEvent;
class nsIFrame;
class nsIView;
class nsIWidget;
class imgIContainer;




#define NS_IEVENTSTATEMANAGER_IID \
{0x69ab5b16, 0x6690, 0x42fc, \
  { 0xa9, 0xe5, 0xa3, 0xb4, 0xf8, 0x0f, 0xcb, 0xa6 } }

#define NS_EVENT_NEEDS_FRAME(event) (!NS_IS_ACTIVATION_EVENT(event))

class nsIEventStateManager : public nsISupports {

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IEVENTSTATEMANAGER_IID)

  NS_IMETHOD Init() = 0;

  NS_IMETHOD PreHandleEvent(nsPresContext* aPresContext, 
                            nsEvent *aEvent, 
                            nsIFrame* aTargetFrame,
                            nsEventStatus* aStatus,
                            nsIView* aView) = 0;

  NS_IMETHOD PostHandleEvent(nsPresContext* aPresContext, 
                             nsEvent *aEvent, 
                             nsIFrame* aTargetFrame,
                             nsEventStatus* aStatus,
                             nsIView* aView) = 0;

  NS_IMETHOD SetPresContext(nsPresContext* aPresContext) = 0;
  NS_IMETHOD ClearFrameRefs(nsIFrame* aFrame) = 0;

  NS_IMETHOD GetEventTarget(nsIFrame **aFrame) = 0;
  NS_IMETHOD GetEventTargetContent(nsEvent* aEvent, nsIContent** aContent) = 0;

  









  virtual nsEventStates GetContentState(nsIContent *aContent,
                                        PRBool aFollowLabels = PR_FALSE) = 0;

  














  virtual PRBool SetContentState(nsIContent *aContent, nsEventStates aState) = 0;

  NS_IMETHOD ContentRemoved(nsIDocument* aDocument, nsIContent* aContent) = 0;
  NS_IMETHOD EventStatusOK(nsGUIEvent* aEvent, PRBool *aOK) = 0;

  

  






  NS_IMETHOD RegisterAccessKey(nsIContent* aContent, PRUint32 aKey) = 0;

  





  NS_IMETHOD UnregisterAccessKey(nsIContent* aContent, PRUint32 aKey) = 0;

  






  NS_IMETHOD GetRegisteredAccessKey(nsIContent* aContent, PRUint32* aKey) = 0;

  NS_IMETHOD SetCursor(PRInt32 aCursor, imgIContainer* aContainer,
                       PRBool aHaveHotspot, float aHotspotX, float aHotspotY,
                       nsIWidget* aWidget, PRBool aLockCursor) = 0;

  NS_IMETHOD NotifyDestroyPresContext(nsPresContext* aPresContext) = 0;
  
  






  NS_IMETHOD_(PRBool) IsHandlingUserInputExternal() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIEventStateManager, NS_IEVENTSTATEMANAGER_IID)

#endif 
