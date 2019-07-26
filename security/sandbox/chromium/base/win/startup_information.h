



#ifndef BASE_WIN_STARTUP_INFORMATION_H_
#define BASE_WIN_STARTUP_INFORMATION_H_

#include <windows.h>

#include "base/base_export.h"
#include "base/basictypes.h"

namespace base {
namespace win {


class BASE_EXPORT StartupInformation {
 public:
  StartupInformation();

  ~StartupInformation();

  
  bool InitializeProcThreadAttributeList(DWORD attribute_count);

  
  bool UpdateProcThreadAttribute(DWORD_PTR attribute,
                                 void* value,
                                 size_t size);

  LPSTARTUPINFOW startup_info() { return &startup_info_.StartupInfo; }
  const LPSTARTUPINFOW startup_info() const {
    return const_cast<const LPSTARTUPINFOW>(&startup_info_.StartupInfo);
  }

  bool has_extended_startup_info() const {
    return !!startup_info_.lpAttributeList;
  }

 private:
  STARTUPINFOEXW startup_info_;
  DISALLOW_COPY_AND_ASSIGN(StartupInformation);
};

}  
}  

#endif  

