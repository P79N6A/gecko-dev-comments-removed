



#include "base/sys_info.h"

#include <windows.h>

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "base/string_util.h"

namespace base {


int SysInfo::NumberOfProcessors() {
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  return static_cast<int>(info.dwNumberOfProcessors);
}


int64 SysInfo::AmountOfPhysicalMemory() {
  MEMORYSTATUSEX memory_info;
  memory_info.dwLength = sizeof(memory_info);
  if (!GlobalMemoryStatusEx(&memory_info)) {
    NOTREACHED();
    return 0;
  }

  int64 rv = static_cast<int64>(memory_info.ullTotalPhys);
  if (rv < 0)
    rv = kint64max;
  return rv;
}


int64 SysInfo::AmountOfFreeDiskSpace(const std::wstring& path) {
  ULARGE_INTEGER available, total, free;
  if (!GetDiskFreeSpaceExW(path.c_str(), &available, &total, &free)) {
    return -1;
  }
  int64 rv = static_cast<int64>(available.QuadPart);
  if (rv < 0)
    rv = kint64max;
  return rv;
}


bool SysInfo::HasEnvVar(const wchar_t* var) {
  return GetEnvironmentVariable(var, NULL, 0) != 0;
}


std::wstring SysInfo::GetEnvVar(const wchar_t* var) {
  DWORD value_length = GetEnvironmentVariable(var, NULL, 0);
  if (value_length == 0) {
    return L"";
  }
  scoped_array<wchar_t> value(new wchar_t[value_length]);
  GetEnvironmentVariable(var, value.get(), value_length);
  return std::wstring(value.get());
}


std::string SysInfo::OperatingSystemName() {
  return "Windows NT";
}


std::string SysInfo::OperatingSystemVersion() {
  OSVERSIONINFO info = {0};
  info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  GetVersionEx(&info);

  return StringPrintf("%lu.%lu", info.dwMajorVersion, info.dwMinorVersion);
}






std::string SysInfo::CPUArchitecture() {
  
  return "x86";
}


void SysInfo::GetPrimaryDisplayDimensions(int* width, int* height) {
  if (width)
    *width = GetSystemMetrics(SM_CXSCREEN);

  if (height)
    *height = GetSystemMetrics(SM_CYSCREEN);
}


int SysInfo::DisplayCount() {
  return GetSystemMetrics(SM_CMONITORS);
}


size_t SysInfo::VMAllocationGranularity() {
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);

  return sysinfo.dwAllocationGranularity;
}


void SysInfo::OperatingSystemVersionNumbers(int32 *major_version,
                                            int32 *minor_version,
                                            int32 *bugfix_version) {
  OSVERSIONINFO info = {0};
  info.dwOSVersionInfoSize = sizeof(info);
  GetVersionEx(&info);
  *major_version = info.dwMajorVersion;
  *minor_version = info.dwMinorVersion;
  *bugfix_version = 0;
}

}  
