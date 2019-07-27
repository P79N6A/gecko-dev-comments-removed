




#ifndef gfx_src_gfxTelemetry_h__
#define gfx_src_gfxTelemetry_h__

namespace mozilla {
namespace gfx {




enum class FeatureStatus
{
  
  Unused,

  
  
  Unavailable,

  
  
  Blocked,

  
  Blacklisted,

  
  Failed,

  
  Disabled,

  
  Available
};

const char* FeatureStatusToString(FeatureStatus aStatus);

} 
} 

#endif 
