



#ifndef LocalCertService_h
#define LocalCertService_h

#include "nsILocalCertService.h"

namespace mozilla {

class LocalCertService final : public nsILocalCertService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSILOCALCERTSERVICE

  LocalCertService();

private:
  nsresult LoginToKeySlot();
  ~LocalCertService();
};

} 

#endif 
