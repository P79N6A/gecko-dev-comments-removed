





#include "IccIPCService.h"

#include "mozilla/dom/ContentChild.h"
#include "mozilla/Preferences.h"

namespace mozilla {
namespace dom {
namespace icc {

NS_IMPL_ISUPPORTS(IccIPCService, nsIIccService)

IccIPCService::IccIPCService()
{
  int32_t numRil = Preferences::GetInt("ril.numRadioInterfaces", 1);
  mIccs.SetLength(numRil);
}

IccIPCService::~IccIPCService()
{
  uint32_t count = mIccs.Length();
  for (uint32_t i = 0; i < count; i++) {
    if (mIccs[i]) {
      mIccs[i]->Shutdown();
    }
  }
}

NS_IMETHODIMP
IccIPCService::GetIccByServiceId(uint32_t aServiceId, nsIIcc** aIcc)
{
  NS_ENSURE_TRUE(aServiceId < mIccs.Length(), NS_ERROR_INVALID_ARG);

  if (!mIccs[aServiceId]) {
    nsRefPtr<IccChild> child = new IccChild();

    
    
    ContentChild::GetSingleton()->SendPIccConstructor(child, aServiceId);
    child->Init();

    mIccs[aServiceId] = child;
  }

  nsCOMPtr<nsIIcc> icc(mIccs[aServiceId]);
  icc.forget(aIcc);

  return NS_OK;
}

} 
} 
} 
