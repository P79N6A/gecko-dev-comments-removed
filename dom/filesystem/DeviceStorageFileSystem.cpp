





#include "mozilla/dom/DeviceStorageFileSystem.h"

#include "DeviceStorage.h"
#include "mozilla/dom/FileSystemUtils.h"
#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsIFile.h"
#include "nsPIDOMWindow.h"

namespace mozilla {
namespace dom {

DeviceStorageFileSystem::DeviceStorageFileSystem(
  const nsAString& aStorageType,
  const nsAString& aStorageName)
  : mDeviceStorage(nullptr)
{
  mStorageType = aStorageType;
  mStorageName = aStorageName;

  
  mString.AppendLiteral("devicestorage-");
  mString.Append(mStorageType);
  mString.AppendLiteral("-");
  mString.Append(mStorageName);

  
  
  
  if (!FileSystemUtils::IsParentProcess()) {
    return;
  }
  nsCOMPtr<nsIFile> rootFile;
  DeviceStorageFile::GetRootDirectoryForType(aStorageType,
                                             aStorageName,
                                             getter_AddRefs(rootFile));

  NS_WARN_IF(!rootFile || NS_FAILED(rootFile->GetPath(mLocalRootPath)));
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

const nsAString&
DeviceStorageFileSystem::GetRootName() const
{
  return mStorageName;
}

} 
} 
