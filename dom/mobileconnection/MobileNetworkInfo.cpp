





#include "mozilla/dom/MobileNetworkInfo.h"

using namespace mozilla::dom;

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(MobileNetworkInfo, mWindow)

NS_IMPL_CYCLE_COLLECTING_ADDREF(MobileNetworkInfo)
NS_IMPL_CYCLE_COLLECTING_RELEASE(MobileNetworkInfo)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(MobileNetworkInfo)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIMobileNetworkInfo)
NS_INTERFACE_MAP_END

MobileNetworkInfo::MobileNetworkInfo(nsPIDOMWindow* aWindow)
  : mWindow(aWindow)
{
}

MobileNetworkInfo::MobileNetworkInfo(const nsAString& aShortName,
                                     const nsAString& aLongName,
                                     const nsAString& aMcc,
                                     const nsAString& aMnc,
                                     const nsAString& aState)
  : mShortName(aShortName)
  , mLongName(aLongName)
  , mMcc(aMcc)
  , mMnc(aMnc)
  , mState(aState)
{
  
  
}

void
MobileNetworkInfo::Update(nsIMobileNetworkInfo* aInfo)
{
  if (!aInfo) {
    return;
  }

  aInfo->GetShortName(mShortName);
  aInfo->GetLongName(mLongName);
  aInfo->GetMcc(mMcc);
  aInfo->GetMnc(mMnc);
  aInfo->GetState(mState);
}

JSObject*
MobileNetworkInfo::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return MozMobileNetworkInfoBinding::Wrap(aCx, this, aGivenProto);
}



NS_IMETHODIMP
MobileNetworkInfo::GetShortName(nsAString& aShortName)
{
  aShortName = mShortName;
  return NS_OK;
}

NS_IMETHODIMP
MobileNetworkInfo::GetLongName(nsAString& aLongName)
{
  aLongName = mLongName;
  return NS_OK;
}

NS_IMETHODIMP
MobileNetworkInfo::GetMcc(nsAString& aMcc)
{
  aMcc = mMcc;
  return NS_OK;
}

NS_IMETHODIMP
MobileNetworkInfo::GetMnc(nsAString& aMnc)
{
  aMnc = mMnc;
  return NS_OK;
}

NS_IMETHODIMP
MobileNetworkInfo::GetState(nsAString& aState)
{
  aState = mState;
  return NS_OK;
}
