






































#include "mozilla/plugins/PluginThreadChild.h"

#include "prlink.h"

#include "base/command_line.h"
#include "base/string_util.h"
#include "chrome/common/child_process.h"
#include "chrome/common/chrome_switches.h"

using mozilla::ipc::GeckoThread;

namespace mozilla {
namespace plugins {

PluginThreadChild::PluginThreadChild() :
    GeckoThread(),
    mPlugin()
{
}

PluginThreadChild::~PluginThreadChild()
{
}

void
PluginThreadChild::Init()
{
    GeckoThread::Init();

    
    
    std::vector<std::wstring> values =
        CommandLine::ForCurrentProcess()->GetLooseValues();

    
    DCHECK(values.size() >= 1);

    std::string pluginFilename = WideToUTF8(values[0]);

    
    mPlugin.Init(pluginFilename, owner_loop(), channel());
}

void
PluginThreadChild::CleanUp()
{
    mPlugin.CleanUp();
    GeckoThread::CleanUp();
}

} 
} 
