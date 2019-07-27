





#ifndef mozilla_dom_icc_IccIPCService_h
#define mozilla_dom_icc_IccIPCService_h

#include "nsCOMPtr.h"
#include "nsIIccService.h"

namespace mozilla {
namespace dom {
namespace icc {

class IccChild;

class IccIPCService final : public nsIIccService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIICCSERVICE

  IccIPCService();

private:
  ~IccIPCService();

  nsTArray<nsRefPtr<IccChild>> mIccs;
};

} 
} 
} 

#endif 