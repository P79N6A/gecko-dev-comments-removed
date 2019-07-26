





#include "nsDOMTouchEvent.h"
#include "nsContentUtils.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/Touch.h"
#include "mozilla/dom/TouchListBinding.h"
#include "mozilla/TouchEvents.h"

using namespace mozilla;
using namespace mozilla::dom;



NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsDOMTouchList)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_2(nsDOMTouchList, mParent, mPoints)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsDOMTouchList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsDOMTouchList)

 JSObject*
nsDOMTouchList::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return TouchListBinding::Wrap(aCx, aScope, this);
}

 bool
nsDOMTouchList::PrefEnabled()
{
  return nsDOMTouchEvent::PrefEnabled();
}

Touch*
nsDOMTouchList::IdentifiedTouch(int32_t aIdentifier) const
{
  for (uint32_t i = 0; i < mPoints.Length(); ++i) {
    Touch* point = mPoints[i];
    if (point && point->Identifier() == aIdentifier) {
      return point;
    }
  }
  return nullptr;
}



nsDOMTouchEvent::nsDOMTouchEvent(mozilla::dom::EventTarget* aOwner,
                                 nsPresContext* aPresContext,
                                 WidgetTouchEvent* aEvent)
  : nsDOMUIEvent(aOwner, aPresContext,
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

NS_IMPL_CYCLE_COLLECTION_INHERITED_3(nsDOMTouchEvent, nsDOMUIEvent,
                                     mTouches,
                                     mTargetTouches,
                                     mChangedTouches)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsDOMTouchEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMUIEvent)

NS_IMPL_ADDREF_INHERITED(nsDOMTouchEvent, nsDOMUIEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMTouchEvent, nsDOMUIEvent)


void
nsDOMTouchEvent::InitTouchEvent(const nsAString& aType,
                                bool aCanBubble,
                                bool aCancelable,
                                nsIDOMWindow* aView,
                                int32_t aDetail,
                                bool aCtrlKey,
                                bool aAltKey,
                                bool aShiftKey,
                                bool aMetaKey,
                                nsDOMTouchList* aTouches,
                                nsDOMTouchList* aTargetTouches,
                                nsDOMTouchList* aChangedTouches,
                                mozilla::ErrorResult& aRv)
{
  aRv = nsDOMUIEvent::InitUIEvent(aType,
                                  aCanBubble,
                                  aCancelable,
                                  aView,
                                  aDetail);
  if (aRv.Failed()) {
    return;
  }

  static_cast<WidgetInputEvent*>(mEvent)->
    InitBasicModifiers(aCtrlKey, aAltKey, aShiftKey, aMetaKey);
  mTouches = aTouches;
  mTargetTouches = aTargetTouches;
  mChangedTouches = aChangedTouches;
}

nsDOMTouchList*
nsDOMTouchEvent::Touches()
{
  if (!mTouches) {
    WidgetTouchEvent* touchEvent = static_cast<WidgetTouchEvent*>(mEvent);
    if (mEvent->message == NS_TOUCH_END || mEvent->message == NS_TOUCH_CANCEL) {
      
      nsTArray< nsRefPtr<Touch> > unchangedTouches;
      const nsTArray< nsRefPtr<Touch> >& touches = touchEvent->touches;
      for (uint32_t i = 0; i < touches.Length(); ++i) {
        if (!touches[i]->mChanged) {
          unchangedTouches.AppendElement(touches[i]);
        }
      }
      mTouches = new nsDOMTouchList(ToSupports(this), unchangedTouches);
    } else {
      mTouches = new nsDOMTouchList(ToSupports(this), touchEvent->touches);
    }
  }
  return mTouches;
}

nsDOMTouchList*
nsDOMTouchEvent::TargetTouches()
{
  if (!mTargetTouches) {
    nsTArray< nsRefPtr<Touch> > targetTouches;
    WidgetTouchEvent* touchEvent = static_cast<WidgetTouchEvent*>(mEvent);
    const nsTArray< nsRefPtr<Touch> >& touches = touchEvent->touches;
    for (uint32_t i = 0; i < touches.Length(); ++i) {
      
      
      if ((mEvent->message != NS_TOUCH_END &&
           mEvent->message != NS_TOUCH_CANCEL) || !touches[i]->mChanged) {
        if (touches[i]->mTarget == mEvent->originalTarget) {
          targetTouches.AppendElement(touches[i]);
        }
      }
    }
    mTargetTouches = new nsDOMTouchList(ToSupports(this), targetTouches);
  }
  return mTargetTouches;
}

nsDOMTouchList*
nsDOMTouchEvent::ChangedTouches()
{
  if (!mChangedTouches) {
    nsTArray< nsRefPtr<Touch> > changedTouches;
    WidgetTouchEvent* touchEvent = static_cast<WidgetTouchEvent*>(mEvent);
    const nsTArray< nsRefPtr<Touch> >& touches = touchEvent->touches;
    for (uint32_t i = 0; i < touches.Length(); ++i) {
      if (touches[i]->mChanged) {
        changedTouches.AppendElement(touches[i]);
      }
    }
    mChangedTouches = new nsDOMTouchList(ToSupports(this), changedTouches);
  }
  return mChangedTouches;
}

#ifdef XP_WIN
namespace mozilla {
namespace widget {
extern int32_t IsTouchDeviceSupportPresent();
} }
#endif

bool
nsDOMTouchEvent::PrefEnabled()
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
        sIsTouchDeviceSupportPresent = mozilla::widget::IsTouchDeviceSupportPresent();
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

nsresult
NS_NewDOMTouchEvent(nsIDOMEvent** aInstancePtrResult,
                    mozilla::dom::EventTarget* aOwner,
                    nsPresContext* aPresContext,
                    WidgetTouchEvent* aEvent)
{
  nsDOMTouchEvent* it = new nsDOMTouchEvent(aOwner, aPresContext, aEvent);
  return CallQueryInterface(it, aInstancePtrResult);
}
