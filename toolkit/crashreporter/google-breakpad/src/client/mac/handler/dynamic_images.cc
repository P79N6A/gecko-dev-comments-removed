




























#include <mach-o/nlist.h>
#include <stdlib.h>
#include <stdio.h>
#include <algorithm>

#include "client/mac/handler/dynamic_images.h"

namespace google_breakpad {




void* ReadTaskMemory(task_port_t target_task,
                     const void* address,
                     size_t length) {
  void* result = NULL;
  mach_vm_address_t page_address = (uint32_t)address & (-4096);
  mach_vm_address_t last_page_address =
    ((uint32_t)address + length + 4095) & (-4096);
  mach_vm_size_t page_size = last_page_address - page_address;
  uint8_t* local_start;
  uint32_t local_length;

  kern_return_t r = vm_read(target_task,
                            page_address,
                            page_size,
                            reinterpret_cast<vm_offset_t*>(&local_start),
                            &local_length);

  if (r == KERN_SUCCESS) {
    result = malloc(length);
    if (result != NULL) {
      memcpy(result, &local_start[(uint32_t)address - page_address], length);
    }
    vm_deallocate(mach_task_self(), (uintptr_t)local_start, local_length);
  }

  return result;
}

#pragma mark -



void DynamicImage::CalculateMemoryInfo() {
  mach_header *header = GetMachHeader();

  const struct load_command *cmd =
    reinterpret_cast<const struct load_command *>(header + 1);

  for (unsigned int i = 0; cmd && (i < header->ncmds); ++i) {
    if (cmd->cmd == LC_SEGMENT) {
      const struct segment_command *seg =
        reinterpret_cast<const struct segment_command *>(cmd);

      if (!strcmp(seg->segname, "__TEXT")) {
        vmaddr_ = seg->vmaddr;
        vmsize_ = seg->vmsize;
        slide_ = 0;
        
        if (seg->fileoff == 0  &&  seg->filesize != 0) {
          slide_ = (uintptr_t)GetLoadAddress() - (uintptr_t)seg->vmaddr;
        }
        return;
      }
    }

    cmd = reinterpret_cast<const struct load_command *>
      (reinterpret_cast<const char *>(cmd) + cmd->cmdsize);
  }
  
  
  vmaddr_ = 0;
  vmsize_ = 0;
  slide_ = 0;
}

#pragma mark -



DynamicImages::DynamicImages(mach_port_t task)
  : task_(task) {
  ReadImageInfoForTask();
}



void DynamicImages::ReadImageInfoForTask() {
  struct nlist l[8];
  memset(l, 0, sizeof(l) );
  
  
  
  
  struct nlist &list = l[0];
  list.n_un.n_name = "_dyld_all_image_infos";
  nlist("/usr/lib/dyld", &list);
  
  if (list.n_value) {
    
    

    
    
    
    dyld_all_image_infos *dyldInfo = reinterpret_cast<dyld_all_image_infos*>
      (ReadTaskMemory(task_,
                      reinterpret_cast<void*>(list.n_value),
                      sizeof(dyld_all_image_infos)));

    if (dyldInfo) {
      
      int count = dyldInfo->infoArrayCount;

      
      
      dyld_image_info *infoArray = reinterpret_cast<dyld_image_info*>
        (ReadTaskMemory(task_,
                        dyldInfo->infoArray,
                        count*sizeof(dyld_image_info)));

      image_list_.reserve(count);
      
      for (int i = 0; i < count; ++i) {
        dyld_image_info &info = infoArray[i];

        
        mach_header *header = reinterpret_cast<mach_header*>
          (ReadTaskMemory(task_, info.load_address_, sizeof(mach_header)));

        if (!header)
          break;   
        
        
        
        
        unsigned int header_size = sizeof(mach_header) + header->sizeofcmds;
        free(header);

        header = reinterpret_cast<mach_header*>
          (ReadTaskMemory(task_, info.load_address_, header_size));
        
        
        char *file_path = NULL;
        if (info.file_path_) {
          
          
          
          file_path = reinterpret_cast<char*>
            (ReadTaskMemory(task_,
                            info.file_path_,
                            0x2000));
        }
        
        
        DynamicImage *new_image = new DynamicImage(header,
                                                   header_size,
                                                   info.load_address_,
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
