




#ifndef nsGSettingsService_h_
#define nsGSettingsService_h_

#include "nsIGSettingsService.h"

#define NS_GSETTINGSSERVICE_CID \
{0xbfd4a9d8, 0xd886, 0x4161, {0x81, 0xef, 0x88, 0x68, 0xda, 0x11, 0x41, 0x70}}

class nsGSettingsService final : public nsIGSettingsService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIGSETTINGSSERVICE

  nsresult Init();

private:
  ~nsGSettingsService();
};

#endif

