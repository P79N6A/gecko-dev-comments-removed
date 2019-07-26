



#include "IccManager.h"

#include "GeneratedEvents.h"
#include "Icc.h"
#include "IccListener.h"
#include "mozilla/dom/IccChangeEvent.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "nsIDOMClassInfo.h"
#include "nsIDOMIccInfo.h"

using namespace mozilla::dom;

DOMCI_DATA(MozIccManager, IccManager)

NS_IMPL_CYCLE_COLLECTION_CLASS(IccManager)

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(IccManager,
                                               nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mJsIccIds)
  
  
  
  
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(IccManager,
                                                  nsDOMEventTargetHelper)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(IccManager,
                                                nsDOMEventTargetHelper)
  tmp->Unroot();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(IccManager)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMozIccManager)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MozIccManager)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(IccManager, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(IccManager, nsDOMEventTargetHelper)

IccManager::IccManager(nsPIDOMWindow* aWindow)
  : mJsIccIds(nullptr)
  , mRooted(false)
{
  BindToOwner(aWindow);

  uint32_t numberOfServices =
    mozilla::Preferences::GetUint("ril.numRadioInterfaces", 1);

  for (uint32_t i = 0; i < numberOfServices; i++) {
    nsRefPtr<IccListener> iccListener = new IccListener(this, i);
    mIccListeners.AppendElement(iccListener);
  }
}

IccManager::~IccManager()
{
  Shutdown();
  Unroot();
}

void
IccManager::Shutdown()
{
  for (uint32_t i = 0; i < mIccListeners.Length(); i++) {
    mIccListeners[i]->Shutdown();
    mIccListeners[i] = nullptr;
  }
  mIccListeners.Clear();
}

nsresult
IccManager::NotifyIccAdd(const nsAString& aIccId)
{
  mJsIccIds = nullptr;

  IccChangeEventInit init;
  init.mBubbles = false;
  init.mCancelable = false;
  init.mIccId = aIccId;

  nsRefPtr<IccChangeEvent> event =
    IccChangeEvent::Constructor(this, NS_LITERAL_STRING("iccdetected"), init);

  return DispatchTrustedEvent(event);
}

nsresult
IccManager::NotifyIccRemove(const nsAString& aIccId)
{
  mJsIccIds = nullptr;

  IccChangeEventInit init;
  init.mBubbles = false;
  init.mCancelable = false;
  init.mIccId = aIccId;

  nsRefPtr<IccChangeEvent> event =
    IccChangeEvent::Constructor(this, NS_LITERAL_STRING("iccundetected"), init);

  return DispatchTrustedEvent(event);
}

void
IccManager::Root()
{
  if (!mRooted) {
    mozilla::HoldJSObjects(this);
    mRooted = true;
  }
}

void
IccManager::Unroot()
{
  if (mRooted) {
    mJsIccIds = nullptr;
    mozilla::DropJSObjects(this);
    mRooted = false;
  }
}



NS_IMETHODIMP
IccManager::GetIccIds(JS::Value* aIccIds)
{
  if (!mJsIccIds) {
    nsTArray<nsString> iccIds;
    for (uint32_t i = 0; i < mIccListeners.Length(); i++) {
      nsRefPtr<Icc> icc = mIccListeners[i]->GetIcc();
      if (icc) {
        iccIds.AppendElement(icc->GetIccId());
      }
    }

    nsresult rv;
    nsIScriptContext* sc = GetContextForEventHandlers(&rv);
    NS_ENSURE_SUCCESS(rv, rv);

    AutoPushJSContext cx(sc->GetNativeContext());
    JS::Rooted<JSObject*> jsIccIds(cx);
    rv = nsTArrayToJSArray(cx, iccIds, jsIccIds.address());
    NS_ENSURE_SUCCESS(rv, rv);

    mJsIccIds = jsIccIds;
    Root();
  }

  aIccIds->setObject(*mJsIccIds);
  return NS_OK;
}

NS_IMETHODIMP
IccManager::GetIccById(const nsAString& aIccId, nsISupports** aIcc)
{
  *aIcc = nullptr;

  for (uint32_t i = 0; i < mIccListeners.Length(); i++) {
    nsRefPtr<Icc> icc = mIccListeners[i]->GetIcc();
    if (icc && aIccId == icc->GetIccId()) {
      icc.forget(aIcc);
      return NS_OK;
    }
  }

  return NS_OK;
}

NS_IMPL_EVENT_HANDLER(IccManager, iccdetected)
NS_IMPL_EVENT_HANDLER(IccManager, iccundetected)
