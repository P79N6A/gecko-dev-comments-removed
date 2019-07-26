




#ifndef nsOSHelperAppService_h__
#define nsOSHelperAppService_h__





#include "nsExternalHelperAppService.h"
#include "nsCExternalHandlerService.h"
#include "nsMIMEInfoImpl.h"
#include "nsCOMPtr.h"
#include <windows.h>

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0600
#include <shlobj.h>

class nsMIMEInfoWin;

class nsOSHelperAppService : public nsExternalHelperAppService
{
public:
  nsOSHelperAppService();
  virtual ~nsOSHelperAppService();

  
  nsresult OSProtocolHandlerExists(const char * aProtocolScheme, bool * aHandlerExists);
  nsresult LoadUriInternal(nsIURI * aURL);
  NS_IMETHOD GetApplicationDescription(const nsACString& aScheme, nsAString& _retval);

  
  already_AddRefed<nsIMIMEInfo> GetMIMEInfoFromOS(const nsACString& aMIMEType, const nsACString& aFileExt, bool *aFound);
  NS_IMETHOD GetProtocolHandlerInfoFromOS(const nsACString &aScheme, 
                                          bool *found,
                                          nsIHandlerInfo **_retval);

  


  static bool GetValueString(HKEY hKey, const char16_t* pValueName, nsAString& result);

  
  static bool CleanupCmdHandlerPath(nsAString& aCommandHandler);

protected:
  nsresult GetDefaultAppInfo(const nsAString& aTypeName, nsAString& aDefaultDescription, nsIFile** aDefaultApplication);
  
  already_AddRefed<nsMIMEInfoWin> GetByExtension(const nsAFlatString& aFileExt, const char *aTypeHint = nullptr);
  nsresult FindOSMimeInfoForType(const char * aMimeContentType, nsIURI * aURI, char ** aFileExtension, nsIMIMEInfo ** aMIMEInfo);

  static nsresult GetMIMEInfoFromRegistry(const nsAFlatString& fileType, nsIMIMEInfo *pInfo);
  
  static bool typeFromExtEquals(const char16_t* aExt, const char *aType);

private:
  IApplicationAssociationRegistration* mAppAssoc;
};

#endif 
