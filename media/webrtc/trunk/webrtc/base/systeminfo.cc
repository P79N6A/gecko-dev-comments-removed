









#include "webrtc/base/systeminfo.h"

#if defined(WEBRTC_WIN)
#include <winsock2.h>
#ifndef EXCLUDE_D3D9
#include <d3d9.h>
#endif
#include <intrin.h>  
#elif defined(WEBRTC_MAC) && !defined(WEBRTC_IOS)
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>
#elif defined(WEBRTC_LINUX)
#include <unistd.h>
#endif
#if defined(WEBRTC_MAC)
#include <sys/sysctl.h>
#endif

#if defined(WEBRTC_WIN)
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/win32.h"
#elif defined(WEBRTC_MAC) && !defined(WEBRTC_IOS)
#include "webrtc/base/macconversion.h"
#elif defined(WEBRTC_LINUX)
#include "webrtc/base/linux.h"
#endif
#include "webrtc/base/common.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/stringutils.h"

namespace rtc {


#if defined(WEBRTC_WIN)
typedef BOOL (WINAPI *LPFN_GLPI)(
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION,
    PDWORD);

static void GetProcessorInformation(int* physical_cpus, int* cache_size) {
  
  LPFN_GLPI glpi = reinterpret_cast<LPFN_GLPI>(GetProcAddress(
      GetModuleHandle(L"kernel32"),
      "GetLogicalProcessorInformation"));
  if (NULL == glpi) {
    return;
  }
  
  
  DWORD return_length = 0;
  scoped_ptr<SYSTEM_LOGICAL_PROCESSOR_INFORMATION[]> infos;
  while (!glpi(infos.get(), &return_length)) {
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
      infos.reset(new SYSTEM_LOGICAL_PROCESSOR_INFORMATION[
          return_length / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION)]);
    } else {
      return;
    }
  }
  *physical_cpus = 0;
  *cache_size = 0;
  for (size_t i = 0;
      i < return_length / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION); ++i) {
    if (infos[i].Relationship == RelationProcessorCore) {
      ++*physical_cpus;
    } else if (infos[i].Relationship == RelationCache) {
      int next_cache_size = static_cast<int>(infos[i].Cache.Size);
      if (next_cache_size >= *cache_size) {
        *cache_size = next_cache_size;
      }
    }
  }
  return;
}
#else


#if (defined(__pic__) || defined(__APPLE__)) && defined(__i386__)
static inline void __cpuid(int cpu_info[4], int info_type) {
  __asm__ volatile (  
    "mov %%ebx, %%edi\n"
    "cpuid\n"
    "xchg %%edi, %%ebx\n"
    : "=a"(cpu_info[0]), "=D"(cpu_info[1]), "=c"(cpu_info[2]), "=d"(cpu_info[3])
    : "a"(info_type)
  );  
}
#elif defined(__i386__) || defined(__x86_64__)
static inline void __cpuid(int cpu_info[4], int info_type) {
  __asm__ volatile (  
    "cpuid\n"
    : "=a"(cpu_info[0]), "=b"(cpu_info[1]), "=c"(cpu_info[2]), "=d"(cpu_info[3])
    : "a"(info_type)
  );  
}
#endif
#endif  



