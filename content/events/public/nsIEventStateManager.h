




































#ifndef nsIEventStateManager_h__
#define nsIEventStateManager_h__

#include "nsEvent.h"
#include "nsISupports.h"

class nsIContent;
class nsPresContext;
class nsIDOMEvent;
class nsIFrame;
class nsIView;
class nsIWidget;
class imgIContainer;





#define NS_IEVENTSTATEMANAGER_IID \
{ 0x9d25327a, 0x7a17, 0x4d19, \
  { 0x92, 0x8c, 0xf7, 0xf3, 0xac, 0x19, 0xb7, 0x63 } }



#define NS_EVENT_NEEDS_FRAME(event) (!NS_IS_FOCUS_EVENT(event))

class nsIEventStateManager : public nsISupports {

public:
  enum EFocusedWithType {
    eEventFocusedByUnknown,     
    eEventFocusedByMouse,       
    eEventFocusedByKey,         
    eEventFocusedByContextMenu, 
    eEventFocusedByApplication  
  };

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

  NS_IMETHOD GetContentState(nsIContent *aContent, PRInt32& aState) = 0;

  











  virtual PRBool SetContentState(nsIContent *aContent, PRInt32 aState) = 0;

  NS_IMETHOD GetFocusedContent(nsIContent **aContent) = 0;
  NS_IMETHOD SetFocusedContent(nsIContent* aContent) = 0;

  
  NS_IMETHOD GetLastFocusedContent(nsIContent **aContent) = 0;

  NS_IMETHOD GetFocusedFrame(nsIFrame **aFrame) = 0;

  NS_IMETHOD ContentRemoved(nsIContent* aContent) = 0;
  NS_IMETHOD EventStatusOK(nsGUIEvent* aEvent, PRBool *aOK) = 0;

  
  virtual PRBool GetBrowseWithCaret() = 0;

  
  
  NS_IMETHOD MoveFocusToCaret(PRBool aCanFocusDoc, PRBool *aIsSelectionWithFocus) = 0;
  NS_IMETHOD MoveCaretToFocus() = 0;

  
  
  NS_IMETHOD ChangeFocusWith(nsIContent *aFocusContent, EFocusedWithType aFocusedWith) = 0;

  
  NS_IMETHOD RegisterAccessKey(nsIContent* aContent, PRUint32 aKey) = 0;
  NS_IMETHOD UnregisterAccessKey(nsIContent* aContent, PRUint32 aKey) = 0;

  NS_IMETHOD SetCursor(PRInt32 aCursor, imgIContainer* aContainer,
                       PRBool aHaveHotspot, float aHotspotX, float aHotspotY,
                       nsIWidget* aWidget, PRBool aLockCursor) = 0;

  
  NS_IMETHOD ShiftFocus(PRBool aDirection, nsIContent* aStart)=0;

  NS_IMETHOD NotifyDestroyPresContext(nsPresContext* aPresContext) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIEventStateManager, NS_IEVENTSTATEMANAGER_IID)

#define NS_EVENT_STATE_ACTIVE        0x00000001 // mouse is down on content
#define NS_EVENT_STATE_FOCUS         0x00000002 // content has focus
#define NS_EVENT_STATE_HOVER         0x00000004 // mouse is hovering over content
#define NS_EVENT_STATE_DRAGOVER      0x00000008 // drag  is hovering over content
#define NS_EVENT_STATE_URLTARGET     0x00000010 // content is URL's target (ref)



#define NS_EVENT_STATE_CHECKED       0x00000020 // CSS3-Selectors
#define NS_EVENT_STATE_ENABLED       0x00000040 // CSS3-Selectors
#define NS_EVENT_STATE_DISABLED      0x00000080 // CSS3-Selectors
#define NS_EVENT_STATE_REQUIRED      0x00000100 // CSS3-UI
#define NS_EVENT_STATE_OPTIONAL      0x00000200 // CSS3-UI
#define NS_EVENT_STATE_VISITED       0x00000400 // CSS2
#define NS_EVENT_STATE_VALID         0x00000800 // CSS3-UI
#define NS_EVENT_STATE_INVALID       0x00001000 // CSS3-UI
#define NS_EVENT_STATE_INRANGE       0x00002000 // CSS3-UI
#define NS_EVENT_STATE_OUTOFRANGE    0x00004000 // CSS3-UI

#define NS_EVENT_STATE_MOZ_READONLY  0x00008000 // CSS3-UI
#define NS_EVENT_STATE_MOZ_READWRITE 0x00010000 // CSS3-UI
#define NS_EVENT_STATE_DEFAULT       0x00020000 // CSS3-UI


#define NS_EVENT_STATE_BROKEN        0x00040000

#define NS_EVENT_STATE_USERDISABLED  0x00080000

#define NS_EVENT_STATE_SUPPRESSED    0x00100000


#define NS_EVENT_STATE_LOADING       0x00200000

#define NS_EVENT_STATE_TYPE_UNSUPPORTED \
                                     0x00400000

#endif
