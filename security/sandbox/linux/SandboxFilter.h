





#ifndef mozilla_SandboxFilter_h
#define mozilla_SandboxFilter_h

struct sock_fprog;

namespace mozilla {

const sock_fprog* GetSandboxFilter();

}

#endif
