





#ifndef mozilla_dom_file_filehandle_h__
#define mozilla_dom_file_filehandle_h__

#include "FileCommon.h"

#include "nsIFile.h"
#include "nsIFileStorage.h"

#include "mozilla/Attributes.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/FileModeBinding.h"

class nsIDOMFile;
class nsIDOMLockedFile;
class nsIFileStorage;
class nsPIDOMWindow;

namespace mozilla {
namespace dom {
class DOMRequest;
namespace indexedDB {
class FileInfo;
} 
} 
} 

BEGIN_FILE_NAMESPACE

class FileService;
class LockedFile;
class FinishHelper;
class FileHelper;







class FileHandle : public DOMEventTargetHelper
{
  friend class FileService;
  friend class LockedFile;
  friend class FinishHelper;
  friend class FileHelper;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(FileHandle, DOMEventTargetHelper)

  static already_AddRefed<FileHandle>
  Create(nsPIDOMWindow* aWindow,
         nsIFileStorage* aFileStorage,
         nsIFile* aFile);

  const nsAString&
  Name() const
  {
    return mName;
  }

  const nsAString&
  Type() const
  {
    return mType;
  }

  virtual int64_t
  GetFileId()
  {
    return -1;
  }

  virtual mozilla::dom::indexedDB::FileInfo*
  GetFileInfo()
  {
    return nullptr;
  }

  virtual already_AddRefed<nsISupports>
  CreateStream(nsIFile* aFile, bool aReadOnly);

  virtual already_AddRefed<nsIDOMFile>
  CreateFileObject(LockedFile* aLockedFile, uint32_t aFileSize);

  
  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  
  nsPIDOMWindow*
  GetParentObject() const
  {
    return GetOwner();
  }

  void
  GetName(nsString& aName) const
  {
    aName = mName;
  }

  void
  GetType(nsString& aType) const
  {
    aType = mType;
  }

  already_AddRefed<LockedFile>
  Open(FileMode aMode, ErrorResult& aError);

  already_AddRefed<DOMRequest>
  GetFile(ErrorResult& aError);

  IMPL_EVENT_HANDLER(abort)
  IMPL_EVENT_HANDLER(error)

protected:
  FileHandle(nsPIDOMWindow* aWindow)
    : DOMEventTargetHelper(aWindow)
  {
  }

  FileHandle(DOMEventTargetHelper* aOwner)
    : DOMEventTargetHelper(aOwner)
  {
  }

  ~FileHandle()
  {
  }

  nsCOMPtr<nsIFileStorage> mFileStorage;

  nsString mName;
  nsString mType;

  nsCOMPtr<nsIFile> mFile;
  nsString mFileName;
};

END_FILE_NAMESPACE

#endif 
