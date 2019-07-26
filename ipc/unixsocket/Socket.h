





#ifndef mozilla_ipc_Socket_h
#define mozilla_ipc_Socket_h

namespace mozilla {
namespace ipc {

int
GetNewSocket(int type, const char* aAddress, int channel, bool auth, bool encrypt);

int
CloseSocket(int aFd);

} 
} 

#endif 
