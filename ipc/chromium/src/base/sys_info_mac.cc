



#include "base/sys_info.h"

#include <CoreServices/CoreServices.h>

namespace base {


void SysInfo::OperatingSystemVersionNumbers(int32_t *major_version,
                                            int32_t *minor_version,
                                            int32_t *bugfix_version) {
  static bool is_initialized = false;
  static int32_t major_version_cached = 0;
  static int32_t minor_version_cached = 0;
  static int32_t bugfix_version_cached = 0;

  if (!is_initialized) {
    
    Gestalt(gestaltSystemVersionMajor,
        reinterpret_cast<SInt32*>(&major_version_cached));
    Gestalt(gestaltSystemVersionMinor,
        reinterpret_cast<SInt32*>(&minor_version_cached));
    Gestalt(gestaltSystemVersionBugFix,
        reinterpret_cast<SInt32*>(&bugfix_version_cached));
    is_initialized = true;
  }

  *major_version = major_version_cached;
  *minor_version = minor_version_cached;
  *bugfix_version = bugfix_version_cached;
}


void SysInfo::CacheSysInfo() {
  
  
  
  NumberOfProcessors();
  int32_t dummy;
  OperatingSystemVersionNumbers(&dummy, &dummy, &dummy);
}

}  
