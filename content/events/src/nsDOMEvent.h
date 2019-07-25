




































#ifndef nsDOMEvent_h__
#define nsDOMEvent_h__

#include "nsIDOMEvent.h"
#include "nsIDOMNSEvent.h"
#include "nsISupports.h"
#include "nsIPrivateDOMEvent.h"
#include "nsCOMPtr.h"
#include "nsIDOMEventTarget.h"
#include "nsPIDOMWindow.h"
#include "nsPoint.h"
#include "nsGUIEvent.h"
#include "nsCycleCollectionParticipant.h"
#include "nsAutoPtr.h"

class nsIContent;
class nsPresContext;
 
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
    eDOMEvents_MozMouseHittest,
    eDOMEvents_mousemove,
    eDOMEvents_contextmenu,
    eDOMEvents_keydown,
    eDOMEvents_keyup,
    eDOMEvents_keypress,
    eDOMEvents_focus,
    eDOMEvents_blur,
    eDOMEvents_load,
    eDOMEvents_popstate,
    eDOMEvents_beforescriptexecute,
    eDOMEvents_afterscriptexecute,
    eDOMEvents_beforeunload,
    eDOMEvents_unload,
    eDOMEvents_hashchange,
    eDOMEvents_readystatechange,
    eDOMEvents_abort,
    eDOMEvents_error,
    eDOMEvents_submit,
    eDOMEvents_reset,
    eDOMEvents_change,
    eDOMEvents_select,
    eDOMEvents_input,
    eDOMEvents_invalid,
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
    eDOMEvents_dragstart,
    eDOMEvents_dragleave,
    eDOMEvents_drop,
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
    eDOMEvents_MozMousePixelScroll,
    eDOMEvents_offline,
    eDOMEvents_online,
    eDOMEvents_copy,
    eDOMEvents_cut,
    eDOMEvents_paste,
#ifdef MOZ_SVG
    eDOMEvents_SVGLoad,
    eDOMEvents_SVGUnload,
    eDOMEvents_SVGAbort,
    eDOMEvents_SVGError,
    eDOMEvents_SVGResize,
    eDOMEvents_SVGScroll,
    eDOMEvents_SVGZoom,
#endif 
#ifdef MOZ_SMIL
    eDOMEvents_beginEvent,
    eDOMEvents_endEvent,
    eDOMEvents_repeatEvent,
#endif 
#ifdef MOZ_MEDIA
    eDOMEvents_loadstart,
    eDOMEvents_progress,
    eDOMEvents_suspend,
    eDOMEvents_emptied,
    eDOMEvents_stalled,
    eDOMEvents_play,
    eDOMEvents_pause,
    eDOMEvents_loadedmetadata,
    eDOMEvents_loadeddata,
    eDOMEvents_waiting,
    eDOMEvents_playing,
    eDOMEvents_canplay,
    eDOMEvents_canplaythrough,
    eDOMEvents_seeking,
    eDOMEvents_seeked,
    eDOMEvents_timeupdate,
    eDOMEvents_ended,
    eDOMEvents_ratechange,
    eDOMEvents_durationchange,
    eDOMEvents_volumechange,
    eDOMEvents_mozaudioavailable,
#endif
    eDOMEvents_afterpaint,
    eDOMEvents_beforepaint,
    eDOMEvents_beforeresize,
    eDOMEvents_MozSwipeGesture,
    eDOMEvents_MozMagnifyGestureStart,
    eDOMEvents_MozMagnifyGestureUpdate,
    eDOMEvents_MozMagnifyGesture,
    eDOMEvents_MozRotateGestureStart,
    eDOMEvents_MozRotateGestureUpdate,
    eDOMEvents_MozRotateGesture,
    eDOMEvents_MozTapGesture,
    eDOMEvents_MozPressTapGesture,
    eDOMEvents_MozTouchDown,
    eDOMEvents_MozTouchMove,
    eDOMEvents_MozTouchUp,
    eDOMEvents_MozScrolledAreaChanged,
    eDOMEvents_transitionend,
    eDOMEvents_animationstart,
    eDOMEvents_animationend,
    eDOMEvents_animationiteration
  };

  nsDOMEvent(nsPresContext* aPresContext, nsEvent* aEvent);
  virtual ~nsDOMEvent();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsDOMEvent, nsIDOMEvent)

  
  NS_DECL_NSIDOMEVENT

  
  NS_DECL_NSIDOMNSEVENT

  
  NS_IMETHOD    DuplicatePrivateData();
  NS_IMETHOD    SetTarget(nsIDOMEventTarget* aTarget);
  NS_IMETHOD_(PRBool)    IsDispatchStopped();
  NS_IMETHOD_(nsEvent*)    GetInternalNSEvent();
  NS_IMETHOD    SetTrusted(PRBool aTrusted);

  virtual void Serialize(IPC::Message* aMsg, PRBool aSerializeInterfaceType);
  virtual PRBool Deserialize(const IPC::Message* aMsg, void** aIter);

  static PopupControlState GetEventPopupControlState(nsEvent *aEvent);

  static void PopupAllowedEventsChanged();

  static void Shutdown();

  static const char* GetEventName(PRUint32 aEventType);
protected:

  
  nsresult SetEventType(const nsAString& aEventTypeArg);
  already_AddRefed<nsIContent> GetTargetFromFrame();

  nsEvent*                    mEvent;
  nsRefPtr<nsPresContext>     mPresContext;
  nsCOMPtr<nsIDOMEventTarget> mTmpRealOriginalTarget;
  nsIDOMEventTarget*          mExplicitOriginalTarget;
  nsString                    mCachedType;
  PRPackedBool                mEventIsInternal;
  PRPackedBool                mPrivateDataDuplicated;
};

#define NS_FORWARD_TO_NSDOMEVENT \
  NS_FORWARD_NSIDOMEVENT(nsDOMEvent::)

#endif
