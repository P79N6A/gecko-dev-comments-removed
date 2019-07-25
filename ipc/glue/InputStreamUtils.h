



#ifndef mozilla_ipc_InputStreamUtils_h
#define mozilla_ipc_InputStreamUtils_h

#include "mozilla/ipc/IPCSerializableParams.h"
#include "nsCOMPtr.h"
#include "nsIInputStream.h"

namespace mozilla {
namespace ipc {

void
SerializeInputStream(nsIInputStream* aInputStream,
                     InputStreamParams& aParams);

void
SerializeInputStream(nsIInputStream* aInputStream,
                     OptionalInputStreamParams& aParams);

already_AddRefed<nsIInputStream>
DeserializeInputStream(const InputStreamParams& aParams);

already_AddRefed<nsIInputStream>
DeserializeInputStream(const OptionalInputStreamParams& aParams);

} 
} 

#endif 
