



#ifndef mozilla_ipc_InputStreamUtils_h
#define mozilla_ipc_InputStreamUtils_h

#include "mozilla/ipc/IPCSerializableParams.h"
#include "nsCOMPtr.h"
#include "nsIInputStream.h"

namespace mozilla {
namespace ipc {

already_AddRefed<nsIInputStream>
DeserializeInputStream(const InputStreamParams& aParams);

} 
} 

#endif 
