




#include <AvailabilityMacros.h>
#include <mach-o/loader.h>
#include <mach-o/dyld_images.h>
#include <mach/task_info.h>
#include <mach/task.h>
#include <mach/mach_init.h>
#include <mach/mach_traps.h>
#include <string.h>
#include <stdlib.h>
#include <vector>

#include "shared-libraries.h"

#ifndef MAC_OS_X_VERSION_10_6
#define MAC_OS_X_VERSION_10_6 1060
#endif

#if MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_6



#define TASK_DYLD_INFO 17
struct task_dyld_info {
    mach_vm_address_t all_image_info_addr;
    mach_vm_size_t all_image_info_size;
  };
typedef struct task_dyld_info task_dyld_info_data_t;
typedef struct task_dyld_info *task_dyld_info_t;
#define TASK_DYLD_INFO_COUNT (sizeof(task_dyld_info_data_t) / sizeof(natural_t))

#endif


#ifdef __i386__
typedef mach_header platform_mach_header;
typedef segment_command mach_segment_command_type;
#define MACHO_MAGIC_NUMBER MH_MAGIC
#define CMD_SEGMENT LC_SEGMENT
#define seg_size uint32_t
#else
typedef mach_header_64 platform_mach_header;
typedef segment_command_64 mach_segment_command_type;
#define MACHO_MAGIC_NUMBER MH_MAGIC_64
#define CMD_SEGMENT LC_SEGMENT_64
#define seg_size uint64_t
#endif

static
void addSharedLibrary(const platform_mach_header* header, char *name, SharedLibraryInfo &info) {
  const struct load_command *cmd =
    reinterpret_cast<const struct load_command *>(header + 1);

  seg_size size;
  
  for (unsigned int i = 0; cmd && (i < header->ncmds); ++i) {
    if (cmd->cmd == CMD_SEGMENT) {
      const mach_segment_command_type *seg =
        reinterpret_cast<const mach_segment_command_type *>(cmd);

      if (!strcmp(seg->segname, "__TEXT")) {
        size = seg->vmsize;
        unsigned long long start = reinterpret_cast<unsigned long long>(header);
        info.AddSharedLibrary(SharedLibrary(start, start+seg->vmsize, 0,
                                            "", name));
        return;
      }
    }

    cmd = reinterpret_cast<const struct load_command *>
      (reinterpret_cast<const char *>(cmd) + cmd->cmdsize);
  }
}



SharedLibraryInfo SharedLibraryInfo::GetInfoForSelf()
{
  SharedLibraryInfo sharedLibraryInfo;

  task_dyld_info_data_t task_dyld_info;
  mach_msg_type_number_t count = TASK_DYLD_INFO_COUNT;
  if (task_info(mach_task_self (), TASK_DYLD_INFO, (task_info_t)&task_dyld_info,
                &count) != KERN_SUCCESS) {
    return sharedLibraryInfo;
  }

  struct dyld_all_image_infos* aii = (struct dyld_all_image_infos*)task_dyld_info.all_image_info_addr;
  size_t infoCount = aii->infoArrayCount;

  
  
  for (size_t i = 0; i < infoCount; ++i) {
    const dyld_image_info *info = &aii->infoArray[i];

    
    
    if (info->imageLoadAddress->magic != MACHO_MAGIC_NUMBER) {
      continue;
    }

    const platform_mach_header* header =
      reinterpret_cast<const platform_mach_header*>(info->imageLoadAddress);

    
    addSharedLibrary(header, (char*)info->imageFilePath, sharedLibraryInfo);

  }
  return sharedLibraryInfo;
}