SystemInfo::SystemInfo()
    : physical_cpus_(1), logical_cpus_(1), cache_size_(0),
      cpu_family_(0), cpu_model_(0), cpu_stepping_(0),
      cpu_speed_(0), memory_(0) {
  
#if defined(__arm__) || defined(_M_ARM)
  cpu_arch_ = SI_ARCH_ARM;
#elif defined(__x86_64__) || defined(_M_X64)
  cpu_arch_ = SI_ARCH_X64;
#elif defined(__i386__) || defined(_M_IX86)
  cpu_arch_ = SI_ARCH_X86;
#else
  cpu_arch_ = SI_ARCH_UNKNOWN;
#endif

#if defined(WEBRTC_WIN)
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  logical_cpus_ = si.dwNumberOfProcessors;
  GetProcessorInformation(&physical_cpus_, &cache_size_);
  if (physical_cpus_ <= 0) {
    physical_cpus_ = logical_cpus_;
  }
  cpu_family_ = si.wProcessorLevel;
  cpu_model_ = si.wProcessorRevision >> 8;
  cpu_stepping_ = si.wProcessorRevision & 0xFF;
#elif defined(WEBRTC_MAC)
  uint32_t sysctl_value;
  size_t length = sizeof(sysctl_value);
  if (!sysctlbyname("hw.physicalcpu_max", &sysctl_value, &length, NULL, 0)) {
    physical_cpus_ = static_cast<int>(sysctl_value);
  }
  length = sizeof(sysctl_value);
  if (!sysctlbyname("hw.logicalcpu_max", &sysctl_value, &length, NULL, 0)) {
    logical_cpus_ = static_cast<int>(sysctl_value);
  }
  uint64_t sysctl_value64;
  length = sizeof(sysctl_value64);
  if (!sysctlbyname("hw.l3cachesize", &sysctl_value64, &length, NULL, 0)) {
    cache_size_ = static_cast<int>(sysctl_value64);
  }
  if (!cache_size_) {
    length = sizeof(sysctl_value64);
    if (!sysctlbyname("hw.l2cachesize", &sysctl_value64, &length, NULL, 0)) {
      cache_size_ = static_cast<int>(sysctl_value64);
    }
  }
  length = sizeof(sysctl_value);
  if (!sysctlbyname("machdep.cpu.family", &sysctl_value, &length, NULL, 0)) {
    cpu_family_ = static_cast<int>(sysctl_value);
  }
  length = sizeof(sysctl_value);
  if (!sysctlbyname("machdep.cpu.model", &sysctl_value, &length, NULL, 0)) {
    cpu_model_ = static_cast<int>(sysctl_value);
  }
  length = sizeof(sysctl_value);
  if (!sysctlbyname("machdep.cpu.stepping", &sysctl_value, &length, NULL, 0)) {
    cpu_stepping_ = static_cast<int>(sysctl_value);
  }
#elif defined(__native_client__)
  
#else  
  ProcCpuInfo proc_info;
  if (proc_info.LoadFromSystem()) {
    proc_info.GetNumCpus(&logical_cpus_);
    proc_info.GetNumPhysicalCpus(&physical_cpus_);
    proc_info.GetCpuFamily(&cpu_family_);
#if defined(CPU_X86)
    
    proc_info.GetSectionIntValue(0, "model", &cpu_model_);
    proc_info.GetSectionIntValue(0, "stepping", &cpu_stepping_);
    proc_info.GetSectionIntValue(0, "cpu MHz", &cpu_speed_);
    proc_info.GetSectionIntValue(0, "cache size", &cache_size_);
    cache_size_ *= 1024;
#endif
  }
  
  
  
  
  
  
  
  int max_freq = rtc::ReadCpuMaxFreq();
  if (max_freq > 0) {
    cpu_speed_ = max_freq / 1000;
  }
#endif


#ifdef CPU_X86
  if (cache_size_ == 0) {
    int cpu_info[4];
    __cpuid(cpu_info, 0x80000000);  
    if (static_cast<uint32>(cpu_info[0]) >= 0x80000006) {
      __cpuid(cpu_info, 0x80000006);
      cache_size_ = (cpu_info[2] >> 16) * 1024;
    }
  }
#endif
}


int SystemInfo::GetMaxCpus() {
  return logical_cpus_;
}


int SystemInfo::GetMaxPhysicalCpus() {
  return physical_cpus_;
}




int SystemInfo::GetCurCpus() {
  int cur_cpus;
#if defined(WEBRTC_WIN)
  DWORD_PTR process_mask, system_mask;
  ::GetProcessAffinityMask(::GetCurrentProcess(), &process_mask, &system_mask);
  for (cur_cpus = 0; process_mask; ++cur_cpus) {
    
    
    process_mask &= (process_mask - 1);
  }
#elif defined(WEBRTC_MAC)
  uint32_t sysctl_value;
  size_t length = sizeof(sysctl_value);
  int error = sysctlbyname("hw.ncpu", &sysctl_value, &length, NULL, 0);
  cur_cpus = !error ? static_cast<int>(sysctl_value) : 1;
#else
  
  cur_cpus = static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));
#endif
  return cur_cpus;
}


SystemInfo::Architecture SystemInfo::GetCpuArchitecture() {
  return cpu_arch_;
}




