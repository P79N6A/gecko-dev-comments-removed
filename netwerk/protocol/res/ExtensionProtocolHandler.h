




#ifndef ExtensionProtocolHandler_h___
#define ExtensionProtocolHandler_h___

#include "SubstitutingProtocolHandler.h"
#include "nsWeakReference.h"

namespace mozilla {

class ExtensionProtocolHandler final : public nsISubstitutingProtocolHandler,
                                       public mozilla::SubstitutingProtocolHandler,
                                       public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_NSIPROTOCOLHANDLER(mozilla::SubstitutingProtocolHandler::)
  NS_FORWARD_NSISUBSTITUTINGPROTOCOLHANDLER(mozilla::SubstitutingProtocolHandler::)

  
  
  ExtensionProtocolHandler()
    : SubstitutingProtocolHandler("moz-extension", URI_STD | URI_DANGEROUS_TO_LOAD | URI_IS_LOCAL_RESOURCE)
  {}

protected:
  ~ExtensionProtocolHandler() {}
};

} 

#endif 
