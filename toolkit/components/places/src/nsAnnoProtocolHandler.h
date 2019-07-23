





































#ifndef nsAnnoProtocolHandler_h___
#define nsAnnoProtocolHandler_h___

#include "nsCOMPtr.h"
#include "nsIAnnotationService.h"
#include "nsIProtocolHandler.h"
#include "nsIURI.h"
#include "nsString.h"
#include "nsWeakReference.h"


#define NS_ANNOPROTOCOLHANDLER_CID \
{ 0xe8b8bdb7, 0xc96c, 0x4d82, { 0x9c, 0x6f, 0x2b, 0x3c, 0x58, 0x5e, 0xc7, 0xea } }

class nsAnnoProtocolHandler : public nsIProtocolHandler, public nsSupportsWeakReference
{
public:
  nsAnnoProtocolHandler() {}

  NS_DECL_ISUPPORTS
  NS_DECL_NSIPROTOCOLHANDLER

private:
  ~nsAnnoProtocolHandler() {}

protected:
  nsresult ParseAnnoURI(nsIURI* aURI, nsIURI** aResultURI, nsCString& aName);
  nsresult GetDefaultIcon(nsIChannel** aChannel);
};

#endif 
