




#include "CellBroadcast.h"
#include "mozilla/dom/MozCellBroadcastBinding.h"
#include "mozilla/dom/MozCellBroadcastEvent.h"
#include "nsIDOMMozCellBroadcastMessage.h"
#include "nsServiceManagerUtils.h"

#define NS_RILCONTENTHELPER_CONTRACTID "@mozilla.org/ril/content-helper;1"

using namespace mozilla::dom;





class CellBroadcast::Listener MOZ_FINAL : public nsICellBroadcastListener
{
private:
  CellBroadcast* mCellBroadcast;

public:
  NS_DECL_ISUPPORTS
  NS_FORWARD_SAFE_NSICELLBROADCASTLISTENER(mCellBroadcast)

  Listener(CellBroadcast* aCellBroadcast)
    : mCellBroadcast(aCellBroadcast)
  {
    MOZ_ASSERT(mCellBroadcast);
  }

  void Disconnect()
  {
    MOZ_ASSERT(mCellBroadcast);
    mCellBroadcast = nullptr;
  }

private:
  ~Listener()
  {
    MOZ_ASSERT(!mCellBroadcast);
  }
};

NS_IMPL_ISUPPORTS(CellBroadcast::Listener, nsICellBroadcastListener)






already_AddRefed<CellBroadcast>
CellBroadcast::Create(nsPIDOMWindow* aWindow, ErrorResult& aRv)
{
  MOZ_ASSERT(aWindow);
  MOZ_ASSERT(aWindow->IsInnerWindow());

  nsCOMPtr<nsICellBroadcastProvider> provider =
    do_GetService(NS_RILCONTENTHELPER_CONTRACTID);
  if (!provider) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  nsRefPtr<CellBroadcast> cb = new CellBroadcast(aWindow, provider);
  return cb.forget();
}

CellBroadcast::CellBroadcast(nsPIDOMWindow *aWindow,
                             nsICellBroadcastProvider *aProvider)
  : DOMEventTargetHelper(aWindow)
  , mProvider(aProvider)
{
  mListener = new Listener(this);
  DebugOnly<nsresult> rv = mProvider->RegisterCellBroadcastMsg(mListener);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                   "Failed registering Cell Broadcast callback with provider");
}

CellBroadcast::~CellBroadcast()
{
  MOZ_ASSERT(mProvider && mListener);

  mListener->Disconnect();
  mProvider->UnregisterCellBroadcastMsg(mListener);
}

NS_IMPL_ISUPPORTS_INHERITED0(CellBroadcast, DOMEventTargetHelper)

JSObject*
CellBroadcast::WrapObject(JSContext* aCx)
{
  return MozCellBroadcastBinding::Wrap(aCx, this);
}



NS_IMETHODIMP
CellBroadcast::NotifyMessageReceived(nsIDOMMozCellBroadcastMessage* aMessage)
{
  MozCellBroadcastEventInit init;
  init.mBubbles = true;
  init.mCancelable = false;
  init.mMessage = aMessage;

  nsRefPtr<MozCellBroadcastEvent> event =
    MozCellBroadcastEvent::Constructor(this, NS_LITERAL_STRING("received"), init);
  return DispatchTrustedEvent(event);
}