std::string SystemInfo::GetCpuVendor() {
  if (cpu_vendor_.empty()) {
#if defined(CPU_X86)
    int cpu_info[4];
    __cpuid(cpu_info, 0);
    cpu_info[0] = cpu_info[1];  
    cpu_info[1] = cpu_info[3];
    
    cpu_info[3] = 0;
    cpu_vendor_ = std::string(reinterpret_cast<char*>(&cpu_info[0]));
#elif defined(CPU_ARM)
    cpu_vendor_ = std::string("ARM");
#else
    cpu_vendor_ = std::string("Undefined");
#endif
  }
  return cpu_vendor_;
}

int SystemInfo::GetCpuCacheSize() {
  return cache_size_;
}


int SystemInfo::GetCpuFamily() {
  return cpu_family_;
}


int SystemInfo::GetCpuModel() {
  return cpu_model_;
}


int SystemInfo::GetCpuStepping() {
  return cpu_stepping_;
}



int SystemInfo::GetMaxCpuSpeed() {
  if (cpu_speed_) {
    return cpu_speed_;
  }
#if defined(WEBRTC_WIN)
  HKEY key;
  static const WCHAR keyName[] =
      L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0";

  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyName , 0, KEY_QUERY_VALUE, &key)
      == ERROR_SUCCESS) {
    DWORD data, len;
    len = sizeof(data);

    if (RegQueryValueEx(key, L"~Mhz", 0, 0, reinterpret_cast<LPBYTE>(&data),
                        &len) == ERROR_SUCCESS) {
      cpu_speed_ = data;
    } else {
      LOG(LS_WARNING) << "Failed to query registry value HKLM\\" << keyName
                      << "\\~Mhz";
      cpu_speed_ = -1;
    }

    RegCloseKey(key);
  } else {
    LOG(LS_WARNING) << "Failed to open registry key HKLM\\" << keyName;
    cpu_speed_ = -1;
  }
#elif defined(WEBRTC_MAC)
  uint64_t sysctl_value;
  size_t length = sizeof(sysctl_value);
  int error = sysctlbyname("hw.cpufrequency_max", &sysctl_value, &length,
                           NULL, 0);
  cpu_speed_ = !error ? static_cast<int>(sysctl_value/1000000) : -1;
#else
  
  cpu_speed_ = 0;
#endif
  return cpu_speed_;
}




int SystemInfo::GetCurCpuSpeed() {
#if defined(WEBRTC_WIN)
  
  
  return GetMaxCpuSpeed();
#elif defined(WEBRTC_MAC)
  uint64_t sysctl_value;
  size_t length = sizeof(sysctl_value);
  int error = sysctlbyname("hw.cpufrequency", &sysctl_value, &length, NULL, 0);
  return !error ? static_cast<int>(sysctl_value/1000000) : GetMaxCpuSpeed();
#else  
  
  return GetMaxCpuSpeed();
#endif
}



int64 SystemInfo::GetMemorySize() {
  if (memory_) {
    return memory_;
  }

#if defined(WEBRTC_WIN)
  MEMORYSTATUSEX status = {0};
  status.dwLength = sizeof(status);

  if (GlobalMemoryStatusEx(&status)) {
    memory_ = status.ullTotalPhys;
  } else {
    LOG_GLE(LS_WARNING) << "GlobalMemoryStatusEx failed.";
    memory_ = -1;
  }

#elif defined(WEBRTC_MAC)
  size_t len = sizeof(memory_);
  int error = sysctlbyname("hw.memsize", &memory_, &len, NULL, 0);
  if (error || memory_ == 0) {
    memory_ = -1;
  }
#else  
  memory_ = static_cast<int64>(sysconf(_SC_PHYS_PAGES)) *
      static_cast<int64>(sysconf(_SC_PAGESIZE));
  if (memory_ < 0) {
    LOG(LS_WARNING) << "sysconf(_SC_PHYS_PAGES) failed."
                    << "sysconf(_SC_PHYS_PAGES) " << sysconf(_SC_PHYS_PAGES)
                    << "sysconf(_SC_PAGESIZE) " << sysconf(_SC_PAGESIZE);
    memory_ = -1;
  }
#endif

  return memory_;
}






std::string SystemInfo::GetMachineModel() {
  if (!machine_model_.empty()) {
    return machine_model_;
  }

#if defined(WEBRTC_MAC)
  char buffer[128];
  size_t length = sizeof(buffer);
  int error = sysctlbyname("hw.model", buffer, &length, NULL, 0);
  if (!error) {
    machine_model_.assign(buffer, length - 1);
  } else {
    machine_model_.clear();
  }
#else
  machine_model_ = "Not available";
#endif

  return machine_model_;
}

