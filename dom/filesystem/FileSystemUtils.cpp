





#include "mozilla/dom/FileSystemUtils.h"

#include "nsXULAppAPI.h"

namespace mozilla {
namespace dom {


void
FileSystemUtils::LocalPathToNormalizedPath(const nsAString& aLocal,
                                           nsAString& aNorm)
{
  nsString result;
  result = aLocal;
#if defined(XP_WIN)
  char16_t* cur = result.BeginWriting();
  char16_t* end = result.EndWriting();
  for (; cur < end; ++cur) {
    if (char16_t('\\') == *cur)
      *cur = char16_t('/');
  }
#endif
  aNorm = result;
}


void
FileSystemUtils::NormalizedPathToLocalPath(const nsAString& aNorm,
                                           nsAString& aLocal)
{
  nsString result;
  result = aNorm;
#if defined(XP_WIN)
  char16_t* cur = result.BeginWriting();
  char16_t* end = result.EndWriting();
  for (; cur < end; ++cur) {
    if (char16_t('/') == *cur)
      *cur = char16_t('\\');
  }
#endif
  aLocal = result;
}


bool
FileSystemUtils::IsDescendantPath(const nsAString& aPath,
                                  const nsAString& aDescendantPath)
{
  
  nsAutoString prefix;
  prefix = aPath + NS_LITERAL_STRING(FILESYSTEM_DOM_PATH_SEPARATOR);

  
  if (aDescendantPath.Length() < prefix.Length() ||
      !StringBeginsWith(aDescendantPath, prefix)) {
    return false;
  }

  return true;
}

} 
} 
