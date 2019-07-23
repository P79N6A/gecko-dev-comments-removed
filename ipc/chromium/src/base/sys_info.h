



#ifndef BASE_SYS_INFO_H_
#define BASE_SYS_INFO_H_

#include "base/basictypes.h"

#include <string>

namespace base {

class SysInfo {
 public:
  
  
  
  static int NumberOfProcessors();

  
  static int64 AmountOfPhysicalMemory();

  
  static int AmountOfPhysicalMemoryMB() {
    return static_cast<int>(AmountOfPhysicalMemory() / 1024 / 1024);
  }

  
  
  static int64 AmountOfFreeDiskSpace(const std::wstring& path);

  
  
  static bool HasEnvVar(const wchar_t* var);

  
  
  
  static std::wstring GetEnvVar(const wchar_t* var);

  
  static std::string OperatingSystemName();

  
  static std::string OperatingSystemVersion();

  
  
  
  
  
  static void OperatingSystemVersionNumbers(int32 *major_version,
                                            int32 *minor_version,
                                            int32 *bugfix_version);

  
  
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
