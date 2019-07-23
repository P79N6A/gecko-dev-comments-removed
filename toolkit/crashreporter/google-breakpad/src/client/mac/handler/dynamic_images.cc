




























extern "C" { 
  #include <mach-o/nlist.h>
  #include <stdlib.h>
  #include <stdio.h>
}

#include "breakpad_nlist_64.h"
#include <dlfcn.h>
#include <mach/mach_vm.h>
#include <algorithm>
#include "client/mac/handler/dynamic_images.h"

namespace google_breakpad {









static mach_vm_size_t GetMemoryRegionSize(task_port_t target_task,
                                          const void* address,
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





static void* ReadTaskString(task_port_t target_task,
                            const void* address) {
  
  
  
  
  mach_vm_size_t size_to_end;
  GetMemoryRegionSize(target_task, address, &size_to_end);

  if (size_to_end > 0) {
    mach_vm_size_t size_to_read =
      size_to_end > kMaxStringLength ? kMaxStringLength : size_to_end;

    kern_return_t kr;
    return ReadTaskMemory(target_task, address, size_to_read, &kr);
  }

  return NULL;
}




void* ReadTaskMemory(task_port_t target_task,
                     const void* address,
                     size_t length,
                     kern_return_t *kr) {
  void* result = NULL;
  int systemPageSize = getpagesize();

  
  mach_vm_address_t page_address =
      reinterpret_cast<mach_vm_address_t>(address) & (-systemPageSize);

  mach_vm_address_t last_page_address =
      (reinterpret_cast<mach_vm_address_t>(address) + length +
       (systemPageSize - 1)) & (-systemPageSize);

  mach_vm_size_t page_size = last_page_address - page_address;
  uint8_t* local_start;
  uint32_t local_length;

  kern_return_t r;

  r = mach_vm_read(target_task,
                   page_address,
                   page_size,
                   reinterpret_cast<vm_offset_t*>(&local_start),
                   &local_length);


  if (kr != NULL) {
    *kr = r;
  }

  if (r == KERN_SUCCESS) {
    result = malloc(length);
    if (result != NULL) {
      memcpy(result,
             &local_start[(mach_vm_address_t)address - page_address],
             length);
    }
    mach_vm_deallocate(mach_task_self(), (uintptr_t)local_start, local_length);
  }

  return result;
}

#pragma mark -



void DynamicImage::CalculateMemoryAndVersionInfo() {
  breakpad_mach_header *header = GetMachHeader();

  
  
  vmaddr_ = 0;
  vmsize_ = 0;
  slide_ = 0;
  version_ = 0;

  bool foundTextSection = false;
  bool foundDylibIDCommand = false;
  
#if __LP64__
  if(header->magic != MH_MAGIC_64) {
    return;
  }
#else
  if(header->magic != MH_MAGIC) {
    return;
  }
#endif

#ifdef __LP64__
  const uint32_t segmentLoadCommand = LC_SEGMENT_64;
#else
  const uint32_t segmentLoadCommand = LC_SEGMENT;
#endif

  const struct load_command *cmd =
    reinterpret_cast<const struct load_command *>(header + 1);

  for (unsigned int i = 0; cmd && (i < header->ncmds); ++i) {
    if (!foundTextSection) {
      if (cmd->cmd == segmentLoadCommand) {
        const breakpad_mach_segment_command *seg =
            reinterpret_cast<const breakpad_mach_segment_command *>(cmd);

        if (!strcmp(seg->segname, "__TEXT")) {
          vmaddr_ = seg->vmaddr;
          vmsize_ = seg->vmsize;
          slide_ = 0;

          if (seg->fileoff == 0  &&  seg->filesize != 0) {
            slide_ = (uintptr_t)GetLoadAddress() - (uintptr_t)seg->vmaddr;
          }
          foundTextSection = true;
        }
      }
    }

    if (!foundDylibIDCommand) {
      if (cmd->cmd == LC_ID_DYLIB) {
        const struct dylib_command *dc =
            reinterpret_cast<const struct dylib_command *>(cmd);

        version_ = dc->dylib.current_version;
        foundDylibIDCommand = true;
      }
    }

    if (foundDylibIDCommand && foundTextSection) {
      return;
    }

    cmd = reinterpret_cast<const struct load_command *>
      (reinterpret_cast<const char *>(cmd) + cmd->cmdsize);
  }

}

void DynamicImage::Print() {
  const char *path = GetFilePath();
  if (!path) {
    path = "(unknown)";
  }
  printf("%p: %s\n", GetLoadAddress(), path);
  breakpad_mach_header *header = GetMachHeader();
  MachHeader(*header).Print();
  printf("vmaddr\t\t: %p\n", reinterpret_cast<void*>(GetVMAddr()));
  printf("vmsize\t\t: %llu\n", GetVMSize());
  printf("slide\t\t: %td\n", GetVMAddrSlide());
}

#pragma mark -



DynamicImages::DynamicImages(mach_port_t task)
  : task_(task) {
  ReadImageInfoForTask();
}

void* DynamicImages::GetDyldAllImageInfosPointer()
{

  const char *imageSymbolName = "_dyld_all_image_infos";
  const char *dyldPath = "/usr/lib/dyld";
#ifndef __LP64__
  struct nlist l[8];
  memset(l, 0, sizeof(l) );

  
  
  
  struct nlist &list = l[0];
  list.n_un.n_name = const_cast<char *>(imageSymbolName);
  nlist(dyldPath,&list);
  if(list.n_value) {
    return reinterpret_cast<void*>(list.n_value);
  }

  return NULL;
#else
  struct nlist_64 l[8];
  struct nlist_64 &list = l[0];

  memset(l, 0, sizeof(l) );

  const char *symbolNames[2] = { imageSymbolName, "\0" };

  int invalidEntriesCount = breakpad_nlist_64(dyldPath,&list,symbolNames);

  if(invalidEntriesCount != 0) {
    return NULL;
  }
  assert(list.n_value);
  return reinterpret_cast<void*>(list.n_value);
#endif

}


void DynamicImages::ReadImageInfoForTask() {
  void *imageList = GetDyldAllImageInfosPointer();

  if (imageList) {
    kern_return_t kr;
    
    

    
    
    
    dyld_all_image_infos *dyldInfo = reinterpret_cast<dyld_all_image_infos*>
      (ReadTaskMemory(task_,
                      reinterpret_cast<void*>(imageList),
                      sizeof(dyld_all_image_infos), &kr));

    if (dyldInfo) {
      
      int count = dyldInfo->infoArrayCount;

      
      
      dyld_image_info *infoArray = reinterpret_cast<dyld_image_info*>
        (ReadTaskMemory(task_,
                        dyldInfo->infoArray,
                        count*sizeof(dyld_image_info), &kr));

      image_list_.reserve(count);

      for (int i = 0; i < count; ++i) {
        dyld_image_info &info = infoArray[i];

        
        breakpad_mach_header *header = reinterpret_cast<breakpad_mach_header*>
          (ReadTaskMemory(task_,
                          info.load_address_,
                          sizeof(breakpad_mach_header), &kr));

        if (!header)
          break;   

        
        
        
        unsigned int header_size =
            sizeof(breakpad_mach_header) + header->sizeofcmds;

        free(header);

        header = reinterpret_cast<breakpad_mach_header*>
          (ReadTaskMemory(task_, info.load_address_, header_size, &kr));

        
        char *file_path = NULL;
        if (info.file_path_) {
          
          
          
          file_path = reinterpret_cast<char*>
            (ReadTaskString(task_, info.file_path_));
        }

        
        DynamicImage *new_image;
        new_image = new DynamicImage(header,
                                     header_size,
                                     (breakpad_mach_header*)info.load_address_,
                                     file_path,
                                     info.file_mod_date_,
                                     task_);

        if (new_image->IsValid()) {
          image_list_.push_back(DynamicImageRef(new_image));
        } else {
          delete new_image;
        }

        if (file_path) {
          free(file_path);
        }
      }

      free(dyldInfo);
      free(infoArray);

      
      sort(image_list_.begin(), image_list_.end() );
      
      
      
      

      vector<DynamicImageRef>::iterator it = unique(image_list_.begin(),
                                                    image_list_.end() );
      image_list_.erase(it, image_list_.end());
    }
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
    if (image->GetMachHeader()->filetype == MH_EXECUTE) {
      return i;
    }
  }

  return -1;
}

}  
