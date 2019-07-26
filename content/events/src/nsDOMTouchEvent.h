



#ifndef nsDOMTouchEvent_h_
#define nsDOMTouchEvent_h_

#include "nsDOMUIEvent.h"
#include "nsIDOMTouchEvent.h"
#include "nsString.h"
#include "nsTArray.h"
#include "mozilla/Attributes.h"
#include "nsJSEnvironment.h"
#include "mozilla/dom/TouchEventBinding.h"
#include "nsWrapperCache.h"

class nsDOMTouchList MOZ_FINAL : public nsIDOMTouchList
                               , public nsWrapperCache
{
  typedef mozilla::dom::Touch Touch;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsDOMTouchList)
  NS_DECL_NSIDOMTOUCHLIST

  nsDOMTouchList(nsISupports* aParent)
    : mParent(aParent)
  {
    SetIsDOMBinding();
    nsJSContext::LikelyShortLivingObjectCreated();
  }
  nsDOMTouchList(nsISupports* aParent,
                 const nsTArray< nsRefPtr<Touch> >& aTouches)
    : mParent(aParent)
    , mPoints(aTouches)
  {
    SetIsDOMBinding();
    nsJSContext::LikelyShortLivingObjectCreated();
  }

  void Append(Touch* aPoint)
  {
    mPoints.AppendElement(aPoint);
  }

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  nsISupports* GetParentObject() const
  {
    return mParent;
  }

  static bool PrefEnabled();

  uint32_t Length() const
  {
    return mPoints.Length();
  }
  Touch* Item(uint32_t aIndex) const
  {
    return mPoints.SafeElementAt(aIndex);
  }
  Touch* IndexedGetter(uint32_t aIndex, bool& aFound) const
  {
    aFound = aIndex < mPoints.Length();
    if (!aFound) {
      return nullptr;
    }
    return mPoints[aIndex];
  }
  Touch* IdentifiedTouch(int32_t aIdentifier) const;

protected:
  nsCOMPtr<nsISupports> mParent;
  nsTArray< nsRefPtr<Touch> > mPoints;
};

class nsDOMTouchEvent : public nsDOMUIEvent,
                        public nsIDOMTouchEvent
{
public:
  nsDOMTouchEvent(mozilla::dom::EventTarget* aOwner,
                  nsPresContext* aPresContext, nsTouchEvent* aEvent);
  virtual ~nsDOMTouchEvent();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsDOMTouchEvent, nsDOMUIEvent)
  NS_DECL_NSIDOMTOUCHEVENT

  NS_FORWARD_TO_NSDOMUIEVENT

  virtual JSObject* WrapObject(JSContext* aCx,
			       JS::Handle<JSObject*> aScope) MOZ_OVERRIDE
  {
    return mozilla::dom::TouchEventBinding::Wrap(aCx, aScope, this);
  }

  nsDOMTouchList* Touches();
  nsDOMTouchList* TargetTouches();
  nsDOMTouchList* ChangedTouches();

  bool AltKey()
  {
    return static_cast<nsInputEvent*>(mEvent)->IsAlt();
  }

  bool MetaKey()
  {
    return static_cast<nsInputEvent*>(mEvent)->IsMeta();
  }

  bool CtrlKey()
  {
    return static_cast<nsInputEvent*>(mEvent)->IsControl();
  }

  bool ShiftKey()
  {
    return static_cast<nsInputEvent*>(mEvent)->IsShift();
  }

  void InitTouchEvent(const nsAString& aType,
                      bool aCanBubble,
                      bool aCancelable,
                      nsIDOMWindow* aView,
                      int32_t aDetail,
                      bool aCtrlKey,
                      bool aAltKey,
                      bool aShiftKey,
                      bool aMetaKey,
                      nsIDOMTouchList* aTouches,
                      nsIDOMTouchList* aTargetTouches,
                      nsIDOMTouchList* aChangedTouches,
                      mozilla::ErrorResult& aRv)
  {
    aRv = InitTouchEvent(aType, aCanBubble, aCancelable, aView, aDetail,
                         aCtrlKey, aAltKey, aShiftKey, aMetaKey,
                         aTouches, aTargetTouches, aChangedTouches);
  }

  static bool PrefEnabled();
protected:
  nsRefPtr<nsDOMTouchList> mTouches;
  nsRefPtr<nsDOMTouchList> mTargetTouches;
  nsRefPtr<nsDOMTouchList> mChangedTouches;
};

#endif 
