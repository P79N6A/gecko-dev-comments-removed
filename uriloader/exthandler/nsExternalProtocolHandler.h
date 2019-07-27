





#ifndef nsExternalProtocolHandler_h___
#define nsExternalProtocolHandler_h___

#include "nsIExternalProtocolHandler.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsWeakReference.h"
#include "nsIExternalProtocolService.h"
#include "mozilla/Attributes.h"

class nsIURI;


class nsExternalProtocolHandler final : public nsIExternalProtocolHandler, public nsSupportsWeakReference
{
public:
	NS_DECL_THREADSAFE_ISUPPORTS
	NS_DECL_NSIPROTOCOLHANDLER
	NS_DECL_NSIEXTERNALPROTOCOLHANDLER

	nsExternalProtocolHandler();

protected:
  ~nsExternalProtocolHandler();

  
  bool HaveExternalProtocolHandler(nsIURI * aURI);
	nsCString	m_schemeName;
};

#endif 

