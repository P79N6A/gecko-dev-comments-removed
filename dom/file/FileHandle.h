





#ifndef mozilla_dom_file_filehandle_h__
#define mozilla_dom_file_filehandle_h__

#include "FileCommon.h"

#include "nsIDOMFileHandle.h"
#include "nsIFile.h"
#include "nsIFileStorage.h"

#include "nsDOMEventTargetHelper.h"

#include "mozilla/Attributes.h"
#include "mozilla/dom/FileHandleBinding.h"

class nsIDOMFile;
class nsIFileStorage;

namespace mozilla {
namespace dom {
class DOMRequest;
} 
} 

BEGIN_FILE_NAMESPACE

class FileService;
class LockedFile;
class FinishHelper;
class FileHelper;







class FileHandle : public nsDOMEventTargetHelper,
                   public nsIDOMFileHandle
{
  friend class FileService;
  friend class LockedFile;
  friend class FinishHelper;
  friend class FileHelper;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMFILEHANDLE

  nsPIDOMWindow* GetParentObject() const
  {
    return GetOwner();
  }
  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;

  void GetName(nsString& aName) const
  {
    aName = mName;
  }
  void GetType(nsString& aType) const
  {
    aType = mType;
  }
  already_AddRefed<nsIDOMLockedFile> Open(FileMode aMode, ErrorResult& aError);
  already_AddRefed<DOMRequest> GetFile(ErrorResult& aError);
  IMPL_EVENT_HANDLER(abort)
  IMPL_EVENT_HANDLER(error)

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(FileHandle, nsDOMEventTargetHelper)

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

  virtual already_AddRefed<nsISupports>
  CreateStream(nsIFile* aFile, bool aReadOnly) = 0;

  virtual already_AddRefed<nsIDOMFile>
  CreateFileObject(LockedFile* aLockedFile, uint32_t aFileSize) = 0;

protected:
  FileHandle()
  {
    SetIsDOMBinding();
  }

  ~FileHandle()
  { }

  nsCOMPtr<nsIFileStorage> mFileStorage;

  nsString mName;
  nsString mType;

  nsCOMPtr<nsIFile> mFile;
  nsString mFileName;
};

END_FILE_NAMESPACE

#endif 
