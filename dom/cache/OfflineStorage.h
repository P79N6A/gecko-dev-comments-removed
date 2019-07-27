





#ifndef mozilla_dom_cache_QuotaOfflineStorage_h
#define mozilla_dom_cache_QuotaOfflineStorage_h

#include "nsISupportsImpl.h"
#include "mozilla/AlreadyAddRefed.h"
#include "mozilla/dom/cache/Context.h"
#include "nsIOfflineStorage.h"
#include "nsTArray.h"

namespace mozilla {
namespace dom {
namespace cache {

class OfflineStorage final : public nsIOfflineStorage
{
public:
  static already_AddRefed<OfflineStorage>
  Register(Context::ThreadsafeHandle* aContext, const QuotaInfo& aQuotaInfo);

  void
  AddDestroyCallback(nsIRunnable* aCallback);

private:
  OfflineStorage(Context::ThreadsafeHandle* aContext,
                 const QuotaInfo& aQuotaInfo,
                 Client* aClient);
  ~OfflineStorage();

  nsRefPtr<Context::ThreadsafeHandle> mContext;
  const QuotaInfo mQuotaInfo;
  nsRefPtr<Client> mClient;
  nsTArray<nsCOMPtr<nsIRunnable>> mDestroyCallbacks;

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOFFLINESTORAGE
};

} 
} 
} 

#endif
