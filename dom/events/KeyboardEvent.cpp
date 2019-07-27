




#include "mozilla/dom/KeyboardEvent.h"
#include "mozilla/TextEvents.h"
#include "prtime.h"

namespace mozilla {
namespace dom {

KeyboardEvent::KeyboardEvent(EventTarget* aOwner,
                             nsPresContext* aPresContext,
                             WidgetKeyboardEvent* aEvent)
  : UIEvent(aOwner, aPresContext,
            aEvent ? aEvent : new WidgetKeyboardEvent(false, 0, nullptr))
  , mInitializedByCtor(false)
  , mInitializedWhichValue(0)
{
  if (aEvent) {
    mEventIsInternal = false;
  }
  else {
    mEventIsInternal = true;
    mEvent->time = PR_Now();
    mEvent->AsKeyboardEvent()->mKeyNameIndex = KEY_NAME_INDEX_USE_STRING;
  }
}

NS_IMPL_ADDREF_INHERITED(KeyboardEvent, UIEvent)
NS_IMPL_RELEASE_INHERITED(KeyboardEvent, UIEvent)

NS_INTERFACE_MAP_BEGIN(KeyboardEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMKeyEvent)
NS_INTERFACE_MAP_END_INHERITING(UIEvent)

bool
KeyboardEvent::AltKey()
{
  return mEvent->AsKeyboardEvent()->IsAlt();
}

NS_IMETHODIMP
KeyboardEvent::GetAltKey(bool* aIsDown)
{
  NS_ENSURE_ARG_POINTER(aIsDown);
  *aIsDown = AltKey();
  return NS_OK;
}

bool
KeyboardEvent::CtrlKey()
{
  return mEvent->AsKeyboardEvent()->IsControl();
}

NS_IMETHODIMP
KeyboardEvent::GetCtrlKey(bool* aIsDown)
{
  NS_ENSURE_ARG_POINTER(aIsDown);
  *aIsDown = CtrlKey();
  return NS_OK;
}

bool
KeyboardEvent::ShiftKey()
{
  return mEvent->AsKeyboardEvent()->IsShift();
}

NS_IMETHODIMP
KeyboardEvent::GetShiftKey(bool* aIsDown)
{
  NS_ENSURE_ARG_POINTER(aIsDown);
  *aIsDown = ShiftKey();
  return NS_OK;
}

bool
KeyboardEvent::MetaKey()
{
  return mEvent->AsKeyboardEvent()->IsMeta();
}

NS_IMETHODIMP
KeyboardEvent::GetMetaKey(bool* aIsDown)
{
  NS_ENSURE_ARG_POINTER(aIsDown);
  *aIsDown = MetaKey();
  return NS_OK;
}

bool
KeyboardEvent::Repeat()
{
  return mEvent->AsKeyboardEvent()->mIsRepeat;
}

NS_IMETHODIMP
KeyboardEvent::GetRepeat(bool* aIsRepeat)
{
  NS_ENSURE_ARG_POINTER(aIsRepeat);
  *aIsRepeat = Repeat();
  return NS_OK;
}

bool
KeyboardEvent::IsComposing()
{
  return mEvent->AsKeyboardEvent()->mIsComposing;
}

NS_IMETHODIMP
KeyboardEvent::GetModifierState(const nsAString& aKey,
                                bool* aState)
{
  NS_ENSURE_ARG_POINTER(aState);

  *aState = GetModifierState(aKey);
  return NS_OK;
}

NS_IMETHODIMP
KeyboardEvent::GetKey(nsAString& aKeyName)
{
  mEvent->AsKeyboardEvent()->GetDOMKeyName(aKeyName);
  return NS_OK;
}

void
KeyboardEvent::GetCode(nsAString& aCodeName)
{
  mEvent->AsKeyboardEvent()->GetDOMCodeName(aCodeName);
}

NS_IMETHODIMP
KeyboardEvent::GetCharCode(uint32_t* aCharCode)
{
  NS_ENSURE_ARG_POINTER(aCharCode);
  *aCharCode = CharCode();
  return NS_OK;
}

uint32_t
KeyboardEvent::CharCode()
{
  
  if (mInitializedByCtor) {
    return mEvent->AsKeyboardEvent()->charCode;
  }

  switch (mEvent->message) {
  case NS_KEY_BEFORE_DOWN:
  case NS_KEY_DOWN:
  case NS_KEY_AFTER_DOWN:
  case NS_KEY_BEFORE_UP:
  case NS_KEY_UP:
  case NS_KEY_AFTER_UP:
    return 0;
  case NS_KEY_PRESS:
    return mEvent->AsKeyboardEvent()->charCode;
  }
  return 0;
}

NS_IMETHODIMP
KeyboardEvent::GetKeyCode(uint32_t* aKeyCode)
{
  NS_ENSURE_ARG_POINTER(aKeyCode);
  *aKeyCode = KeyCode();
  return NS_OK;
}

uint32_t
KeyboardEvent::KeyCode()
{
  
  if (mInitializedByCtor) {
    return mEvent->AsKeyboardEvent()->keyCode;
  }

  if (mEvent->HasKeyEventMessage()) {
    return mEvent->AsKeyboardEvent()->keyCode;
  }
  return 0;
}

uint32_t
KeyboardEvent::Which()
{
  
  if (mInitializedByCtor) {
    return mInitializedWhichValue;
  }

  switch (mEvent->message) {
    case NS_KEY_BEFORE_DOWN:
    case NS_KEY_DOWN:
    case NS_KEY_AFTER_DOWN:
    case NS_KEY_BEFORE_UP:
    case NS_KEY_UP:
    case NS_KEY_AFTER_UP:
      return KeyCode();
    case NS_KEY_PRESS:
      
      
      {
        uint32_t keyCode = mEvent->AsKeyboardEvent()->keyCode;
        if (keyCode == NS_VK_RETURN || keyCode == NS_VK_BACK) {
          return keyCode;
        }
        return CharCode();
      }
  }

  return 0;
}

NS_IMETHODIMP
KeyboardEvent::GetLocation(uint32_t* aLocation)
{
  NS_ENSURE_ARG_POINTER(aLocation);

  *aLocation = Location();
  return NS_OK;
}

uint32_t
KeyboardEvent::Location()
{
  return mEvent->AsKeyboardEvent()->location;
}


already_AddRefed<KeyboardEvent>
KeyboardEvent::Constructor(const GlobalObject& aGlobal,
                           const nsAString& aType,
                           const KeyboardEventInit& aParam,
                           ErrorResult& aRv)
{
  nsCOMPtr<EventTarget> target = do_QueryInterface(aGlobal.GetAsSupports());
  nsRefPtr<KeyboardEvent> newEvent =
    new KeyboardEvent(target, nullptr, nullptr);
  newEvent->InitWithKeyboardEventInit(target, aType, aParam, aRv);

  return newEvent.forget();
}

void
KeyboardEvent::InitWithKeyboardEventInit(EventTarget* aOwner,
                                         const nsAString& aType,
                                         const KeyboardEventInit& aParam,
                                         ErrorResult& aRv)
{
  bool trusted = Init(aOwner);
  aRv = InitKeyEvent(aType, aParam.mBubbles, aParam.mCancelable,
                     aParam.mView, false, false, false, false,
                     aParam.mKeyCode, aParam.mCharCode);
  InitModifiers(aParam);
  SetTrusted(trusted);
  mDetail = aParam.mDetail;
  mInitializedByCtor = true;
  mInitializedWhichValue = aParam.mWhich;

  WidgetKeyboardEvent* internalEvent = mEvent->AsKeyboardEvent();
  internalEvent->location = aParam.mLocation;
  internalEvent->mIsRepeat = aParam.mRepeat;
  internalEvent->mIsComposing = aParam.mIsComposing;
  internalEvent->mKeyNameIndex =
    WidgetKeyboardEvent::GetKeyNameIndex(aParam.mKey);
  if (internalEvent->mKeyNameIndex == KEY_NAME_INDEX_USE_STRING) {
    internalEvent->mKeyValue = aParam.mKey;
  }
  internalEvent->mCodeNameIndex =
    WidgetKeyboardEvent::GetCodeNameIndex(aParam.mCode);
  if (internalEvent->mCodeNameIndex == CODE_NAME_INDEX_USE_STRING) {
    internalEvent->mCodeValue = aParam.mCode;
  }
}

NS_IMETHODIMP
KeyboardEvent::InitKeyEvent(const nsAString& aType,
                            bool aCanBubble,
                            bool aCancelable,
                            nsIDOMWindow* aView,
                            bool aCtrlKey,
                            bool aAltKey,
                            bool aShiftKey,
                            bool aMetaKey,
                            uint32_t aKeyCode,
                            uint32_t aCharCode)
{
  nsresult rv = UIEvent::InitUIEvent(aType, aCanBubble, aCancelable, aView, 0);
  NS_ENSURE_SUCCESS(rv, rv);

  WidgetKeyboardEvent* keyEvent = mEvent->AsKeyboardEvent();
  keyEvent->InitBasicModifiers(aCtrlKey, aAltKey, aShiftKey, aMetaKey);
  keyEvent->keyCode = aKeyCode;
  keyEvent->charCode = aCharCode;

  return NS_OK;
}

} 
} 

using namespace mozilla;
using namespace mozilla::dom;

nsresult
NS_NewDOMKeyboardEvent(nsIDOMEvent** aInstancePtrResult,
                       EventTarget* aOwner,
                       nsPresContext* aPresContext,
                       WidgetKeyboardEvent* aEvent)
{
  KeyboardEvent* it = new KeyboardEvent(aOwner, aPresContext, aEvent);
  NS_ADDREF(it);
  *aInstancePtrResult = static_cast<Event*>(it);
  return NS_OK;
}
