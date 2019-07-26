




#ifndef nsGIOService_h_
#define nsGIOService_h_

#include "nsIGIOService.h"

#define NS_GIOSERVICE_CID \
{0xe3a1f3c9, 0x3ae1, 0x4b40, {0xa5, 0xe0, 0x7b, 0x45, 0x7f, 0xc9, 0xa9, 0xad}}

class nsGIOService MOZ_FINAL : public nsIGIOService
{
  ~nsGIOService() {}

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIGIOSERVICE
};

#endif

