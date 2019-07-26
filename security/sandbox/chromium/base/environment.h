



#ifndef BASE_ENVIRONMENT_H_
#define BASE_ENVIRONMENT_H_

#include <map>
#include <string>

#include "base/base_export.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/string16.h"
#include "build/build_config.h"

namespace base {

namespace env_vars {

#if defined(OS_POSIX)
BASE_EXPORT extern const char kHome[];
#endif

}  

class BASE_EXPORT Environment {
 public:
  virtual ~Environment();

  
  
  static Environment* Create();

  
  
  virtual bool GetVar(const char* variable_name, std::string* result) = 0;

  
  virtual bool HasVar(const char* variable_name);

  
  virtual bool SetVar(const char* variable_name,
                      const std::string& new_value) = 0;

  
  virtual bool UnSetVar(const char* variable_name) = 0;
};


#if defined(OS_WIN)

typedef string16 NativeEnvironmentString;
typedef std::map<NativeEnvironmentString, NativeEnvironmentString>
    EnvironmentMap;











BASE_EXPORT string16 AlterEnvironment(const wchar_t* env,
                                      const EnvironmentMap& changes);

#elif defined(OS_POSIX)

typedef std::string NativeEnvironmentString;
typedef std::map<NativeEnvironmentString, NativeEnvironmentString>
    EnvironmentMap;








BASE_EXPORT scoped_ptr<char*[]> AlterEnvironment(
    const char* const* env,
    const EnvironmentMap& changes);

#endif

}  

#endif  
