




#include "gfxTelemetry.h"

namespace mozilla {
namespace gfx {

const char*
FeatureStatusToString(FeatureStatus aStatus)
{
  switch (aStatus) {
    case FeatureStatus::Unused:
      return "unused";
    case FeatureStatus::Unavailable:
      return "unavailable";
    case FeatureStatus::Blocked:
      return "blocked";
    case FeatureStatus::Blacklisted:
      return "blacklisted";
    case FeatureStatus::Failed:
      return "failed";
    case FeatureStatus::Disabled:
      return "disabled";
    case FeatureStatus::Available:
      return "available";
    default:
      MOZ_ASSERT_UNREACHABLE("missing status case");
      return "unknown";
  }
}

} 
} 
