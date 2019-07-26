



#ifndef BASE_SYS_INFO_H_
#define BASE_SYS_INFO_H_

#include "base/basictypes.h"

#include <string>

namespace base {

class SysInfo {
 public:
  
  
  
  static int NumberOfProcessors();

  
  static int64_t AmountOfPhysicalMemory();

  
  static int AmountOfPhysicalMemoryMB() {
    return static_cast<int>(AmountOfPhysicalMemory() / 1024 / 1024);
  }

  
  
  static int64_t AmountOfFreeDiskSpace(const std::wstring& path);

  
  
  static bool HasEnvVar(const wchar_t* var);

  
  
  
  static std::wstring GetEnvVar(const wchar_t* var);

  
  static std::string OperatingSystemName();

  
  static std::string OperatingSystemVersion();

  
  
  
  
  
  static void OperatingSystemVersionNumbers(int32_t *major_version,
                                            int32_t *minor_version,
                                            int32_t *bugfix_version);

  
  
  static std::string CPUArchitecture();

  
  
  static void GetPrimaryDisplayDimensions(int* width, int* height);

  
  static int DisplayCount();

  
  
  static size_t VMAllocationGranularity();

#if defined(OS_MACOSX)
  
  
  
  
  static void CacheSysInfo();
#endif
};

}  

#endif  
