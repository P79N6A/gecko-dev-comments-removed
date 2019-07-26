



#ifndef BASE_SYS_INFO_H_
#define BASE_SYS_INFO_H_

#include <string>

#include "base/base_export.h"
#include "base/basictypes.h"
#include "base/files/file_path.h"
#include "build/build_config.h"

namespace base {

class BASE_EXPORT SysInfo {
 public:
  
  static int NumberOfProcessors();

  
  static int64 AmountOfPhysicalMemory();

  
  
  static int64 AmountOfAvailablePhysicalMemory();

  
  static int AmountOfPhysicalMemoryMB() {
    return static_cast<int>(AmountOfPhysicalMemory() / 1024 / 1024);
  }

  
  
  static int64 AmountOfFreeDiskSpace(const FilePath& path);

  
  static int64 Uptime();

  
  static std::string OperatingSystemName();

  
  static std::string OperatingSystemVersion();

  
  
  
  
  
  
  
  
  static void OperatingSystemVersionNumbers(int32* major_version,
                                            int32* minor_version,
                                            int32* bugfix_version);

  
  
  
  
  static std::string OperatingSystemArchitecture();

  
  
  
  
  static std::string CPUModelName();

  
  
  static size_t VMAllocationGranularity();

#if defined(OS_POSIX) && !defined(OS_MACOSX)
  
  static size_t MaxSharedMemorySize();
#endif  

#if defined(OS_CHROMEOS)
  
  
  static std::string GetLinuxStandardBaseVersionKey();

  
  
  static void ParseLsbRelease(const std::string& lsb_release,
                              int32* major_version,
                              int32* minor_version,
                              int32* bugfix_version);

  
  static FilePath GetLsbReleaseFilePath();
#endif  

#if defined(OS_ANDROID)
  
  static std::string GetAndroidBuildCodename();

  
  static std::string GetAndroidBuildID();

  
  static std::string GetDeviceName();

  static int DalvikHeapSizeMB();
  static int DalvikHeapGrowthLimitMB();
#endif  
};

}  

#endif  
