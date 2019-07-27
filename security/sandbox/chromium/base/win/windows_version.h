



#ifndef BASE_WIN_WINDOWS_VERSION_H_
#define BASE_WIN_WINDOWS_VERSION_H_

#include <string>

#include "base/base_export.h"
#include "base/basictypes.h"

typedef void* HANDLE;

namespace base {
namespace win {





enum Version {
  VERSION_PRE_XP = 0,  
  VERSION_XP,
  VERSION_SERVER_2003, 
  VERSION_VISTA,       
  VERSION_WIN7,        
  VERSION_WIN8,        
  VERSION_WIN8_1,      
  VERSION_WIN_LAST,    
};




enum VersionType {
  SUITE_HOME,
  SUITE_PROFESSIONAL,
  SUITE_SERVER,
  SUITE_LAST,
};




class BASE_EXPORT OSInfo {
 public:
  struct VersionNumber {
    int major;
    int minor;
    int build;
  };

  struct ServicePack {
    int major;
    int minor;
  };

  
  
  
  
  
  enum WindowsArchitecture {
    X86_ARCHITECTURE,
    X64_ARCHITECTURE,
    IA64_ARCHITECTURE,
    OTHER_ARCHITECTURE,
  };

  
  
  
  
  
  enum WOW64Status {
    WOW64_DISABLED,
    WOW64_ENABLED,
    WOW64_UNKNOWN,
  };

  static OSInfo* GetInstance();

  Version version() const { return version_; }
  
  VersionNumber version_number() const { return version_number_; }
  VersionType version_type() const { return version_type_; }
  ServicePack service_pack() const { return service_pack_; }
  WindowsArchitecture architecture() const { return architecture_; }
  int processors() const { return processors_; }
  size_t allocation_granularity() const { return allocation_granularity_; }
  WOW64Status wow64_status() const { return wow64_status_; }
  std::string processor_model_name();

  
  
  static WOW64Status GetWOW64StatusForProcess(HANDLE process_handle);

 private:
  OSInfo();
  ~OSInfo();

  Version version_;
  VersionNumber version_number_;
  VersionType version_type_;
  ServicePack service_pack_;
  WindowsArchitecture architecture_;
  int processors_;
  size_t allocation_granularity_;
  WOW64Status wow64_status_;
  std::string processor_model_name_;

  DISALLOW_COPY_AND_ASSIGN(OSInfo);
};



BASE_EXPORT Version GetVersion();

}  
}  

#endif  
