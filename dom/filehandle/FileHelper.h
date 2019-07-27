





#ifndef mozilla_dom_FileHelper_h
#define mozilla_dom_FileHelper_h

#include "js/TypeDecls.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsIRequestObserver.h"

namespace mozilla {
namespace dom {

class FileHandleBase;
class FileHelper;
class FileRequestBase;
class FileOutputStreamWrapper;
class MutableFileBase;

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
  friend class FileOutputStreamWrapper;

public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER

  nsresult
  ResultCode() const
  {
    return mResultCode;
  }

  nsresult
  Enqueue();

  nsresult
  AsyncRun(FileHelperListener* aListener);

  virtual nsresult
  GetSuccessResult(JSContext* aCx, JS::MutableHandle<JS::Value> aVal);

  void
  OnStreamProgress(uint64_t aProgress, uint64_t aProgressMax);

  void
  OnStreamClose();

  void
  OnStreamDestroy();

  static FileHandleBase*
  GetCurrentFileHandle();

protected:
  FileHelper(FileHandleBase* aFileHandle, FileRequestBase* aRequest);

  virtual ~FileHelper();

  virtual nsresult
  DoAsyncRun(nsISupports* aStream) = 0;

  virtual void
  ReleaseObjects();

  void
  Finish();

  nsRefPtr<MutableFileBase> mMutableFile;
  nsRefPtr<FileHandleBase> mFileHandle;
  nsRefPtr<FileRequestBase> mFileRequest;

  nsRefPtr<FileHelperListener> mListener;
  nsCOMPtr<nsIRequest> mRequest;

private:
  nsresult mResultCode;
  bool mFinished;
};

} 
} 

#endif 
