





































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
  const InfallibleTArray<PCrashReporterChild*>* reporters = nsnull;
  switch (XRE_GetProcessType()) {
    case GeckoProcessType_Content: {
      ContentChild* child = ContentChild::GetSingleton();
      reporters = &child->ManagedPCrashReporterChild();
      break;
    }
    case GeckoProcessType_Plugin: {
      PluginModuleChild* child = PluginModuleChild::current();
      reporters = &child->ManagedPCrashReporterChild();
      break;
    }
    default:
      break;
  }
  if (reporters && reporters->Length() > 0) {
    return reporters->ElementAt(0);
  }
  return nsnull;
}

}
}
