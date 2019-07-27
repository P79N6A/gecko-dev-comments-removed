









#ifndef WEBRTC_BASE_SYSTEMINFO_H__
#define WEBRTC_BASE_SYSTEMINFO_H__

#include <string>

#include "webrtc/base/basictypes.h"

namespace rtc {

class SystemInfo {
 public:
  enum Architecture {
    SI_ARCH_UNKNOWN = -1,
    SI_ARCH_X86 = 0,
    SI_ARCH_X64 = 1,
    SI_ARCH_ARM = 2
  };

  SystemInfo();

  
  int GetMaxPhysicalCpus();
  
  int GetMaxCpus();
  
  int GetCurCpus();
  
  Architecture GetCpuArchitecture();
  std::string GetCpuVendor();
  int GetCpuFamily();
  int GetCpuModel();
  int GetCpuStepping();
  
  int GetCpuCacheSize();
  
  int GetMaxCpuSpeed();
  int GetCurCpuSpeed();
  
  int64 GetMemorySize();
  
  std::string GetMachineModel();

  
  struct GpuInfo {
    GpuInfo() : vendor_id(0), device_id(0) {}
    std::string device_name;
    std::string description;
    int vendor_id;
    int device_id;
    std::string driver;
    std::string driver_version;
  };
  bool GetGpuInfo(GpuInfo *info);

 private:
  int physical_cpus_;
  int logical_cpus_;
  int cache_size_;
  Architecture cpu_arch_;
  std::string cpu_vendor_;
  int cpu_family_;
  int cpu_model_;
  int cpu_stepping_;
  int cpu_speed_;
  int64 memory_;
  std::string machine_model_;
};

}  

#endif
