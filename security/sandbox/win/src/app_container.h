



#ifndef SANDBOX_WIN_SRC_APP_CONTAINER_H_
#define SANDBOX_WIN_SRC_APP_CONTAINER_H_

#include <windows.h>

#include <vector>

#include "base/memory/scoped_ptr.h"
#include "base/strings/string16.h"
#include "sandbox/win/src/sandbox_types.h"

namespace base {
namespace win {
class StartupInformation;
}
}

namespace sandbox {



class AppContainerAttributes {
 public:
  AppContainerAttributes();
  ~AppContainerAttributes();

  
  ResultCode SetAppContainer(const base::string16& app_container_sid,
                             const std::vector<base::string16>& capabilities);

  
  
  
  
  
  ResultCode ShareForStartup(
      base::win::StartupInformation* startup_information) const;

  bool HasAppContainer() const;

 private:
  SECURITY_CAPABILITIES capabilities_;
  std::vector<SID_AND_ATTRIBUTES> attributes_;

  DISALLOW_COPY_AND_ASSIGN(AppContainerAttributes);
};





ResultCode CreateAppContainer(const base::string16& sid,
                              const base::string16& name);



ResultCode DeleteAppContainer(const base::string16& sid);



base::string16 LookupAppContainer(const base::string16& sid);

}  

#endif  
