





#ifndef nsOSHelperAppService_h__
#define nsOSHelperAppService_h__





#include "nsExternalHelperAppService.h"
#include "nsCExternalHandlerService.h"
#include "nsMIMEInfoImpl.h"
#include "nsCOMPtr.h"

class nsOSHelperAppService : public nsExternalHelperAppService
{
public:
  nsOSHelperAppService();
  virtual ~nsOSHelperAppService();

  
  NS_IMETHOD GetApplicationDescription(const nsACString& aScheme, nsAString& _retval);
  
  
  NS_IMETHOD GetFromTypeAndExtension(const nsACString& aType, const nsACString& aFileExt, nsIMIMEInfo ** aMIMEInfo);
  already_AddRefed<nsIMIMEInfo> GetMIMEInfoFromOS(const nsACString& aMIMEType, const nsACString& aFileExt, bool * aFound);
  NS_IMETHOD GetProtocolHandlerInfoFromOS(const nsACString &aScheme,
                                          bool *found,
                                          nsIHandlerInfo **_retval);

  
  
  
  
  virtual nsresult GetFileTokenForPath(const char16_t * platformAppPath, nsIFile ** aFile);

  nsresult OSProtocolHandlerExists(const char * aScheme,
                                   bool * aHandlerExists);

private:
  uint32_t mPermissions;
};

#endif 
