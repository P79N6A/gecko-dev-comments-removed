







































#ifndef nsOSHelperAppService_h__
#define nsOSHelperAppService_h__





#include "nsExternalHelperAppService.h"
#include "nsCExternalHandlerService.h"
#include "nsCOMPtr.h"

class nsOSHelperAppService : public nsExternalHelperAppService
{
public:
  nsOSHelperAppService();
  virtual ~nsOSHelperAppService();

  
  NS_IMETHOD GetApplicationDescription(const nsACString& aScheme, nsAString& _retval);
  
  
  NS_IMETHOD GetFromTypeAndExtension(const nsACString& aType, const nsACString& aFileExt, nsIMIMEInfo ** aMIMEInfo);
  already_AddRefed<nsIMIMEInfo> GetMIMEInfoFromOS(const nsACString& aMIMEType, const nsACString& aFileExt, PRBool * aFound);
  already_AddRefed<nsIHandlerInfo> GetProtocolInfoFromOS(const nsACString &aScheme);

  
  
  
  
  virtual nsresult GetFileTokenForPath(const PRUnichar * platformAppPath, nsIFile ** aFile);

  nsresult OSProtocolHandlerExists(const char * aScheme,
                                   PRBool * aHandlerExists);

protected:
  
  void UpdateCreatorInfo(nsIMIMEInfo * aMIMEInfo);
};

#endif 
