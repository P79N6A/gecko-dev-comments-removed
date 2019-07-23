






































#include "mozilla/plugins/PluginProcessParent.h"

#include "base/command_line.h"
#include "base/path_service.h"
#include "base/process_util.h"
#include "base/string_util.h"
#include "chrome/common/chrome_switches.h"

namespace mozilla {
namespace plugins {


const char* PluginProcessParent::kPluginProcessName = "gecko-plugin"
#ifdef OS_WIN
                                                    ".exe"
#endif
                                                    ;

PluginProcessParent::PluginProcessParent(const std::string& aPluginFilePath) :
    mPluginFilePath(aPluginFilePath)
{
}

PluginProcessParent::~PluginProcessParent()
{
}

bool
PluginProcessParent::Launch()
{
    if (!CreateChannel()) {
        return false;
    }

    FilePath exePath =
        FilePath::FromWStringHack(CommandLine::ForCurrentProcess()->program());
    exePath = exePath.DirName();
    exePath = exePath.AppendASCII(kPluginProcessName);

#if defined(OS_POSIX)
    int srcChannelFd, dstChannelFd;
    channel().GetClientFileDescriptorMapping(&srcChannelFd, &dstChannelFd);
    mFileMap.push_back(std::pair<int,int>(srcChannelFd, dstChannelFd));
#endif

    CommandLine cmdLine(exePath.ToWStringHack());
    cmdLine.AppendSwitchWithValue(switches::kProcessChannelID, channel_id());
    cmdLine.AppendLooseValue(UTF8ToWide(mPluginFilePath));

    base::ProcessHandle process;
#if defined(OS_WIN)
    base::LaunchApp(cmdLine, false, false, &process);
#elif defined(OS_POSIX)
    base::LaunchApp(cmdLine.argv(), mFileMap, false, &process);
#else
#error Bad!
#endif

    if (!process) {
        return false;
    }
    SetHandle(process);

    return true;
}


} 
} 
