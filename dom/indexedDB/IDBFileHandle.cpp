





#include "IDBFileHandle.h"

#include "mozilla/dom/file/File.h"
#include "mozilla/dom/quota/FileStreams.h"
#include "nsDOMClassInfoID.h"

#include "IDBDatabase.h"

USING_INDEXEDDB_NAMESPACE
USING_QUOTA_NAMESPACE

namespace {

inline
already_AddRefed<nsIFile>
GetFileFor(FileInfo* aFileInfo)

{
  FileManager* fileManager = aFileInfo->Manager();
  nsCOMPtr<nsIFile> directory = fileManager->GetDirectory();
  NS_ENSURE_TRUE(directory, nullptr);

  nsCOMPtr<nsIFile> file = fileManager->GetFileForId(directory,
                                                     aFileInfo->Id());
  NS_ENSURE_TRUE(file, nullptr);

  return file.forget();
}

} 


already_AddRefed<IDBFileHandle>
IDBFileHandle::Create(IDBDatabase* aDatabase,
                      const nsAString& aName,
                      const nsAString& aType,
                      already_AddRefed<FileInfo> aFileInfo)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsRefPtr<FileInfo> fileInfo(aFileInfo);
  NS_ASSERTION(fileInfo, "Null pointer!");

  nsRefPtr<IDBFileHandle> newFile = new IDBFileHandle();

  newFile->BindToOwner(aDatabase);

  newFile->mFileStorage = aDatabase;
  newFile->mName = aName;
  newFile->mType = aType;

  newFile->mFile = GetFileFor(fileInfo);
  NS_ENSURE_TRUE(newFile->mFile, nullptr);
  newFile->mFileName.AppendInt(fileInfo->Id());

  fileInfo.swap(newFile->mFileInfo);

  return newFile.forget();
}

already_AddRefed<nsISupports>
IDBFileHandle::CreateStream(nsIFile* aFile, bool aReadOnly)
{
  const nsACString& origin = mFileStorage->StorageOrigin();

  nsCOMPtr<nsISupports> result;

  if (aReadOnly) {
    nsRefPtr<FileInputStream> stream = FileInputStream::Create(
      origin, aFile, -1, -1, nsIFileInputStream::DEFER_OPEN);
    result = NS_ISUPPORTS_CAST(nsIFileInputStream*, stream);
  }
  else {
    nsRefPtr<FileStream> stream = FileStream::Create(
      origin, aFile, -1, -1, nsIFileStream::DEFER_OPEN);
    result = NS_ISUPPORTS_CAST(nsIFileStream*, stream);
  }
  NS_ENSURE_TRUE(result, nullptr);

  return result.forget();
}

already_AddRefed<nsIDOMFile>
IDBFileHandle::CreateFileObject(mozilla::dom::file::LockedFile* aLockedFile,
                                uint32_t aFileSize)
{
  nsCOMPtr<nsIDOMFile> file = new mozilla::dom::file::File(
    mName, mType, aFileSize, mFile, aLockedFile, mFileInfo);

  return file.forget();
}

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(IDBFileHandle)
  NS_INTERFACE_MAP_ENTRY(nsIIDBFileHandle)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(IDBFileHandle)
NS_INTERFACE_MAP_END_INHERITING(FileHandle)

NS_IMPL_ADDREF_INHERITED(IDBFileHandle, FileHandle)
NS_IMPL_RELEASE_INHERITED(IDBFileHandle, FileHandle)

DOMCI_DATA(IDBFileHandle, IDBFileHandle)

NS_IMETHODIMP
IDBFileHandle::GetDatabase(nsIIDBDatabase** aDatabase)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsCOMPtr<nsIIDBDatabase> database = do_QueryInterface(mFileStorage);
  NS_ASSERTION(database, "This should always succeed!");

  database.forget(aDatabase);
  return NS_OK;
}
