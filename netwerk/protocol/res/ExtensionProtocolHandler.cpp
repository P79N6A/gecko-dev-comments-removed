





#include "ExtensionProtocolHandler.h"

namespace mozilla {

NS_IMPL_QUERY_INTERFACE(ExtensionProtocolHandler, nsISubstitutingProtocolHandler,
                        nsIProtocolHandler, nsISupportsWeakReference)
NS_IMPL_ADDREF_INHERITED(ExtensionProtocolHandler, SubstitutingProtocolHandler)
NS_IMPL_RELEASE_INHERITED(ExtensionProtocolHandler, SubstitutingProtocolHandler)

} 
