





#include "SandboxUtil.h"

#include "LinuxCapabilities.h"
#include "LinuxSched.h"
#include "SandboxLogging.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include "mozilla/Assertions.h"
#include "mozilla/unused.h"
#include "sandbox/linux/services/linux_syscalls.h"

namespace mozilla {

bool
IsSingleThreaded()
{
  
  
  
  
  
  struct stat sb;
  if (stat("/proc/self/task", &sb) < 0) {
    MOZ_DIAGNOSTIC_ASSERT(false, "Couldn't access /proc/self/task!");
    return false;
  }
  MOZ_DIAGNOSTIC_ASSERT(sb.st_nlink >= 3);
  return sb.st_nlink == 3;
}

static bool
WriteStringToFile(const char* aPath, const char* aStr, const size_t aLen)
{
  int fd = open(aPath, O_WRONLY);
  if (fd < 0) {
    return false;
  }
  ssize_t written = write(fd, aStr, aLen);
  if (close(fd) != 0 || written != ssize_t(aLen)) {
    return false;
  }
  return true;
}

bool
UnshareUserNamespace()
{
  
  
  uid_t uid = getuid();
  gid_t gid = getgid();
  char buf[80];
  size_t len;

  if (syscall(__NR_unshare, CLONE_NEWUSER) != 0) {
    return false;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  len = size_t(snprintf(buf, sizeof(buf), "%u %u 1\n", uid, uid));
  MOZ_ASSERT(len < sizeof(buf));
  if (!WriteStringToFile("/proc/self/uid_map", buf, len)) {
    MOZ_CRASH("Failed to write /proc/self/uid_map");
  }

  unused << WriteStringToFile("/proc/self/setgroups", "deny", 4);

  len = size_t(snprintf(buf, sizeof(buf), "%u %u 1\n", gid, gid));
  MOZ_ASSERT(len < sizeof(buf));
  if (!WriteStringToFile("/proc/self/gid_map", buf, len)) {
    MOZ_CRASH("Failed to write /proc/self/gid_map");
  }
  return true;
}

} 
