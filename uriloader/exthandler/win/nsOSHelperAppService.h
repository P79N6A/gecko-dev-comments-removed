





































#ifndef nsOSHelperAppService_h__
#define nsOSHelperAppService_h__





#include "nsExternalHelperAppService.h"
#include "nsCExternalHandlerService.h"
#include "nsCOMPtr.h"
#include <windows.h>

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0600
#define INITGUID
#include <shlobj.h>

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
  NS_IMETHOD GetProtocolHandlerInfoFromOS(const nsACString &aScheme, 
                                          PRBool *found,
                                          nsIHandlerInfo **_retval);

  


  static PRBool GetValueString(HKEY hKey, const PRUnichar* pValueName, nsAString& result);

  
  static PRBool CleanupCmdHandlerPath(nsAString& aCommandHandler);

protected:
  nsresult GetDefaultAppInfo(const nsAString& aTypeName, nsAString& aDefaultDescription, nsIFile** aDefaultApplication);
  
  already_AddRefed<nsMIMEInfoWin> GetByExtension(const nsAFlatString& aFileExt, const char *aTypeHint = nsnull);
  nsresult FindOSMimeInfoForType(const char * aMimeContentType, nsIURI * aURI, char ** aFileExtension, nsIMIMEInfo ** aMIMEInfo);

  static nsresult GetMIMEInfoFromRegistry(const nsAFlatString& fileType, nsIMIMEInfo *pInfo);
  
  static PRBool typeFromExtEquals(const PRUnichar* aExt, const char *aType);

private:
#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_LONGHORN
  IApplicationAssociationRegistration* mAppAssoc;
#endif
};

#endif 
