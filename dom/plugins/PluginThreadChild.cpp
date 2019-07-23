






































#include "mozilla/plugins/PluginThreadChild.h"

#include "prlink.h"

#include "base/command_line.h"
#include "base/string_util.h"
#include "chrome/common/child_process.h"
#include "chrome/common/chrome_switches.h"

using mozilla::ipc::MozillaChildThread;

namespace mozilla {
namespace plugins {

PluginThreadChild::PluginThreadChild(ProcessHandle aParentHandle) :
    MozillaChildThread(aParentHandle, MessageLoop::TYPE_UI)
{
    NS_ASSERTION(!gInstance, "Two PluginThreadChild?");
    gInstance = this;
}

PluginThreadChild::~PluginThreadChild()
{
    gInstance = NULL;
}

PluginThreadChild* PluginThreadChild::gInstance;

void
PluginThreadChild::Init()
{
    MozillaChildThread::Init();

    std::string pluginFilename;

#if defined(OS_POSIX)
    
    
    
    std::vector<std::string> values = CommandLine::ForCurrentProcess()->argv();
    NS_ABORT_IF_FALSE(values.size() >= 2, "not enough args");

    pluginFilename = UnmungePluginDsoPath(values[1]);

#elif defined(OS_WIN)
    std::vector<std::wstring> values =
        CommandLine::ForCurrentProcess()->GetLooseValues();
    NS_ABORT_IF_FALSE(values.size() >= 1, "not enough loose args");

    pluginFilename = WideToUTF8(values[0]);

#else
#  error Sorry
#endif

    
    mPlugin.Init(pluginFilename,
                 GetParentProcessHandle(), owner_loop(), channel());
}

void
PluginThreadChild::CleanUp()
{
    mPlugin.CleanUp();
    MozillaChildThread::CleanUp();
}


void
PluginThreadChild::AppendNotesToCrashReport(const nsCString& aNotes)
{
    AssertPluginThread();

    if (gInstance) {
        gInstance->mPlugin.SendAppendNotesToCrashReport(aNotes);
    }
}

} 
} 
