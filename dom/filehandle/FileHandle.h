





#ifndef mozilla_dom_FileHandle_h
#define mozilla_dom_FileHandle_h

#include "js/TypeDecls.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/FileModeBinding.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "nsCOMPtr.h"
#include "nsString.h"

class nsIDOMFile;
class nsIFile;
class nsIOfflineStorage;
class nsPIDOMWindow;

namespace mozilla {

class ErrorResult;

namespace dom {

class DOMRequest;
class FileService;
class LockedFile;
class FinishHelper;
class FileHelper;

namespace indexedDB {
class FileInfo;
} 








class FileHandle : public DOMEventTargetHelper
{
  friend class FileService;
  friend class LockedFile;
  friend class FinishHelper;
  friend class FileHelper;

public:
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

  virtual bool
  IsShuttingDown();

  virtual bool
  IsInvalid()
  {
    return false;
  }

  
  
  virtual nsIOfflineStorage*
  Storage() = 0;

  virtual already_AddRefed<nsISupports>
  CreateStream(nsIFile* aFile, bool aReadOnly);

  virtual void
  SetThreadLocals()
  {
  }

  virtual void
  UnsetThreadLocals()
  {
  }

  virtual already_AddRefed<nsIDOMFile>
  CreateFileObject(LockedFile* aLockedFile, uint32_t aFileSize);

  
  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
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
  FileHandle(nsPIDOMWindow* aWindow);

  FileHandle(DOMEventTargetHelper* aOwner);

  virtual ~FileHandle();

  nsString mName;
  nsString mType;

  nsCOMPtr<nsIFile> mFile;

  nsCString mStorageId;
  nsString mFileName;
};

} 
} 

#endif 
