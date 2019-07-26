



#include "base/sys_info.h"
#include "base/basictypes.h"

#include <errno.h>
#include <string.h>
#ifndef ANDROID
#include <sys/statvfs.h>
#endif
#include <sys/utsname.h>
#include <unistd.h>

#if defined(OS_MACOSX)
#include <mach/mach_host.h>
#include <mach/mach_init.h>
#endif

#if defined(OS_NETBSD)
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

#include "base/logging.h"
#include "base/string_util.h"

namespace base {

int SysInfo::NumberOfProcessors() {
  
  
#ifdef _SC_NPROCESSORS_ONLN
  static long res = sysconf(_SC_NPROCESSORS_ONLN);
#else
  static long res = 1;
#endif
  if (res == -1) {
    NOTREACHED();
    return 1;
  }

  return static_cast<int>(res);
}


int64_t SysInfo::AmountOfPhysicalMemory() {
  
#if defined(OS_MACOSX)
  struct host_basic_info hostinfo;
  mach_msg_type_number_t count = HOST_BASIC_INFO_COUNT;
  int result = host_info(mach_host_self(),
                         HOST_BASIC_INFO,
                         reinterpret_cast<host_info_t>(&hostinfo),
                         &count);
  DCHECK_EQ(HOST_BASIC_INFO_COUNT, count);
  if (result != KERN_SUCCESS) {
    NOTREACHED();
    return 0;
  }

  return static_cast<int64_t>(hostinfo.max_mem);
#elif defined(OS_NETBSD)
  int mib[2];
  int rc;
  int64_t memSize;
  size_t len = sizeof(memSize);

  mib[0] = CTL_HW;
  mib[1] = HW_PHYSMEM64;
  rc = sysctl( mib, 2, &memSize, &len, NULL, 0 );
  if (-1 != rc)  {
    return memSize;
  }
  return 0;

#else
  long pages = sysconf(_SC_PHYS_PAGES);
  long page_size = sysconf(_SC_PAGE_SIZE);
  if (pages == -1 || page_size == -1) {
    NOTREACHED();
    return 0;
  }

  return static_cast<int64_t>(pages) * page_size;
#endif
}


int64_t SysInfo::AmountOfFreeDiskSpace(const std::wstring& path) {
#ifndef ANDROID
  struct statvfs stats;
  if (statvfs(WideToUTF8(path).c_str(), &stats) != 0) {
    return -1;
  }
  return static_cast<int64_t>(stats.f_bavail) * stats.f_frsize;
#else
  return -1;
#endif
}


bool SysInfo::HasEnvVar(const wchar_t* var) {
  std::string var_utf8 = WideToUTF8(std::wstring(var));
  return getenv(var_utf8.c_str()) != NULL;
}


std::wstring SysInfo::GetEnvVar(const wchar_t* var) {
  std::string var_utf8 = WideToUTF8(std::wstring(var));
  char* value = getenv(var_utf8.c_str());
  if (!value) {
    return L"";
  } else {
    return UTF8ToWide(value);
  }
}


std::string SysInfo::OperatingSystemName() {
  utsname info;
  if (uname(&info) < 0) {
    NOTREACHED();
    return "";
  }
  return std::string(info.sysname);
}


std::string SysInfo::OperatingSystemVersion() {
  utsname info;
  if (uname(&info) < 0) {
    NOTREACHED();
    return "";
  }
  return std::string(info.release);
}


std::string SysInfo::CPUArchitecture() {
  utsname info;
  if (uname(&info) < 0) {
    NOTREACHED();
    return "";
  }
  return std::string(info.machine);
}


void SysInfo::GetPrimaryDisplayDimensions(int* width, int* height) {
  NOTIMPLEMENTED();
}


int SysInfo::DisplayCount() {
  NOTIMPLEMENTED();
  return 1;
}


size_t SysInfo::VMAllocationGranularity() {
  return getpagesize();
}

}  
