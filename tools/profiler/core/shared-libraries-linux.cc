



#include "shared-libraries.h"

#define PATH_MAX_TOSTRING(x) #x
#define PATH_MAX_STRING(x) PATH_MAX_TOSTRING(x)
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <fstream>
#include "platform.h"
#include "shared-libraries.h"

#include "common/linux/file_id.h"
#include <algorithm>

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))


static std::string getId(const char *bin_name)
{
  using namespace google_breakpad;
  using namespace std;

  uint8_t identifier[kMDGUIDSize];
  char id_str[37]; 

  FileID file_id(bin_name);
  if (file_id.ElfFileIdentifier(identifier)) {
    FileID::ConvertIdentifierToString(identifier, id_str, ARRAY_SIZE(id_str));
    
    
    char *id_end = remove(id_str, id_str + strlen(id_str), '-');
    
    
    return string(id_str, id_end) + '0';
  }

  return "";
}

#if !defined(MOZ_WIDGET_GONK)

#include "nsDebug.h"
#ifdef ANDROID
#include "ElfLoader.h" 
#else
#include <link.h> 
#endif
#include <features.h>
#include <dlfcn.h>
#include <sys/types.h>

#ifdef ANDROID
extern "C" MOZ_EXPORT __attribute__((weak))
int dl_iterate_phdr(
          int (*callback) (struct dl_phdr_info *info,
                           size_t size, void *data),
          void *data);
#endif

int dl_iterate_callback(struct dl_phdr_info *dl_info, size_t size, void *data)
{
  SharedLibraryInfo& info = *reinterpret_cast<SharedLibraryInfo*>(data);

  if (dl_info->dlpi_phnum <= 0)
    return 0;

  unsigned long libStart = -1;
  unsigned long libEnd = 0;

  for (size_t i = 0; i < dl_info->dlpi_phnum; i++) {
    if (dl_info->dlpi_phdr[i].p_type != PT_LOAD) {
      continue;
    }
    unsigned long start = dl_info->dlpi_addr + dl_info->dlpi_phdr[i].p_vaddr;
    unsigned long end = start + dl_info->dlpi_phdr[i].p_memsz;
    if (start < libStart)
      libStart = start;
    if (end > libEnd)
      libEnd = end;
  }
  const char *name = dl_info->dlpi_name;
  SharedLibrary shlib(libStart, libEnd, 0, getId(name), name);
  info.AddSharedLibrary(shlib);

  return 0;
}
#endif

SharedLibraryInfo SharedLibraryInfo::GetInfoForSelf()
{
  SharedLibraryInfo info;

#if !defined(MOZ_WIDGET_GONK)
#ifdef ANDROID
  if (!dl_iterate_phdr) {
    
    
    
    
    return info;
  }
#endif
  dl_iterate_phdr(dl_iterate_callback, &info);
#ifndef ANDROID
  return info;
#endif
#endif

  pid_t pid = getpid();
  char path[PATH_MAX];
  snprintf(path, PATH_MAX, "/proc/%d/maps", pid);
  std::ifstream maps(path);
  std::string line;
  int count = 0;
  while (std::getline(maps, line)) {
    int ret;
    
    unsigned long start;
    unsigned long end;
    char perm[6] = "";
    unsigned long offset;
    char name[PATH_MAX] = "";
    ret = sscanf(line.c_str(),
                 "%lx-%lx %6s %lx %*s %*x %" PATH_MAX_STRING(PATH_MAX) "s\n",
                 &start, &end, perm, &offset, name);
    if (!strchr(perm, 'x')) {
      
      continue;
    }
    if (ret != 5 && ret != 4) {
      LOG("Get maps line failed");
      continue;
    }
#if defined(ANDROID) && !defined(MOZ_WIDGET_GONK)
    
    
    if (strcmp(name, "/dev/ashmem/dalvik-jit-code-cache") != 0)
      continue;
#else
    if (strcmp(perm, "r-xp") != 0) {
      
      
      
      continue;
    }
#endif
    SharedLibrary shlib(start, end, offset, getId(name), name);
    info.AddSharedLibrary(shlib);
    if (count > 10000) {
      LOG("Get maps failed");
      break;
    }
    count++;
  }
  return info;
}