#if defined(WEBRTC_MAC) && !defined(WEBRTC_IOS)

static CFTypeRef SearchForProperty(io_service_t port, CFStringRef name) {
  return IORegistryEntrySearchCFProperty(port, kIOServicePlane,
      name, kCFAllocatorDefault,
      kIORegistryIterateRecursively | kIORegistryIterateParents);
}

static void GetProperty(io_service_t port, CFStringRef name, int* value) {
  if (!value) return;
  CFTypeRef ref = SearchForProperty(port, name);
  if (ref) {
    CFTypeID refType = CFGetTypeID(ref);
    if (CFNumberGetTypeID() == refType) {
      CFNumberRef number = reinterpret_cast<CFNumberRef>(ref);
      p_convertCFNumberToInt(number, value);
    } else if (CFDataGetTypeID() == refType) {
      CFDataRef data = reinterpret_cast<CFDataRef>(ref);
      if (CFDataGetLength(data) == sizeof(UInt32)) {
        *value = *reinterpret_cast<const UInt32*>(CFDataGetBytePtr(data));
      }
    }
    CFRelease(ref);
  }
}

static void GetProperty(io_service_t port, CFStringRef name,
                        std::string* value) {
  if (!value) return;
  CFTypeRef ref = SearchForProperty(port, name);
  if (ref) {
    CFTypeID refType = CFGetTypeID(ref);
    if (CFStringGetTypeID() == refType) {
      CFStringRef stringRef = reinterpret_cast<CFStringRef>(ref);
      p_convertHostCFStringRefToCPPString(stringRef, *value);
    } else if (CFDataGetTypeID() == refType) {
      CFDataRef dataRef = reinterpret_cast<CFDataRef>(ref);
      *value = std::string(reinterpret_cast<const char*>(
          CFDataGetBytePtr(dataRef)), CFDataGetLength(dataRef));
    }
    CFRelease(ref);
  }
}
#endif



bool SystemInfo::GetGpuInfo(GpuInfo *info) {
  if (!info) return false;
#if defined(WEBRTC_WIN) && !defined(EXCLUDE_D3D9)
  D3DADAPTER_IDENTIFIER9 identifier;
  HRESULT hr = E_FAIL;
  HINSTANCE d3d_lib = LoadLibrary(L"d3d9.dll");

  if (d3d_lib) {
    typedef IDirect3D9* (WINAPI *D3DCreate9Proc)(UINT);
    D3DCreate9Proc d3d_create_proc = reinterpret_cast<D3DCreate9Proc>(
        GetProcAddress(d3d_lib, "Direct3DCreate9"));
    if (d3d_create_proc) {
      IDirect3D9* d3d = d3d_create_proc(D3D_SDK_VERSION);
      if (d3d) {
        hr = d3d->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &identifier);
        d3d->Release();
      }
    }
    FreeLibrary(d3d_lib);
  }

  if (hr != D3D_OK) {
    LOG(LS_ERROR) << "Failed to access Direct3D9 information.";
    return false;
  }

  info->device_name = identifier.DeviceName;
  info->description = identifier.Description;
  info->vendor_id = identifier.VendorId;
  info->device_id = identifier.DeviceId;
  info->driver = identifier.Driver;
  
  std::stringstream ss;
  ss << HIWORD(identifier.DriverVersion.HighPart) << "."
     << LOWORD(identifier.DriverVersion.HighPart) << "."
     << HIWORD(identifier.DriverVersion.LowPart) << "."
     << LOWORD(identifier.DriverVersion.LowPart);
  info->driver_version = ss.str();
  return true;
#elif defined(WEBRTC_MAC) && !defined(WEBRTC_IOS)
  
  io_service_t display_service_port = CGDisplayIOServicePort(
      kCGDirectMainDisplay);
  GetProperty(display_service_port, CFSTR("vendor-id"), &info->vendor_id);
  GetProperty(display_service_port, CFSTR("device-id"), &info->device_id);
  GetProperty(display_service_port, CFSTR("model"), &info->description);
  return true;
#else  
  
  return false;
#endif
}
}  
