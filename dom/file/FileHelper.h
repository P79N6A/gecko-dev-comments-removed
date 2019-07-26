





#ifndef mozilla_dom_file_filehelper_h__
#define mozilla_dom_file_filehelper_h__

#include "FileCommon.h"

#include "nsIRequestObserver.h"

class nsIFileStorage;

BEGIN_FILE_NAMESPACE

class FileHelper;
class FileRequest;
class FileOutputStreamWrapper;
class LockedFile;

class FileHelperListener
{
public:
  NS_IMETHOD_(MozExternalRefCountType)
  AddRef() = 0;

  NS_IMETHOD_(MozExternalRefCountType)
  Release() = 0;

  virtual void
  OnFileHelperComplete(FileHelper* aFileHelper) = 0;
};






class FileHelper : public nsIRequestObserver
{
  friend class FileRequest;
  friend class FileOutputStreamWrapper;

public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER

  nsresult
  Enqueue();

  nsresult
  AsyncRun(FileHelperListener* aListener);

  void
  OnStreamProgress(uint64_t aProgress, uint64_t aProgressMax);

  void
  OnStreamClose();

  void
  OnStreamDestroy();

  static LockedFile*
  GetCurrentLockedFile();

protected:
  FileHelper(LockedFile* aLockedFile, FileRequest* aRequest);

  virtual ~FileHelper();

  virtual nsresult
  DoAsyncRun(nsISupports* aStream) = 0;

  virtual nsresult
  GetSuccessResult(JSContext* aCx, JS::MutableHandle<JS::Value> aVal);

  virtual void
  ReleaseObjects();

  void
  Finish();

  nsCOMPtr<nsIFileStorage> mFileStorage;
  nsRefPtr<LockedFile> mLockedFile;
  nsRefPtr<FileRequest> mFileRequest;

  nsRefPtr<FileHelperListener> mListener;
  nsCOMPtr<nsIRequest> mRequest;

private:
  nsresult mResultCode;
  bool mFinished;
};

END_FILE_NAMESPACE

#endif 
