





#include "mozilla/dom/OSFileSystem.h"

#include "mozilla/dom/Directory.h"
#include "mozilla/dom/File.h"
#include "mozilla/dom/FileSystemUtils.h"
#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsIFile.h"
#include "nsPIDOMWindow.h"

namespace mozilla {
namespace dom {

OSFileSystem::OSFileSystem(const nsAString& aRootDir)
{
  mLocalRootPath = aRootDir;
  FileSystemUtils::LocalPathToNormalizedPath(mLocalRootPath,
                                             mNormalizedLocalRootPath);

  
  
  mRequiresPermissionChecks = false;

  mString = mLocalRootPath;

#ifdef DEBUG
  mPermission.AssignLiteral("never-used");
#endif
}

void
OSFileSystem::Init(nsPIDOMWindow* aWindow)
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  MOZ_ASSERT(!mWindow, "No duple Init() calls");
  MOZ_ASSERT(aWindow);
  mWindow = aWindow;
}

nsPIDOMWindow*
OSFileSystem::GetWindow() const
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  return mWindow;
}

void
OSFileSystem::GetRootName(nsAString& aRetval) const
{
  return aRetval.AssignLiteral("/");
}

bool
OSFileSystem::IsSafeFile(nsIFile* aFile) const
{
  
  
  
  MOZ_CRASH("Don't use OSFileSystem with the Device Storage API");
  return true;
}

bool
OSFileSystem::IsSafeDirectory(Directory* aDir) const
{
  
  
  
  MOZ_CRASH("Don't use OSFileSystem with the Device Storage API");
  return true;
}

} 
} 
