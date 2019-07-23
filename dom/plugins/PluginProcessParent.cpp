






































#include "mozilla/plugins/PluginProcessParent.h"

#include "base/string_util.h"

using mozilla::ipc::GeckoChildProcessHost;

namespace mozilla {
namespace plugins {


PluginProcessParent::PluginProcessParent(const std::string& aPluginFilePath) :
    GeckoChildProcessHost(GeckoChildProcess_Plugin),
    mPluginFilePath(aPluginFilePath)
{
}

PluginProcessParent::~PluginProcessParent()
{
}

bool
PluginProcessParent::Launch()
{
    std::vector<std::wstring> args;
    args.push_back(UTF8ToWide(mPluginFilePath));
    return mozilla::ipc::GeckoChildProcessHost::Launch(args);
}


} 
} 
