






































#include "mozilla/ipc/IOThreadChild.h"
#include "mozilla/plugins/PluginProcessChild.h"

#include "prlink.h"

#include "base/command_line.h"
#include "base/string_util.h"
#include "chrome/common/chrome_switches.h"

#ifdef XP_WIN
#include <objbase.h>
#endif

using mozilla::ipc::IOThreadChild;

namespace mozilla {
namespace plugins {

bool
PluginProcessChild::Init()
{
#ifdef XP_WIN
    
    
    ::OleInitialize(NULL);
#endif

    
    
    message_loop()->set_exception_restoration(true);

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

    mPlugin.Init(pluginFilename, ParentHandle(),
                 IOThreadChild::message_loop(),
                 IOThreadChild::channel());

    return true;
}

void
PluginProcessChild::CleanUp()
{
#ifdef XP_WIN
    ::OleUninitialize();
#endif
}


void
PluginProcessChild::AppendNotesToCrashReport(const nsCString& aNotes)
{
    AssertPluginThread();

    PluginProcessChild* p = PluginProcessChild::current();
    if (p) {
        p->mPlugin.SendAppendNotesToCrashReport(aNotes);
    }
}

} 
} 
