





#include "mozilla/dom/DeviceStorageFileSystem.h"

#include "DeviceStorage.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/Directory.h"
#include "mozilla/dom/FileSystemUtils.h"
#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsDeviceStorage.h"
#include "nsIDOMFile.h"
#include "nsIFile.h"
#include "nsPIDOMWindow.h"

namespace mozilla {
namespace dom {

DeviceStorageFileSystem::DeviceStorageFileSystem(
  const nsAString& aStorageType,
  const nsAString& aStorageName)
  : mDeviceStorage(nullptr)
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");

  mStorageType = aStorageType;
  mStorageName = aStorageName;

  
  mString.AppendLiteral("devicestorage-");
  mString.Append(mStorageType);
  mString.Append('-');
  mString.Append(mStorageName);

  mIsTesting =
    mozilla::Preferences::GetBool("device.storage.prompt.testing", false);

  
  nsresult rv =
    DeviceStorageTypeChecker::GetPermissionForType(mStorageType, mPermission);
  NS_WARN_IF(NS_FAILED(rv));

  
  
  
  if (!FileSystemUtils::IsParentProcess()) {
    return;
  }
  nsCOMPtr<nsIFile> rootFile;
  DeviceStorageFile::GetRootDirectoryForType(aStorageType,
                                             aStorageName,
                                             getter_AddRefs(rootFile));

  NS_WARN_IF(!rootFile || NS_FAILED(rootFile->GetPath(mLocalRootPath)));
  FileSystemUtils::LocalPathToNormalizedPath(mLocalRootPath,
    mNormalizedLocalRootPath);

  
  
  
  DebugOnly<DeviceStorageTypeChecker*> typeChecker
    = DeviceStorageTypeChecker::CreateOrGet();
  MOZ_ASSERT(typeChecker);
}

DeviceStorageFileSystem::~DeviceStorageFileSystem()
{
}

void
DeviceStorageFileSystem::Init(nsDOMDeviceStorage* aDeviceStorage)
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  MOZ_ASSERT(aDeviceStorage);
  mDeviceStorage = aDeviceStorage;
}

void
DeviceStorageFileSystem::Shutdown()
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  mDeviceStorage = nullptr;
  mShutdown = true;
}

nsPIDOMWindow*
DeviceStorageFileSystem::GetWindow() const
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  if (!mDeviceStorage) {
    return nullptr;
  }
  return mDeviceStorage->GetOwner();
}

already_AddRefed<nsIFile>
DeviceStorageFileSystem::GetLocalFile(const nsAString& aRealPath) const
{
  MOZ_ASSERT(FileSystemUtils::IsParentProcess(),
             "Should be on parent process!");
  nsAutoString localPath;
  FileSystemUtils::NormalizedPathToLocalPath(aRealPath, localPath);
  localPath = mLocalRootPath + localPath;
  nsCOMPtr<nsIFile> file;
  nsresult rv = NS_NewLocalFile(localPath, false, getter_AddRefs(file));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return nullptr;
  }
  return file.forget();
}

bool
DeviceStorageFileSystem::GetRealPath(nsIDOMFile* aFile, nsAString& aRealPath) const
{
  MOZ_ASSERT(FileSystemUtils::IsParentProcess(),
             "Should be on parent process!");
  MOZ_ASSERT(aFile, "aFile Should not be null.");

  aRealPath.Truncate();

  nsAutoString filePath;
  if (NS_FAILED(aFile->GetMozFullPathInternal(filePath))) {
    return false;
  }

  return LocalPathToRealPath(filePath, aRealPath);
}

const nsAString&
DeviceStorageFileSystem::GetRootName() const
{
  return mStorageName;
}

bool
DeviceStorageFileSystem::IsSafeFile(nsIFile* aFile) const
{
  MOZ_ASSERT(FileSystemUtils::IsParentProcess(),
             "Should be on parent process!");
  MOZ_ASSERT(aFile);

  
  nsAutoString path;
  if (NS_FAILED(aFile->GetPath(path))) {
    return false;
  }
  if (!LocalPathToRealPath(path, path)) {
    return false;
  }

  
  DeviceStorageTypeChecker* typeChecker
    = DeviceStorageTypeChecker::CreateOrGet();
  MOZ_ASSERT(typeChecker);
  return typeChecker->Check(mStorageType, aFile);
}

bool
DeviceStorageFileSystem::IsSafeDirectory(Directory* aDir) const
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  MOZ_ASSERT(aDir);
  nsRefPtr<FileSystemBase> fs = aDir->GetFileSystem();
  MOZ_ASSERT(fs);
  
  return fs->ToString() == mString;
}

bool
DeviceStorageFileSystem::LocalPathToRealPath(const nsAString& aLocalPath,
                                             nsAString& aRealPath) const
{
  nsAutoString path;
  FileSystemUtils::LocalPathToNormalizedPath(aLocalPath, path);
  if (!FileSystemUtils::IsDescendantPath(mNormalizedLocalRootPath, path)) {
    aRealPath.Truncate();
    return false;
  }
  aRealPath = Substring(path, mNormalizedLocalRootPath.Length());
  return true;
}

} 
} 
