




































#ifndef nsDOMEvent_h__
#define nsDOMEvent_h__

#include "nsIDOMEvent.h"
#include "nsIDOMNSEvent.h"
#include "nsISupports.h"
#include "nsIPrivateDOMEvent.h"
#include "nsCOMPtr.h"
#include "nsIDOMEventTarget.h"
#include "nsPIDOMWindow.h"
#include "nsPresContext.h"
#include "nsPoint.h"
#include "nsGUIEvent.h"
#include "nsCycleCollectionParticipant.h"

class nsIContent;
class nsIScrollableView;
 
class nsDOMEvent : public nsIDOMEvent,
                   public nsIDOMNSEvent,
                   public nsIPrivateDOMEvent
{
public:

  
  enum nsDOMEvents {
    eDOMEvents_mousedown=0,
    eDOMEvents_mouseup,
    eDOMEvents_click,
    eDOMEvents_dblclick,
    eDOMEvents_mouseover,
    eDOMEvents_mouseout,
    eDOMEvents_mousemove,
    eDOMEvents_contextmenu,
    eDOMEvents_keydown,
    eDOMEvents_keyup,
    eDOMEvents_keypress,
    eDOMEvents_focus,
    eDOMEvents_blur,
    eDOMEvents_load,
    eDOMEvents_beforeunload,
    eDOMEvents_unload,
    eDOMEvents_abort,
    eDOMEvents_error,
    eDOMEvents_submit,
    eDOMEvents_reset,
    eDOMEvents_change,
    eDOMEvents_select,
    eDOMEvents_input,
    eDOMEvents_paint,
    eDOMEvents_text,
    eDOMEvents_compositionstart,
    eDOMEvents_compositionend,
    eDOMEvents_popupShowing,
    eDOMEvents_popupShown,
    eDOMEvents_popupHiding,
    eDOMEvents_popupHidden,
    eDOMEvents_close,
    eDOMEvents_command,
    eDOMEvents_broadcast,
    eDOMEvents_commandupdate,
    eDOMEvents_dragenter,
    eDOMEvents_dragover,
    eDOMEvents_dragexit,
    eDOMEvents_dragdrop,
    eDOMEvents_draggesture,
    eDOMEvents_drag,
    eDOMEvents_dragend,
    eDOMEvents_resize,
    eDOMEvents_scroll,
    eDOMEvents_overflow,
    eDOMEvents_underflow,
    eDOMEvents_overflowchanged,
    eDOMEvents_subtreemodified,
    eDOMEvents_nodeinserted,
    eDOMEvents_noderemoved,
    eDOMEvents_noderemovedfromdocument,
    eDOMEvents_nodeinsertedintodocument,
    eDOMEvents_attrmodified,
    eDOMEvents_characterdatamodified,
    eDOMEvents_DOMActivate,
    eDOMEvents_DOMFocusIn,
    eDOMEvents_DOMFocusOut,
    eDOMEvents_pageshow,
    eDOMEvents_pagehide,
    eDOMEvents_DOMMouseScroll,
    eDOMEvents_offline,
    eDOMEvents_online,
    eDOMEvents_copy,
    eDOMEvents_cut,
    eDOMEvents_paste
#ifdef MOZ_SVG
   ,
    eDOMEvents_SVGLoad,
    eDOMEvents_SVGUnload,
    eDOMEvents_SVGAbort,
    eDOMEvents_SVGError,
    eDOMEvents_SVGResize,
    eDOMEvents_SVGScroll,
    eDOMEvents_SVGZoom
#endif 
#ifdef MOZ_MEDIA
    ,
    eDOMEvents_loadstart,
    eDOMEvents_progress,
    eDOMEvents_loadedmetadata,
    eDOMEvents_loadedfirstframe,
    eDOMEvents_emptied,
    eDOMEvents_stalled,
    eDOMEvents_play,
    eDOMEvents_pause,
    eDOMEvents_waiting,
    eDOMEvents_seeking,
    eDOMEvents_seeked,
    eDOMEvents_timeupdate,
    eDOMEvents_ended,
    eDOMEvents_dataunavailable,
    eDOMEvents_canshowcurrentframe,
    eDOMEvents_canplay,
    eDOMEvents_canplaythrough,
    eDOMEvents_ratechange,
    eDOMEvents_durationchange,
    eDOMEvents_volumechange
#endif
  };

  nsDOMEvent(nsPresContext* aPresContext, nsEvent* aEvent);
  virtual ~nsDOMEvent();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsDOMEvent, nsIDOMEvent)

  
  NS_DECL_NSIDOMEVENT

  
  NS_DECL_NSIDOMNSEVENT

  
  NS_IMETHOD    DuplicatePrivateData();
  NS_IMETHOD    SetTarget(nsIDOMEventTarget* aTarget);
  NS_IMETHOD    SetCurrentTarget(nsIDOMEventTarget* aCurrentTarget);
  NS_IMETHOD    SetOriginalTarget(nsIDOMEventTarget* aOriginalTarget);
  NS_IMETHOD    IsDispatchStopped(PRBool* aIsDispatchStopped);
  NS_IMETHOD    GetInternalNSEvent(nsEvent** aNSEvent);
  NS_IMETHOD    HasOriginalTarget(PRBool* aResult);
  NS_IMETHOD    SetTrusted(PRBool aTrusted);

  static PopupControlState GetEventPopupControlState(nsEvent *aEvent);

  static void PopupAllowedEventsChanged();

  static void Shutdown();

protected:

  
  nsresult SetEventType(const nsAString& aEventTypeArg);
  static const char* GetEventName(PRUint32 aEventType);
  already_AddRefed<nsIDOMEventTarget> GetTargetFromFrame();

  nsEvent*                    mEvent;
  nsCOMPtr<nsPresContext>     mPresContext;
  nsCOMPtr<nsIDOMEventTarget> mTmpRealOriginalTarget;
  nsCOMPtr<nsIDOMEventTarget> mExplicitOriginalTarget;
  PRPackedBool                mEventIsInternal;
  PRPackedBool                mPrivateDataDuplicated;
};

#define NS_FORWARD_TO_NSDOMEVENT \
  NS_FORWARD_NSIDOMEVENT(nsDOMEvent::)

#endif
