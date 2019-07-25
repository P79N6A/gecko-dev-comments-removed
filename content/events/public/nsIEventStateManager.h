




































#ifndef nsIEventStateManager_h__
#define nsIEventStateManager_h__

#include "nsEvent.h"
#include "nsISupports.h"

class nsIContent;
class nsIDocument;
class nsPresContext;
class nsIDOMEvent;
class nsIFrame;
class nsIView;
class nsIWidget;
class imgIContainer;





#define NS_IEVENTSTATEMANAGER_IID \
{ 0x92edd580, 0x062e, 0x4471, \
  { 0xad, 0xeb, 0x68, 0x32, 0x9b, 0x0e, 0xc2, 0xe4 } }

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

  









  virtual PRInt32 GetContentState(nsIContent *aContent,
                                  PRBool aFollowLabels = PR_FALSE) = 0;

  











  virtual PRBool SetContentState(nsIContent *aContent, PRInt32 aState) = 0;

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

#define NS_EVENT_STATE_ACTIVE        (1 << 0)  // mouse is down on content
#define NS_EVENT_STATE_FOCUS         (1 << 1)  // content has focus
#define NS_EVENT_STATE_HOVER         (1 << 2)  // mouse is hovering over content
#define NS_EVENT_STATE_DRAGOVER      (1 << 3)  // drag  is hovering over content
#define NS_EVENT_STATE_URLTARGET     (1 << 4)  // content is URL's target (ref)



#define NS_EVENT_STATE_CHECKED       (1 << 5)  // CSS3-Selectors
#define NS_EVENT_STATE_ENABLED       (1 << 6)  // CSS3-Selectors
#define NS_EVENT_STATE_DISABLED      (1 << 7)  // CSS3-Selectors
#define NS_EVENT_STATE_REQUIRED      (1 << 8)  // CSS3-UI
#define NS_EVENT_STATE_OPTIONAL      (1 << 9)  // CSS3-UI
#define NS_EVENT_STATE_VISITED       (1 << 10) // CSS2
#define NS_EVENT_STATE_UNVISITED     (1 << 11)
#define NS_EVENT_STATE_VALID         (1 << 12) // CSS3-UI
#define NS_EVENT_STATE_INVALID       (1 << 13) // CSS3-UI
#define NS_EVENT_STATE_INRANGE       (1 << 14) // CSS3-UI
#define NS_EVENT_STATE_OUTOFRANGE    (1 << 15) // CSS3-UI

#define NS_EVENT_STATE_MOZ_READONLY  (1 << 16) // CSS3-UI
#define NS_EVENT_STATE_MOZ_READWRITE (1 << 17) // CSS3-UI
#define NS_EVENT_STATE_DEFAULT       (1 << 18) // CSS3-UI


#define NS_EVENT_STATE_BROKEN        (1 << 19)

#define NS_EVENT_STATE_USERDISABLED  (1 << 20)

#define NS_EVENT_STATE_SUPPRESSED    (1 << 21)


#define NS_EVENT_STATE_LOADING       (1 << 22)

#define NS_EVENT_STATE_TYPE_UNSUPPORTED \
                                     (1 << 23)
#ifdef MOZ_MATHML
#define NS_EVENT_STATE_INCREMENT_SCRIPT_LEVEL \
                                     (1 << 24)
#endif

#define NS_EVENT_STATE_HANDLER_BLOCKED \
                                     (1 << 25)

#define NS_EVENT_STATE_HANDLER_DISABLED \
                                     (1 << 26)

#define NS_EVENT_STATE_INDETERMINATE (1 << 27) // CSS3-Selectors


#define NS_EVENT_STATE_HANDLER_CRASHED \
                                     (1 << 28)


#define NS_EVENT_STATE_FOCUSRING     (1 << 29)


#define NS_EVENT_STATE_MOZ_PLACEHOLDER (1 << 30)


#define NS_EVENT_STATE_MOZ_SUBMITINVALID (1U << 31)










#endif 
