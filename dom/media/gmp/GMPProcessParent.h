





#ifndef GMPProcessParent_h
#define GMPProcessParent_h 1

#include "mozilla/Attributes.h"
#include "base/basictypes.h"
#include "base/file_path.h"
#include "base/thread.h"
#include "base/waitable_event.h"
#include "chrome/common/child_process_host.h"
#include "mozilla/ipc/GeckoChildProcessHost.h"

namespace mozilla {
namespace gmp {

class GMPProcessParent MOZ_FINAL : public mozilla::ipc::GeckoChildProcessHost
{
public:
  explicit GMPProcessParent(const std::string& aGMPPath);
  ~GMPProcessParent();

  
  
  bool Launch(int32_t aTimeoutMs);

  void Delete();

  virtual bool CanShutdown() MOZ_OVERRIDE { return true; }
  const std::string& GetPluginFilePath() { return mGMPPath; }

  using mozilla::ipc::GeckoChildProcessHost::GetShutDownEvent;
  using mozilla::ipc::GeckoChildProcessHost::GetChannel;
  using mozilla::ipc::GeckoChildProcessHost::GetChildProcessHandle;

private:
  std::string mGMPPath;

  DISALLOW_COPY_AND_ASSIGN(GMPProcessParent);
};

} 
} 

#endif 
