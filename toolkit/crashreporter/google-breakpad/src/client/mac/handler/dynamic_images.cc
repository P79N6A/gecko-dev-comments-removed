




























#include "client/mac/handler/dynamic_images.h"

extern "C" { 
  #include <mach-o/nlist.h>
  #include <stdlib.h>
  #include <stdio.h>
}

#include <assert.h>
#include <AvailabilityMacros.h>
#include <dlfcn.h>
#include <mach/task_info.h>
#include <sys/sysctl.h>
#include <TargetConditionals.h>

#include <algorithm>
#include <string>
#include <vector>

#include "breakpad_nlist_64.h"

#if !TARGET_OS_IPHONE
#include <CoreServices/CoreServices.h>

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

#endif  

namespace google_breakpad {

using std::string;
using std::vector;









static mach_vm_size_t GetMemoryRegionSize(task_port_t target_task,
                                          const uint64_t address,
                                          mach_vm_size_t *size_to_end) {
  mach_vm_address_t region_base = (mach_vm_address_t)address;
  mach_vm_size_t region_size;
  natural_t nesting_level = 0;
  vm_region_submap_info_64 submap_info;
  mach_msg_type_number_t info_count = VM_REGION_SUBMAP_INFO_COUNT_64;

  
  vm_region_recurse_info_t region_info;
  region_info = reinterpret_cast<vm_region_recurse_info_t>(&submap_info);

  kern_return_t result =
    mach_vm_region_recurse(target_task,
                           &region_base,
                           &region_size,
                           &nesting_level,
                           region_info,
                           &info_count);

  if (result == KERN_SUCCESS) {
    
    *size_to_end = region_base + region_size -(mach_vm_address_t)address;

    
    
    
    
    if (*size_to_end < 4096) {
      
      mach_vm_address_t region_base2 =
        (mach_vm_address_t)(region_base + region_size);
      mach_vm_size_t region_size2;

      
      result =
        mach_vm_region_recurse(target_task,
                               &region_base2,
                               &region_size2,
                               &nesting_level,
                               region_info,
                               &info_count);

      
      if (result == KERN_SUCCESS
          && region_base2 == region_base + region_size) {
        region_size += region_size2;
      }
    }

    *size_to_end = region_base + region_size -(mach_vm_address_t)address;
  } else {
    region_size = 0;
    *size_to_end = 0;
  }

  return region_size;
}

#define kMaxStringLength 8192





static string ReadTaskString(task_port_t target_task,
                             const uint64_t address) {
  
  
  
  
  mach_vm_size_t size_to_end;
  GetMemoryRegionSize(target_task, address, &size_to_end);

  if (size_to_end > 0) {
    mach_vm_size_t size_to_read =
      size_to_end > kMaxStringLength ? kMaxStringLength : size_to_end;

    vector<uint8_t> bytes;
    if (ReadTaskMemory(target_task, address, (size_t)size_to_read, bytes) !=
        KERN_SUCCESS)
      return string();

    return string(reinterpret_cast<const char*>(&bytes[0]));
  }

  return string();
}




kern_return_t ReadTaskMemory(task_port_t target_task,
                             const uint64_t address,
                             size_t length,
                             vector<uint8_t> &bytes) {
  int systemPageSize = getpagesize();

  
  mach_vm_address_t page_address = address & (-systemPageSize);

  mach_vm_address_t last_page_address =
      (address + length + (systemPageSize - 1)) & (-systemPageSize);

  mach_vm_size_t page_size = last_page_address - page_address;
  uint8_t* local_start;
  uint32_t local_length;

  kern_return_t r = mach_vm_read(target_task,
                                 page_address,
                                 page_size,
                                 reinterpret_cast<vm_offset_t*>(&local_start),
                                 &local_length);

  if (r != KERN_SUCCESS)
    return r;

  bytes.resize(length);
  memcpy(&bytes[0],
         &local_start[(mach_vm_address_t)address - page_address],
         length);
  mach_vm_deallocate(mach_task_self(), (uintptr_t)local_start, local_length);
  return KERN_SUCCESS;
}

#pragma mark -




struct MachO32 {
  typedef mach_header mach_header_type;
  typedef segment_command mach_segment_command_type;
  typedef dyld_image_info32 dyld_image_info;
  typedef dyld_all_image_infos32 dyld_all_image_infos;
  typedef struct nlist nlist_type;
  static const uint32_t magic = MH_MAGIC;
  static const uint32_t segment_load_command = LC_SEGMENT;
};

struct MachO64 {
  typedef mach_header_64 mach_header_type;
  typedef segment_command_64 mach_segment_command_type;
  typedef dyld_image_info64 dyld_image_info;
  typedef dyld_all_image_infos64 dyld_all_image_infos;
  typedef struct nlist_64 nlist_type;
  static const uint32_t magic = MH_MAGIC_64;
  static const uint32_t segment_load_command = LC_SEGMENT_64;
};

template<typename MachBits>
bool FindTextSection(DynamicImage& image) {
  typedef typename MachBits::mach_header_type mach_header_type;
  typedef typename MachBits::mach_segment_command_type
      mach_segment_command_type;
  
  const mach_header_type* header =
      reinterpret_cast<const mach_header_type*>(&image.header_[0]);

  if(header->magic != MachBits::magic) {
    return false;
  }

  const struct load_command *cmd =
      reinterpret_cast<const struct load_command *>(header + 1);

  bool found_text_section = false;
  bool found_dylib_id_command = false;
  for (unsigned int i = 0; cmd && (i < header->ncmds); ++i) {
    if (!found_text_section) {
      if (cmd->cmd == MachBits::segment_load_command) {
        const mach_segment_command_type *seg =
            reinterpret_cast<const mach_segment_command_type *>(cmd);

        if (!strcmp(seg->segname, "__TEXT")) {
          image.vmaddr_ = seg->vmaddr;
          image.vmsize_ = seg->vmsize;
          image.slide_ = 0;

          if (seg->fileoff == 0 && seg->filesize != 0) {
            image.slide_ =
                (uintptr_t)image.GetLoadAddress() - (uintptr_t)seg->vmaddr;
          }
          found_text_section = true;
        }
      }
    }

    if (!found_dylib_id_command) {
      if (cmd->cmd == LC_ID_DYLIB) {
        const struct dylib_command *dc =
            reinterpret_cast<const struct dylib_command *>(cmd);

        image.version_ = dc->dylib.current_version;
        found_dylib_id_command = true;
      }
    }

    if (found_dylib_id_command && found_text_section) {
      return true;
    }

    cmd = reinterpret_cast<const struct load_command *>
        (reinterpret_cast<const char *>(cmd) + cmd->cmdsize);
  }

  return false;
}



void DynamicImage::CalculateMemoryAndVersionInfo() {
  
  
  vmaddr_ = 0;
  vmsize_ = 0;
  slide_ = 0;
  version_ = 0;

  
  if (Is64Bit())
    FindTextSection<MachO64>(*this);
  else
    FindTextSection<MachO32>(*this);
}



template<typename MachBits>
uint32_t GetFileTypeFromHeader(DynamicImage& image) {
  typedef typename MachBits::mach_header_type mach_header_type;

  const mach_header_type* header =
      reinterpret_cast<const mach_header_type*>(&image.header_[0]);
  return header->filetype;
}

uint32_t DynamicImage::GetFileType() {
  if (Is64Bit())
    return GetFileTypeFromHeader<MachO64>(*this);

  return GetFileTypeFromHeader<MachO32>(*this);
}

#pragma mark -



DynamicImages::DynamicImages(mach_port_t task)
    : task_(task),
      cpu_type_(DetermineTaskCPUType(task)),
      image_list_() {
  ReadImageInfoForTask();
}

template<typename MachBits>
static uint64_t LookupSymbol(const char* symbol_name,
                             const char* filename,
                             cpu_type_t cpu_type) {
  typedef typename MachBits::nlist_type nlist_type;

  nlist_type symbol_info[8] = {};
  const char *symbolNames[2] = { symbol_name, "\0" };
  nlist_type &list = symbol_info[0];
  int invalidEntriesCount = breakpad_nlist(filename,
                                           &list,
                                           symbolNames,
                                           cpu_type);

  if(invalidEntriesCount != 0) {
    return 0;
  }

  assert(list.n_value);
  return list.n_value;
}

#if TARGET_OS_IPHONE
static bool HasTaskDyldInfo() {
  return true;
}
#else
static SInt32 GetOSVersionInternal() {
  SInt32 os_version = 0;
  Gestalt(gestaltSystemVersion, &os_version);
  return os_version;
}

static SInt32 GetOSVersion() {
  static SInt32 os_version = GetOSVersionInternal();
  return os_version;
}

static bool HasTaskDyldInfo() {
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_6
  return true;
#else
  return GetOSVersion() >= 0x1060;
#endif
}
#endif  

uint64_t DynamicImages::GetDyldAllImageInfosPointer() {
  if (HasTaskDyldInfo()) {
    task_dyld_info_data_t task_dyld_info;
    mach_msg_type_number_t count = TASK_DYLD_INFO_COUNT;
    if (task_info(task_, TASK_DYLD_INFO, (task_info_t)&task_dyld_info,
                  &count) != KERN_SUCCESS) {
      return 0;
    }

    return (uint64_t)task_dyld_info.all_image_info_addr;
  } else {
    const char *imageSymbolName = "_dyld_all_image_infos";
    const char *dyldPath = "/usr/lib/dyld";

    if (Is64Bit())
      return LookupSymbol<MachO64>(imageSymbolName, dyldPath, cpu_type_);
    return LookupSymbol<MachO32>(imageSymbolName, dyldPath, cpu_type_);
  }
}




template<typename MachBits>
void ReadImageInfo(DynamicImages& images,
                   uint64_t image_list_address) {
  typedef typename MachBits::dyld_image_info dyld_image_info;
  typedef typename MachBits::dyld_all_image_infos dyld_all_image_infos;
  typedef typename MachBits::mach_header_type mach_header_type;

  
  

  
  
  
  vector<uint8_t> dyld_all_info_bytes;
  if (ReadTaskMemory(images.task_,
                     image_list_address,
                     sizeof(dyld_all_image_infos),
                     dyld_all_info_bytes) != KERN_SUCCESS)
    return;

  dyld_all_image_infos *dyldInfo =
    reinterpret_cast<dyld_all_image_infos*>(&dyld_all_info_bytes[0]);

  
  int count = dyldInfo->infoArrayCount;

  
  
  vector<uint8_t> dyld_info_array_bytes;
    if (ReadTaskMemory(images.task_,
                       dyldInfo->infoArray,
                       count * sizeof(dyld_image_info),
                       dyld_info_array_bytes) != KERN_SUCCESS)
      return;

    dyld_image_info *infoArray =
        reinterpret_cast<dyld_image_info*>(&dyld_info_array_bytes[0]);
    images.image_list_.reserve(count);

    for (int i = 0; i < count; ++i) {
      dyld_image_info &info = infoArray[i];

      
      vector<uint8_t> mach_header_bytes;
      if (ReadTaskMemory(images.task_,
                         info.load_address_,
                         sizeof(mach_header_type),
                         mach_header_bytes) != KERN_SUCCESS)
        continue;  

      mach_header_type *header =
          reinterpret_cast<mach_header_type*>(&mach_header_bytes[0]);

      
      
      size_t header_size =
          sizeof(mach_header_type) + header->sizeofcmds;

      if (ReadTaskMemory(images.task_,
                         info.load_address_,
                         header_size,
                         mach_header_bytes) != KERN_SUCCESS)
        continue;

      header = reinterpret_cast<mach_header_type*>(&mach_header_bytes[0]);

      
      string file_path;
      if (info.file_path_) {
        
        
        
        file_path = ReadTaskString(images.task_, info.file_path_);
      }

      
      DynamicImage *new_image;
      new_image = new DynamicImage(&mach_header_bytes[0],
                                   header_size,
                                   info.load_address_,
                                   file_path,
                                   info.file_mod_date_,
                                   images.task_,
                                   images.cpu_type_);

      if (new_image->IsValid()) {
        images.image_list_.push_back(DynamicImageRef(new_image));
      } else {
        delete new_image;
      }
    }

    
    sort(images.image_list_.begin(), images.image_list_.end());
    
    
    
    

    vector<DynamicImageRef>::iterator it = unique(images.image_list_.begin(),
                                                  images.image_list_.end());
    images.image_list_.erase(it, images.image_list_.end());
}

void DynamicImages::ReadImageInfoForTask() {
  uint64_t imageList = GetDyldAllImageInfosPointer();

  if (imageList) {
    if (Is64Bit())
      ReadImageInfo<MachO64>(*this, imageList);
    else
      ReadImageInfo<MachO32>(*this, imageList);
  }
}


DynamicImage  *DynamicImages::GetExecutableImage() {
  int executable_index = GetExecutableImageIndex();

  if (executable_index >= 0) {
    return GetImage(executable_index);
  }

  return NULL;
}



int DynamicImages::GetExecutableImageIndex() {
  int image_count = GetImageCount();

  for (int i = 0; i < image_count; ++i) {
    DynamicImage  *image = GetImage(i);
    if (image->GetFileType() == MH_EXECUTE) {
      return i;
    }
  }

  return -1;
}



cpu_type_t DynamicImages::DetermineTaskCPUType(task_t task) {
  if (task == mach_task_self())
    return GetNativeCPUType();

  int mib[CTL_MAXNAME];
  size_t mibLen = CTL_MAXNAME;
  int err = sysctlnametomib("sysctl.proc_cputype", mib, &mibLen);
  if (err == 0) {
    assert(mibLen < CTL_MAXNAME);
    pid_for_task(task, &mib[mibLen]);
    mibLen += 1;

    cpu_type_t cpu_type;
    size_t cpuTypeSize = sizeof(cpu_type);
    sysctl(mib, mibLen, &cpu_type, &cpuTypeSize, 0, 0);
    return cpu_type;
  }

  return GetNativeCPUType();
}

}  
