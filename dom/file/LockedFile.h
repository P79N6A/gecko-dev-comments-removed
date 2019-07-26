





#ifndef mozilla_dom_file_lockedfile_h__
#define mozilla_dom_file_lockedfile_h__

#include "mozilla/Attributes.h"
#include "FileCommon.h"
#include "mozilla/dom/FileModeBinding.h"
#include "nsIDOMLockedFile.h"
#include "nsIRunnable.h"

#include "nsDOMEventTargetHelper.h"

class nsIInputStream;

namespace mozilla {
class EventChainPreVisitor;
} 

BEGIN_FILE_NAMESPACE

class FileHandle;
class FileRequest;
class MetadataHelper;

class LockedFile : public nsDOMEventTargetHelper,
                   public nsIDOMLockedFile,
                   public nsIRunnable
{
  friend class FinishHelper;
  friend class FileService;
  friend class FileHelper;
  friend class MetadataHelper;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMLOCKEDFILE
  NS_DECL_NSIRUNNABLE

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(LockedFile, nsDOMEventTargetHelper)

  enum RequestMode
  {
    NORMAL = 0, 
    PARALLEL
  };

  enum ReadyState
  {
    INITIAL = 0,
    LOADING,
    FINISHING,
    DONE
  };

  static already_AddRefed<LockedFile>
  Create(FileHandle* aFileHandle,
         FileMode aMode,
         RequestMode aRequestMode = NORMAL);

  
  virtual nsresult
  PreHandleEvent(EventChainPreVisitor& aVisitor) MOZ_OVERRIDE;

  nsresult
  CreateParallelStream(nsISupports** aStream);

  nsresult
  GetOrCreateStream(nsISupports** aStream);

  bool
  IsOpen() const;

  bool
  IsAborted() const
  {
    return mAborted;
  }

  FileHandle*
  Handle() const
  {
    return mFileHandle;
  }

  nsresult
  OpenInputStream(bool aWholeFile, uint64_t aStart, uint64_t aLength,
                  nsIInputStream** aResult);

private:
  LockedFile();
  ~LockedFile();

  void
  OnNewRequest();

  void
  OnRequestFinished();

  already_AddRefed<FileRequest>
  GenerateFileRequest();

  nsresult
  WriteOrAppend(JS::Handle<JS::Value> aValue, JSContext* aCx,
                nsISupports** _retval, bool aAppend);

  nsresult
  Finish();

  nsRefPtr<FileHandle> mFileHandle;
  ReadyState mReadyState;
  FileMode mMode;
  RequestMode mRequestMode;
  uint64_t mLocation;
  uint32_t mPendingRequests;

  nsTArray<nsCOMPtr<nsISupports> > mParallelStreams;
  nsCOMPtr<nsISupports> mStream;

  bool mAborted;
  bool mCreating;
};

class FinishHelper MOZ_FINAL : public nsIRunnable
{
  friend class LockedFile;

public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIRUNNABLE

private:
  FinishHelper(LockedFile* aLockedFile);
  ~FinishHelper()
  { }

  nsRefPtr<LockedFile> mLockedFile;
  nsTArray<nsCOMPtr<nsISupports> > mParallelStreams;
  nsCOMPtr<nsISupports> mStream;

  bool mAborted;
};

END_FILE_NAMESPACE

#endif 
