




#ifndef nsGConfService_h_
#define nsGConfService_h_

#include "nsIGConfService.h"
#include "gconf/gconf-client.h"
#include "mozilla/Attributes.h"

#define NS_GCONFSERVICE_CID \
{0xd96d5985, 0xa13a, 0x4bdc, {0x93, 0x86, 0xef, 0x34, 0x8d, 0x7a, 0x97, 0xa1}}

class nsGConfService final : public nsIGConfService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIGCONFSERVICE

  nsGConfService() : mClient(nullptr) {}
  nsresult Init();

private:
  ~nsGConfService();

  GConfClient *mClient;
};

#endif
