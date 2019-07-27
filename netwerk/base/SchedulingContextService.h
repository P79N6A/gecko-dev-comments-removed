





#ifndef mozilla__net__SchedulingContextService_h
#define mozilla__net__SchedulingContextService_h

#include "nsCOMPtr.h"
#include "nsInterfaceHashtable.h"
#include "nsIObserver.h"
#include "nsISchedulingContext.h"
#include "nsWeakReference.h"

class nsIUUIDGenerator;

namespace mozilla {
namespace net {

class SchedulingContextService final : public nsISchedulingContextService
                                     , public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISCHEDULINGCONTEXTSERVICE
  NS_DECL_NSIOBSERVER

  SchedulingContextService();

  nsresult Init();
  void Shutdown();
  static nsresult Create(nsISupports *outer, const nsIID& iid, void **result);

private:
  virtual ~SchedulingContextService();

  static SchedulingContextService *sSelf;

  nsInterfaceHashtable<nsIDHashKey, nsIWeakReference> mTable;
  nsCOMPtr<nsIUUIDGenerator> mUUIDGen;
};

} 
} 

#endif 
