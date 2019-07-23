

































 

#ifndef nsOSHelperAppService_h__
#define nsOSHelperAppService_h__





#include "nsExternalHelperAppService.h"
#include "nsCExternalHandlerService.h"
#include "nsCOMPtr.h"

class nsMIMEInfoBeOS;

class nsOSHelperAppService : public nsExternalHelperAppService
{
public:
	nsOSHelperAppService();
	virtual ~nsOSHelperAppService();

	already_AddRefed<nsIMIMEInfo> GetMIMEInfoFromOS(const nsACString& aMIMEType, const nsACString& aFileExt, PRBool *aFound);
	already_AddRefed<nsIHandlerInfo> GetProtocolInfoFromOS(const nsACString &aScheme);

	
	nsresult OSProtocolHandlerExists(const char * aProtocolScheme, PRBool * aHandlerExists);
	nsresult LoadUriInternal(nsIURI * aURL);

protected:
	nsresult SetMIMEInfoForType(const char *aMIMEType, nsMIMEInfoBeOS **_retval);
	nsresult GetMimeInfoFromExtension(const char *aFileExt, nsMIMEInfoBeOS **_retval);
	nsresult GetMimeInfoFromMIMEType(const char *aMIMEType, nsMIMEInfoBeOS **_retval);
};

#endif 
