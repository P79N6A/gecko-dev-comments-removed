



#ifndef mozilla_ipc_URIUtils_h
#define mozilla_ipc_URIUtils_h

#include "mozilla/ipc/URIParams.h"
#include "nsCOMPtr.h"
#include "nsIURI.h"

namespace mozilla {
namespace ipc {

void
SerializeURI(nsIURI* aURI,
             URIParams& aParams);

void
SerializeURI(nsIURI* aURI,
             OptionalURIParams& aParams);

already_AddRefed<nsIURI>
DeserializeURI(const URIParams& aParams);

already_AddRefed<nsIURI>
DeserializeURI(const OptionalURIParams& aParams);

} 
} 

#endif 
