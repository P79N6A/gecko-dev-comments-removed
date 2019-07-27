



#include "IccListener.h"

#include "Icc.h"
#include "IccManager.h"
#include "nsIDOMClassInfo.h"
#include "nsIIccInfo.h"
#include "nsRadioInterfaceLayer.h"

using namespace mozilla::dom;

NS_IMPL_ISUPPORTS(IccListener, nsIIccListener)

IccListener::IccListener(IccManager* aIccManager, uint32_t aClientId)
  : mClientId(aClientId)
  , mIccManager(aIccManager)
{
  MOZ_ASSERT(mIccManager);

  mProvider = do_GetService(NS_RILCONTENTHELPER_CONTRACTID);

  if (!mProvider) {
    NS_WARNING("Could not acquire nsIIccProvider!");
    return;
  }

  nsCOMPtr<nsIIccInfo> iccInfo;
  mProvider->GetIccInfo(mClientId, getter_AddRefs(iccInfo));
  if (iccInfo) {
    nsString iccId;
    iccInfo->GetIccid(iccId);
    if (!iccId.IsEmpty()) {
      mIcc = new Icc(mIccManager->GetOwner(), mClientId, iccInfo);
    }
  }

  DebugOnly<nsresult> rv = mProvider->RegisterIccMsg(mClientId, this);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                   "Failed registering icc messages with provider");
}

IccListener::~IccListener()
{
  Shutdown();
}

void
IccListener::Shutdown()
{
  if (mProvider) {
    mProvider->UnregisterIccMsg(mClientId, this);
    mProvider = nullptr;
  }

  if (mIcc) {
    mIcc->Shutdown();
    mIcc = nullptr;
  }

  mIccManager = nullptr;
}



NS_IMETHODIMP
IccListener::NotifyStkCommand(const nsAString& aMessage)
{
  if (!mIcc) {
    return NS_OK;
  }

  return mIcc->NotifyStkEvent(NS_LITERAL_STRING("stkcommand"), aMessage);
}

NS_IMETHODIMP
IccListener::NotifyStkSessionEnd()
{
  if (!mIcc) {
    return NS_OK;
  }

  return mIcc->NotifyEvent(NS_LITERAL_STRING("stksessionend"));
}

NS_IMETHODIMP
IccListener::NotifyCardStateChanged()
{
  if (!mIcc) {
    return NS_OK;
  }

  return mIcc->NotifyEvent(NS_LITERAL_STRING("cardstatechange"));
}

NS_IMETHODIMP
IccListener::NotifyIccInfoChanged()
{
  nsCOMPtr<nsIIccInfo> iccInfo;
  mProvider->GetIccInfo(mClientId, getter_AddRefs(iccInfo));

  
  
  
  
  
  if (!mIcc) {
    if (iccInfo) {
      nsString iccId;
      iccInfo->GetIccid(iccId);
      if (!iccId.IsEmpty()) {
        mIcc = new Icc(mIccManager->GetOwner(), mClientId, iccInfo);
        mIccManager->NotifyIccAdd(iccId);
        mIcc->NotifyEvent(NS_LITERAL_STRING("iccinfochange"));
      }
    }
  } else {
    mIcc->UpdateIccInfo(iccInfo);
    mIcc->NotifyEvent(NS_LITERAL_STRING("iccinfochange"));
    if (!iccInfo) {
      nsString iccId = mIcc->GetIccId();
      mIcc->Shutdown();
      mIcc = nullptr;
      mIccManager->NotifyIccRemove(iccId);
    }
  }

  return NS_OK;
}
