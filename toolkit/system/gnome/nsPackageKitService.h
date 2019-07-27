




#ifndef nsPackageKitService_h_
#define nsPackageKitService_h_

#include "nsIPackageKitService.h"

#define NS_PACKAGEKITSERVICE_CID \
{0x9c95515e, 0x611d, 0x11e4, {0xb9, 0x7e, 0x60, 0xa4, 0x4c, 0x71, 0x70, 0x42}}

class nsPackageKitService MOZ_FINAL : public nsIPackageKitService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPACKAGEKITSERVICE

  nsresult Init();

private:
  ~nsPackageKitService();
};

#endif
