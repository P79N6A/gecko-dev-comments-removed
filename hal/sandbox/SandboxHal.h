






































#ifndef mozilla_SandboxHal_h
#define mozilla_SandboxHal_h

namespace mozilla {
namespace hal_sandbox {

class PHalChild;
class PHalParent;

PHalChild* CreateHalChild();

PHalParent* CreateHalParent();

}
}

#endif  
