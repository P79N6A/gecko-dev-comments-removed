







































#ifndef nsExternalProtocolHandler_h___
#define nsExternalProtocolHandler_h___

#include "nsIExternalProtocolHandler.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsWeakReference.h"
#include "nsIExternalProtocolService.h"

class nsIURI;


class nsExternalProtocolHandler : public nsIExternalProtocolHandler, public nsSupportsWeakReference
{
public:
	NS_DECL_ISUPPORTS
	NS_DECL_NSIPROTOCOLHANDLER
	NS_DECL_NSIEXTERNALPROTOCOLHANDLER

	nsExternalProtocolHandler();
	~nsExternalProtocolHandler();

protected:
  
  PRBool HaveExternalProtocolHandler(nsIURI * aURI);
	nsCString	m_schemeName;
  nsCOMPtr<nsIExternalProtocolService> m_extProtService;
};

#endif 

