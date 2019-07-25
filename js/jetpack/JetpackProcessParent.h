




































#ifndef mozilla_jetpack_JetpackProcessParent_h
#define mozilla_jetpack_JetpackProcessParent_h

#include "base/basictypes.h"

#include "base/file_path.h"
#include "base/scoped_ptr.h"
#include "base/thread.h"
#include "base/waitable_event.h"
#include "chrome/common/child_process_host.h"

#include "mozilla/ipc/GeckoChildProcessHost.h"

namespace mozilla {
namespace jetpack {

class JetpackProcessParent : mozilla::ipc::GeckoChildProcessHost
{
public:
  JetpackProcessParent();
  ~JetpackProcessParent();

  


  void Launch();

  using mozilla::ipc::GeckoChildProcessHost::GetShutDownEvent;
  using mozilla::ipc::GeckoChildProcessHost::GetChannel;
  using mozilla::ipc::GeckoChildProcessHost::GetChildProcessHandle;

private:
    DISALLOW_EVIL_CONSTRUCTORS(JetpackProcessParent);
};

} 
} 

#endif 
