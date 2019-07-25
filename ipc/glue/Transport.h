







































#ifndef mozilla_ipc_Transport_h
#define mozilla_ipc_Transport_h 1

#include "base/process_util.h"
#include "chrome/common/ipc_channel.h"

#ifdef OS_POSIX
# include "mozilla/ipc/Transport_posix.h"
#elif OS_WIN
# include "mozilla/ipc/Transport_win.h"
#endif

namespace mozilla {
namespace ipc {


typedef IPC::Channel Transport;

bool CreateTransport(base::ProcessHandle aProcOne, base::ProcessHandle aProcTwo,
                     TransportDescriptor* aOne, TransportDescriptor* aTwo);

Transport* OpenDescriptor(const TransportDescriptor& aTd,
                          Transport::Mode aMode);


} 
} 

#endif  
