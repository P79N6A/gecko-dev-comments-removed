



































#ifndef CLIENT_MAC_HANDLER_DYNAMIC_IMAGES_H__
#define CLIENT_MAC_HANDLER_DYNAMIC_IMAGES_H__

#include <mach/mach.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <sys/types.h>
#include <vector>

namespace google_breakpad {

using std::vector;




typedef struct dyld_image_info {
  struct mach_header        *load_address_;
  char                      *file_path_;
  uintptr_t                 file_mod_date_;
} dyld_image_info;





typedef struct dyld_all_image_infos {
  uint32_t                      version;  
  uint32_t                      infoArrayCount;
  const struct dyld_image_info  *infoArray;
  void*                         notification;
  bool                          processDetachedFromSharedRegion;
} dyld_all_image_infos;


#ifdef __LP64__
typedef mach_header_64 breakpad_mach_header;
typedef segment_command_64 breakpad_mach_segment_command;
#else
typedef mach_header breakpad_mach_header;
typedef segment_command breakpad_mach_segment_command;
#endif





class MachHeader {
 public:
  explicit MachHeader(const breakpad_mach_header &header) : header_(header) {}

  void Print() {
    printf("magic\t\t: %4x\n", header_.magic);
    printf("cputype\t\t: %d\n", header_.cputype);
    printf("cpusubtype\t: %d\n", header_.cpusubtype);
    printf("filetype\t: %d\n", header_.filetype);
    printf("ncmds\t\t: %d\n", header_.ncmds);
    printf("sizeofcmds\t: %d\n", header_.sizeofcmds);
    printf("flags\t\t: %d\n", header_.flags);
  }

  breakpad_mach_header   header_;
};



class DynamicImage {
 public:
  DynamicImage(breakpad_mach_header *header, 
               int header_size,              
               breakpad_mach_header *load_address,
               char *inFilePath,
               uintptr_t image_mod_date,
               mach_port_t task)
    : header_(header),
      header_size_(header_size),
      load_address_(load_address),
      file_mod_date_(image_mod_date),
      task_(task) {
    InitializeFilePath(inFilePath);
    CalculateMemoryAndVersionInfo();
  }

  ~DynamicImage() {
    if (file_path_) {
      free(file_path_);
    }
    free(header_);
  }

  
  breakpad_mach_header *GetMachHeader() {return header_;}

  
  int GetHeaderSize() const {return header_size_;}

  
  char *GetFilePath() {return file_path_;}

  uintptr_t GetModDate() const {return file_mod_date_;}

  
  breakpad_mach_header *GetLoadAddress() const {return load_address_;}

  
  mach_vm_address_t GetVMAddr() const {return vmaddr_;}

  
  ptrdiff_t GetVMAddrSlide() const {return slide_;}

  
  mach_vm_size_t GetVMSize() const {return vmsize_;}

  
  mach_port_t GetTask() {return task_;}

  uint32_t GetVersion() {return version_;}
  
  bool operator<(const DynamicImage &inInfo) {
    return GetLoadAddress() < inInfo.GetLoadAddress();
  }

  
  void Print();
 
 private:
  friend class DynamicImages;

  
  bool IsValid() {return GetVMSize() != 0;}

  
  void InitializeFilePath(char *inFilePath) {
    if (inFilePath) {
      size_t path_size = 1 + strlen(inFilePath);
      file_path_ = reinterpret_cast<char*>(malloc(path_size));
      strlcpy(file_path_, inFilePath, path_size);
    } else {
      file_path_ = NULL;
    }
  }

  
  void CalculateMemoryAndVersionInfo();

  breakpad_mach_header    *header_;        
  int                     header_size_;    
  breakpad_mach_header    *load_address_;  
  mach_vm_address_t       vmaddr_;
  mach_vm_size_t          vmsize_;
  ptrdiff_t               slide_;
  uint32_t                version_;        
  char                    *file_path_;     
  uintptr_t               file_mod_date_;  

  mach_port_t             task_;
};







class DynamicImageRef {
 public:
  explicit DynamicImageRef(DynamicImage *inP) : p(inP) {}
  
  DynamicImageRef(const DynamicImageRef &inRef) : p(inRef.p) {}

  bool operator<(const DynamicImageRef &inRef) const {
    return (*const_cast<DynamicImageRef*>(this)->p)
      < (*const_cast<DynamicImageRef&>(inRef).p);
  }

  bool operator==(const DynamicImageRef &inInfo) const {
    return (*const_cast<DynamicImageRef*>(this)->p).GetLoadAddress() ==
        (*const_cast<DynamicImageRef&>(inInfo)).GetLoadAddress();
  }

  
  DynamicImage  *operator->() {return p;}
  operator DynamicImage*() {return p;}

 private:
  DynamicImage  *p;
};





class DynamicImages {
 public:
  explicit DynamicImages(mach_port_t task);

  ~DynamicImages() {
    for (int i = 0; i < (int)image_list_.size(); ++i) {
      delete image_list_[i];
    }
  }

  
  int GetImageCount() const {return image_list_.size();}

  
  DynamicImage *GetImage(int i) {
    if (i < (int)image_list_.size()) {
      return image_list_[i];
    }
    return NULL;
  }

  
  DynamicImage *GetExecutableImage();
  int GetExecutableImageIndex();

  
  mach_port_t GetTask() const {return task_;}

  
  void Print() {
    for (int i = 0; i < (int)image_list_.size(); ++i) {
      image_list_[i]->Print();
    }
  }

  void TestPrint() {
    const breakpad_mach_header *header;
    for (int i = 0; i < (int)image_list_.size(); ++i) {
      printf("dyld: %p: name = %s\n", _dyld_get_image_header(i),
             _dyld_get_image_name(i) );

      const void *imageHeader = _dyld_get_image_header(i);
      header = reinterpret_cast<const breakpad_mach_header*>(imageHeader);

      MachHeader(*header).Print();
    }
  }

 private:
  bool IsOurTask() {return task_ == mach_task_self();}

  
  void ReadImageInfoForTask();
  void* GetDyldAllImageInfosPointer();

  mach_port_t              task_;
  vector<DynamicImageRef>  image_list_;
};



void* ReadTaskMemory(task_port_t target_task,
                     const void* address,
                     size_t len,
                     kern_return_t *kr);

}   

#endif 
