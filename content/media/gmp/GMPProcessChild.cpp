




#include "GMPProcessChild.h"

#include "base/command_line.h"
#include "base/string_util.h"
#include "chrome/common/chrome_switches.h"
#include "mozilla/ipc/IOThreadChild.h"
#include "mozilla/BackgroundHangMonitor.h"

using mozilla::ipc::IOThreadChild;

namespace mozilla {
namespace gmp {

GMPProcessChild::GMPProcessChild(ProcessHandle parentHandle)
: ProcessChild(parentHandle)
{
}

GMPProcessChild::~GMPProcessChild()
{
}

bool
GMPProcessChild::Init()
{
  std::string pluginFilename;

#if defined(OS_POSIX)
  
  
  
  std::vector<std::string> values = CommandLine::ForCurrentProcess()->argv();
  NS_ABORT_IF_FALSE(values.size() >= 2, "not enough args");
  pluginFilename = values[1];
#elif defined(OS_WIN)
  std::vector<std::wstring> values = CommandLine::ForCurrentProcess()->GetLooseValues();
  NS_ABORT_IF_FALSE(values.size() >= 1, "not enough loose args");
  pluginFilename = WideToUTF8(values[0]);
#else
#error Not implemented
#endif

  BackgroundHangMonitor::Startup();

  return mPlugin.Init(pluginFilename,
                      ParentHandle(),
                      IOThreadChild::message_loop(),
                      IOThreadChild::channel());
}

void
GMPProcessChild::CleanUp()
{
  BackgroundHangMonitor::Shutdown();
}

} 
} 
