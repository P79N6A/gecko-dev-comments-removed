





#include "mozilla/dom/TouchEvent.h"
#include "mozilla/dom/Touch.h"
#include "mozilla/dom/TouchListBinding.h"
#include "mozilla/Preferences.h"
#include "mozilla/TouchEvents.h"
#include "nsContentUtils.h"

namespace mozilla {

#ifdef XP_WIN
namespace widget {
extern int32_t IsTouchDeviceSupportPresent();
} 
#endif 

namespace dom {





NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(TouchList)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_2(TouchList, mParent, mPoints)

NS_IMPL_CYCLE_COLLECTING_ADDREF(TouchList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(TouchList)

JSObject*
TouchList::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return TouchListBinding::Wrap(aCx, aScope, this);
}


bool
TouchList::PrefEnabled(JSContext* aCx, JSObject* aGlobal)
{
  return TouchEvent::PrefEnabled(aCx, aGlobal);
}

Touch*
TouchList::IdentifiedTouch(int32_t aIdentifier) const
{
  for (uint32_t i = 0; i < mPoints.Length(); ++i) {
    Touch* point = mPoints[i];
    if (point && point->Identifier() == aIdentifier) {
      return point;
    }
  }
  return nullptr;
}





TouchEvent::TouchEvent(EventTarget* aOwner,
                       nsPresContext* aPresContext,
                       WidgetTouchEvent* aEvent)
  : UIEvent(aOwner, aPresContext,
            aEvent ? aEvent : new WidgetTouchEvent(false, 0, nullptr))
{
  if (aEvent) {
    mEventIsInternal = false;

    for (uint32_t i = 0; i < aEvent->touches.Length(); ++i) {
      Touch* touch = aEvent->touches[i];
      touch->InitializePoints(mPresContext, aEvent);
    }
  } else {
    mEventIsInternal = true;
    mEvent->time = PR_Now();
  }
}

NS_IMPL_CYCLE_COLLECTION_INHERITED_3(TouchEvent, UIEvent,
                                     mTouches,
                                     mTargetTouches,
                                     mChangedTouches)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(TouchEvent)
NS_INTERFACE_MAP_END_INHERITING(UIEvent)

NS_IMPL_ADDREF_INHERITED(TouchEvent, UIEvent)
NS_IMPL_RELEASE_INHERITED(TouchEvent, UIEvent)

void
TouchEvent::InitTouchEvent(const nsAString& aType,
                           bool aCanBubble,
                           bool aCancelable,
                           nsIDOMWindow* aView,
                           int32_t aDetail,
                           bool aCtrlKey,
                           bool aAltKey,
                           bool aShiftKey,
                           bool aMetaKey,
                           TouchList* aTouches,
                           TouchList* aTargetTouches,
                           TouchList* aChangedTouches,
                           ErrorResult& aRv)
{
  aRv = UIEvent::InitUIEvent(aType, aCanBubble, aCancelable, aView, aDetail);
  if (aRv.Failed()) {
    return;
  }

  mEvent->AsInputEvent()->InitBasicModifiers(aCtrlKey, aAltKey,
                                             aShiftKey, aMetaKey);
  mTouches = aTouches;
  mTargetTouches = aTargetTouches;
  mChangedTouches = aChangedTouches;
}

TouchList*
TouchEvent::Touches()
{
  if (!mTouches) {
    WidgetTouchEvent* touchEvent = mEvent->AsTouchEvent();
    if (mEvent->message == NS_TOUCH_END || mEvent->message == NS_TOUCH_CANCEL) {
      
      nsTArray< nsRefPtr<Touch> > unchangedTouches;
      const nsTArray< nsRefPtr<Touch> >& touches = touchEvent->touches;
      for (uint32_t i = 0; i < touches.Length(); ++i) {
        if (!touches[i]->mChanged) {
          unchangedTouches.AppendElement(touches[i]);
        }
      }
      mTouches = new TouchList(ToSupports(this), unchangedTouches);
    } else {
      mTouches = new TouchList(ToSupports(this), touchEvent->touches);
    }
  }
  return mTouches;
}

TouchList*
TouchEvent::TargetTouches()
{
  if (!mTargetTouches) {
    nsTArray< nsRefPtr<Touch> > targetTouches;
    WidgetTouchEvent* touchEvent = mEvent->AsTouchEvent();
    const nsTArray< nsRefPtr<Touch> >& touches = touchEvent->touches;
    for (uint32_t i = 0; i < touches.Length(); ++i) {
      
      
      if ((mEvent->message != NS_TOUCH_END &&
           mEvent->message != NS_TOUCH_CANCEL) || !touches[i]->mChanged) {
        if (touches[i]->mTarget == mEvent->originalTarget) {
          targetTouches.AppendElement(touches[i]);
        }
      }
    }
    mTargetTouches = new TouchList(ToSupports(this), targetTouches);
  }
  return mTargetTouches;
}

TouchList*
TouchEvent::ChangedTouches()
{
  if (!mChangedTouches) {
    nsTArray< nsRefPtr<Touch> > changedTouches;
    WidgetTouchEvent* touchEvent = mEvent->AsTouchEvent();
    const nsTArray< nsRefPtr<Touch> >& touches = touchEvent->touches;
    for (uint32_t i = 0; i < touches.Length(); ++i) {
      if (touches[i]->mChanged) {
        changedTouches.AppendElement(touches[i]);
      }
    }
    mChangedTouches = new TouchList(ToSupports(this), changedTouches);
  }
  return mChangedTouches;
}


bool
TouchEvent::PrefEnabled(JSContext* aCx, JSObject* aGlobal)
{
  bool prefValue = false;
  int32_t flag = 0;
  if (NS_SUCCEEDED(Preferences::GetInt("dom.w3c_touch_events.enabled",
                                        &flag))) {
    if (flag == 2) {
#ifdef XP_WIN
      static bool sDidCheckTouchDeviceSupport = false;
      static bool sIsTouchDeviceSupportPresent = false;
      
      if (!sDidCheckTouchDeviceSupport) {
        sDidCheckTouchDeviceSupport = true;
        sIsTouchDeviceSupportPresent = widget::IsTouchDeviceSupportPresent();
      }
      prefValue = sIsTouchDeviceSupportPresent;
#else
      NS_WARNING("dom.w3c_touch_events.enabled=2 not implemented!");
      prefValue = false;
#endif
    } else {
      prefValue = !!flag;
    }
  }
  if (prefValue) {
    nsContentUtils::InitializeTouchEventTable();
  }
  return prefValue;
}

bool
TouchEvent::AltKey()
{
  return mEvent->AsTouchEvent()->IsAlt();
}

bool
TouchEvent::MetaKey()
{
  return mEvent->AsTouchEvent()->IsMeta();
}

bool
TouchEvent::CtrlKey()
{
  return mEvent->AsTouchEvent()->IsControl();
}

bool
TouchEvent::ShiftKey()
{
  return mEvent->AsTouchEvent()->IsShift();
}

} 
} 

using namespace mozilla;
using namespace mozilla::dom;

nsresult
NS_NewDOMTouchEvent(nsIDOMEvent** aInstancePtrResult,
                    EventTarget* aOwner,
                    nsPresContext* aPresContext,
                    WidgetTouchEvent* aEvent)
{
  TouchEvent* it = new TouchEvent(aOwner, aPresContext, aEvent);
  return CallQueryInterface(it, aInstancePtrResult);
}
