





#include <prio.h>
#include "mozilla/GuardObjects.h"

namespace mozilla {
namespace devtools {



















class MOZ_STACK_CLASS AutoMemMap
{
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER;

  PRFileInfo64 fileInfo;
  PRFileDesc*  fd;
  PRFileMap*   fileMap;
  void*        addr;

  AutoMemMap(const AutoMemMap& aOther) = delete;
  void operator=(const AutoMemMap& aOther) = delete;

public:
  explicit AutoMemMap(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM)
      : fd(nullptr)
      , fileMap(nullptr)
      , addr(nullptr)
  {
      MOZ_GUARD_OBJECT_NOTIFIER_INIT;
  };
  ~AutoMemMap();

  
  nsresult init(const char* filePath, PRIntn flags = PR_RDONLY, PRIntn mode = 0,
                PRFileMapProtect prot = PR_PROT_READONLY);

  
  uint32_t size() const {
      MOZ_ASSERT(fileInfo.size <= UINT32_MAX,
                 "Should only call size() if init() succeeded.");
      return uint32_t(fileInfo.size);
  }

  
  void* address() { MOZ_ASSERT(addr); return addr; }
  const void* address() const { MOZ_ASSERT(addr); return addr; }
};

} 
} 
