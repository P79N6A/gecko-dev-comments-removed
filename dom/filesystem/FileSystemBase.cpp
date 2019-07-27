





#include "mozilla/dom/FileSystemBase.h"

#include "DeviceStorageFileSystem.h"
#include "nsCharSeparatedTokenizer.h"

namespace mozilla {
namespace dom {


already_AddRefed<FileSystemBase>
FileSystemBase::FromString(const nsAString& aString)
{
  if (StringBeginsWith(aString, NS_LITERAL_STRING("devicestorage-"))) {
    
    

    nsCharSeparatedTokenizer tokenizer(aString, char16_t('-'));
    tokenizer.nextToken();

    nsString storageType;
    if (tokenizer.hasMoreTokens()) {
      storageType = tokenizer.nextToken();
    }

    nsString storageName;
    if (tokenizer.hasMoreTokens()) {
      storageName = tokenizer.nextToken();
    }

    nsRefPtr<DeviceStorageFileSystem> f =
      new DeviceStorageFileSystem(storageType, storageName);
    return f.forget();
  }
  return nullptr;
}

FileSystemBase::FileSystemBase()
  : mShutdown(false)
  , mRequiresPermissionChecks(true)
{
}

FileSystemBase::~FileSystemBase()
{
}

void
FileSystemBase::Shutdown()
{
  mShutdown = true;
}

nsPIDOMWindow*
FileSystemBase::GetWindow() const
{
  return nullptr;
}

bool
FileSystemBase::IsSafeFile(nsIFile* aFile) const
{
  return false;
}

bool
FileSystemBase::IsSafeDirectory(Directory* aDir) const
{
  return false;
}

} 
} 
