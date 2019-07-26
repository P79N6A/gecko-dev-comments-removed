





#include "mozilla/dom/FileSystemBase.h"

#include "DeviceStorageFileSystem.h"
#include "nsCharSeparatedTokenizer.h"

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS1(FileSystemBase, nsISupportsWeakReference)


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

    nsCOMPtr<DeviceStorageFileSystem> f =
      new DeviceStorageFileSystem(storageType, storageName);
    return f.forget();
  }
  return nullptr;
}

FileSystemBase::FileSystemBase()
{
}

FileSystemBase::~FileSystemBase()
{
}

nsPIDOMWindow*
FileSystemBase::GetWindow() const
{
  return nullptr;
}

} 
} 
