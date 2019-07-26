




#ifndef nsGnomeVFSService_h_
#define nsGnomeVFSService_h_

#include "nsIGnomeVFSService.h"
#include "mozilla/Attributes.h"

#define NS_GNOMEVFSSERVICE_CID \
{0x5f43022c, 0x6194, 0x4b37, {0xb2, 0x6d, 0xe4, 0x10, 0x24, 0x62, 0x52, 0x64}}

class nsGnomeVFSService MOZ_FINAL : public nsIGnomeVFSService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIGNOMEVFSSERVICE

  nsresult Init();
};

#endif
