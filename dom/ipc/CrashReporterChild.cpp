





































#include "mozilla/plugins/PluginModuleChild.h"
#include "ContentChild.h"
#include "CrashReporterChild.h"
#include "nsXULAppAPI.h"

using mozilla::plugins::PluginModuleChild;

namespace mozilla {
namespace dom {


PCrashReporterChild*
CrashReporterChild::GetCrashReporter()
{
  switch (XRE_GetProcessType()) {
    case GeckoProcessType_Content: {
      ContentChild* child = ContentChild::GetSingleton();
      return child->ManagedPCrashReporterChild()[0];
    }
    case GeckoProcessType_Plugin: {
      PluginModuleChild* child = PluginModuleChild::current();
      return child->ManagedPCrashReporterChild()[0];
    }
    default:
      return nsnull;
  }
}

}
}
