





#ifndef mozilla_dom_file_filehandle_h__
#define mozilla_dom_file_filehandle_h__

#include "FileCommon.h"

#include "nsIDOMFileHandle.h"
#include "nsIFile.h"
#include "nsIFileStorage.h"

#include "nsDOMEventTargetHelper.h"

class nsIDOMFile;
class nsIFileStorage;

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
  { }

  ~FileHandle()
  { }

  nsCOMPtr<nsIFileStorage> mFileStorage;

  nsString mName;
  nsString mType;

  nsCOMPtr<nsIFile> mFile;
  nsString mFileName;

  NS_DECL_EVENT_HANDLER(abort)
  NS_DECL_EVENT_HANDLER(error)
};

END_FILE_NAMESPACE

#endif
