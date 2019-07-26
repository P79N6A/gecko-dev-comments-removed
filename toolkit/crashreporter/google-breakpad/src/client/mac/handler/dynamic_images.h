



































#ifndef CLIENT_MAC_HANDLER_DYNAMIC_IMAGES_H__
#define CLIENT_MAC_HANDLER_DYNAMIC_IMAGES_H__

#include <mach/mach.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <sys/types.h>

#include <string>
#include <vector>

#include "mach_vm_compat.h"

namespace google_breakpad {

using std::string;
using std::vector;




typedef struct dyld_image_info32 {
  uint32_t                   load_address_;  
  uint32_t                   file_path_;     
  uint32_t                   file_mod_date_;
} dyld_image_info32;

typedef struct dyld_image_info64 {
  uint64_t                   load_address_;  
  uint64_t                   file_path_;     
  uint64_t                   file_mod_date_;
} dyld_image_info64;





typedef struct dyld_all_image_infos32 {
  uint32_t                      version;  
  uint32_t                      infoArrayCount;
  uint32_t                      infoArray;  
  uint32_t                      notification;
  bool                          processDetachedFromSharedRegion;
} dyld_all_image_infos32;

typedef struct dyld_all_image_infos64 {
  uint32_t                      version;  
  uint32_t                      infoArrayCount;
  uint64_t                      infoArray;  
  uint64_t                      notification;
  bool                          processDetachedFromSharedRegion;
} dyld_all_image_infos64;


#ifdef __LP64__
typedef mach_header_64 breakpad_mach_header;
typedef segment_command_64 breakpad_mach_segment_command;
#else
typedef mach_header breakpad_mach_header;
typedef segment_command breakpad_mach_segment_command;
#endif


class DynamicImage;
template<typename MachBits>
bool FindTextSection(DynamicImage& image);

template<typename MachBits>
uint32_t GetFileTypeFromHeader(DynamicImage& image);



class DynamicImage {
 public:
  DynamicImage(uint8_t *header,     
               size_t header_size,  
               uint64_t load_address,
               string file_path,
               uintptr_t image_mod_date,
               mach_port_t task,
               cpu_type_t cpu_type)
    : header_(header, header + header_size),
      header_size_(header_size),
      load_address_(load_address),
      vmaddr_(0),
      vmsize_(0),
      slide_(0),
      version_(0),
      file_path_(file_path),
      file_mod_date_(image_mod_date),
      task_(task),
      cpu_type_(cpu_type) {
    CalculateMemoryAndVersionInfo();
  }

  
  size_t GetHeaderSize() const {return header_.size();}

  
  string GetFilePath() {return file_path_;}

  uint64_t GetModDate() const {return file_mod_date_;}

  
  uint64_t GetLoadAddress() const {return load_address_;}

  
  mach_vm_address_t GetVMAddr() const {return vmaddr_;}

  
  ptrdiff_t GetVMAddrSlide() const {return slide_;}

  
  mach_vm_size_t GetVMSize() const {return vmsize_;}

  
  mach_port_t GetTask() {return task_;}

  
  cpu_type_t GetCPUType() {return cpu_type_;}

  
  uint32_t GetFileType();

  
  bool Is64Bit() { return (GetCPUType() & CPU_ARCH_ABI64) == CPU_ARCH_ABI64; }

  uint32_t GetVersion() {return version_;}
  
  bool operator<(const DynamicImage &inInfo) {
    return GetLoadAddress() < inInfo.GetLoadAddress();
  }

  
  bool IsValid() {return GetVMSize() != 0;}

 private:
  DynamicImage(const DynamicImage &);
  DynamicImage &operator=(const DynamicImage &);

  friend class DynamicImages;
  template<typename MachBits>
  friend bool FindTextSection(DynamicImage& image);
  template<typename MachBits>
  friend uint32_t GetFileTypeFromHeader(DynamicImage& image);

  
  void CalculateMemoryAndVersionInfo();

  const vector<uint8_t>   header_;        
  size_t                  header_size_;    
  uint64_t                load_address_;   
  mach_vm_address_t       vmaddr_;
  mach_vm_size_t          vmsize_;
  ptrdiff_t               slide_;
  uint32_t                version_;        
  string                  file_path_;     
  uintptr_t               file_mod_date_;  

  mach_port_t             task_;
  cpu_type_t              cpu_type_;        
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


class DynamicImages;
template<typename MachBits>
void ReadImageInfo(DynamicImages& images, uint64_t image_list_address);





class DynamicImages {
 public:
  explicit DynamicImages(mach_port_t task);

  ~DynamicImages() {
    for (int i = 0; i < GetImageCount(); ++i) {
      delete image_list_[i];
    }
  }

  
  int GetImageCount() const {return static_cast<int>(image_list_.size());}

  
  DynamicImage *GetImage(int i) {
    if (i < (int)image_list_.size()) {
      return image_list_[i];
    }
    return NULL;
  }

  
  DynamicImage *GetExecutableImage();
  int GetExecutableImageIndex();

  
  mach_port_t GetTask() const {return task_;}

  
  cpu_type_t GetCPUType() {return cpu_type_;}

  
  bool Is64Bit() { return (GetCPUType() & CPU_ARCH_ABI64) == CPU_ARCH_ABI64; }

  
  static cpu_type_t DetermineTaskCPUType(task_t task);

  
  static cpu_type_t GetNativeCPUType() {
#if defined(__i386__)
    return CPU_TYPE_I386;
#elif defined(__x86_64__)
    return CPU_TYPE_X86_64;
#elif defined(__ppc__)
    return CPU_TYPE_POWERPC;
#elif defined(__ppc64__)
    return CPU_TYPE_POWERPC64;
#elif defined(__arm__)
    return CPU_TYPE_ARM;
#else
#error "GetNativeCPUType not implemented for this architecture"
#endif
  }

 private:
  template<typename MachBits>
  friend void ReadImageInfo(DynamicImages& images, uint64_t image_list_address);

  bool IsOurTask() {return task_ == mach_task_self();}

  
  void ReadImageInfoForTask();
  uint64_t GetDyldAllImageInfosPointer();

  mach_port_t              task_;
  cpu_type_t               cpu_type_;  
  vector<DynamicImageRef>  image_list_;
};



kern_return_t ReadTaskMemory(task_port_t target_task,
                             const uint64_t address,
                             size_t length,
                             vector<uint8_t> &bytes);

}   

#endif 
