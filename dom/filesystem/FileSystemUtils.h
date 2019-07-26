





#ifndef mozilla_dom_FileSystemUtils_h
#define mozilla_dom_FileSystemUtils_h

#include "nsString.h"

namespace mozilla {
namespace dom {

#define FILESYSTEM_DOM_PATH_SEPARATOR "/"





class FileSystemUtils
{
public:
  


  static void
  LocalPathToNormalizedPath(const nsAString& aLocal, nsAString& aNorm);

  



  static void
  NormalizedPathToLocalPath(const nsAString& aNorm, nsAString& aLocal);

  



  static bool
  IsDescendantPath(const nsAString& aPath, const nsAString& aDescendantPath);

  static bool
  IsParentProcess();

  static const char16_t kSeparatorChar = char16_t('/');
};

} 
} 

#endif 
