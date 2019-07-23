





































#ifndef nsOSHelperAppService_h__
#define nsOSHelperAppService_h__





#include "nsExternalHelperAppService.h"
#include "nsCExternalHandlerService.h"
#include "nsCOMPtr.h"
#include <windows.h>

class nsMIMEInfoWin;

class nsOSHelperAppService : public nsExternalHelperAppService
{
public:
  nsOSHelperAppService();
  virtual ~nsOSHelperAppService();

  
  nsresult OSProtocolHandlerExists(const char * aProtocolScheme, PRBool * aHandlerExists);
  nsresult LoadUriInternal(nsIURI * aURL);
  NS_IMETHOD GetApplicationDescription(const nsACString& aScheme, nsAString& _retval);

  
  already_AddRefed<nsIMIMEInfo> GetMIMEInfoFromOS(const nsACString& aMIMEType, const nsACString& aFileExt, PRBool *aFound);
  already_AddRefed<nsIHandlerInfo> GetProtocolInfoFromOS(const nsACString &aScheme);

  


  static PRBool GetValueString(HKEY hKey, const PRUnichar* pValueName, nsAString& result);

protected:
  nsresult GetDefaultAppInfo(const nsAString& aTypeName, nsAString& aDefaultDescription, nsIFile** aDefaultApplication);
  
  already_AddRefed<nsMIMEInfoWin> GetByExtension(const nsAFlatString& aFileExt, const char *aTypeHint = nsnull);
  nsresult FindOSMimeInfoForType(const char * aMimeContentType, nsIURI * aURI, char ** aFileExtension, nsIMIMEInfo ** aMIMEInfo);

  static nsresult GetMIMEInfoFromRegistry(const nsAFlatString& fileType, nsIMIMEInfo *pInfo);
  
  static PRBool typeFromExtEquals(const PRUnichar* aExt, const char *aType);
};

#endif 
