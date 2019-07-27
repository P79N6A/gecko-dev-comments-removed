



#ifndef BASE_PATH_SERVICE_H_
#define BASE_PATH_SERVICE_H_

#include <string>

#include "base/base_export.h"
#include "base/base_paths.h"
#include "base/gtest_prod_util.h"
#include "build/build_config.h"

namespace base {
class FilePath;
class ScopedPathOverride;
}  




class BASE_EXPORT PathService {
 public:
  
  
  
  
  
  
  
  static bool Get(int key, base::FilePath* path);

  
  
  
  
  
  
  
  
  
  
  
  
  
  static bool Override(int key, const base::FilePath& path);

  
  
  
  
  
  
  
  
  
  static bool OverrideAndCreateIfNeeded(int key,
                                        const base::FilePath& path,
                                        bool is_absolute,
                                        bool create);

  
  
  
  
  
  
  
  
  typedef bool (*ProviderFunc)(int, base::FilePath*);

  
  
  static void RegisterProvider(ProviderFunc provider,
                               int key_start,
                               int key_end);

  
  static void DisableCache();

 private:
  friend class base::ScopedPathOverride;
  FRIEND_TEST_ALL_PREFIXES(PathServiceTest, RemoveOverride);

  
  
  
  static bool RemoveOverride(int key);
};

#endif  
