






































#include "mozilla/plugins/PluginProcessParent.h"

#include "base/string_util.h"

using mozilla::ipc::GeckoChildProcessHost;

namespace mozilla {
namespace plugins {


PluginProcessParent::PluginProcessParent(const std::string& aPluginFilePath) :
    GeckoChildProcessHost(GeckoProcessType_Plugin),
    mPluginFilePath(aPluginFilePath)
{
  
}

PluginProcessParent::~PluginProcessParent()
{
}

bool
PluginProcessParent::Launch()
{
    std::vector<std::string> args;
    args.push_back(mPluginFilePath);
    return SyncLaunch(args);
}


} 
} 
