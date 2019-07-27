





#ifndef mozilla_dom_FileSystemTaskBase_h
#define mozilla_dom_FileSystemTaskBase_h

#include "mozilla/ErrorResult.h"
#include "mozilla/dom/FileSystemRequestParent.h"
#include "mozilla/dom/PFileSystemRequestChild.h"

class nsIDOMFile;

namespace mozilla {
namespace dom {

class BlobParent;
class FileSystemBase;
class FileSystemParams;
























































































class FileSystemTaskBase
  : public nsRunnable
  , public PFileSystemRequestChild
{
public:
  




  void
  Start();

  



  void
  SetError(const nsresult& aErrorCode);

  FileSystemBase*
  GetFileSystem() const;

  


  virtual void
  GetPermissionAccessType(nsCString& aAccess) const = 0;

  NS_DECL_NSIRUNNABLE
protected:
  


  explicit FileSystemTaskBase(FileSystemBase* aFileSystem);

  



  FileSystemTaskBase(FileSystemBase* aFileSystem,
                     const FileSystemParams& aParam,
                     FileSystemRequestParent* aParent);

  virtual
  ~FileSystemTaskBase();

  




  virtual nsresult
  Work() = 0;

  




  virtual void
  HandlerCallback() = 0;

  





  virtual FileSystemParams
  GetRequestParams(const nsString& aFileSystem) const = 0;

  





  virtual FileSystemResponseValue
  GetSuccessRequestResult() const = 0;

  





  virtual void
  SetSuccessRequestResult(const FileSystemResponseValue& aValue) = 0;

  bool
  HasError() const { return mErrorValue != NS_OK; }

  
  virtual bool
  Recv__delete__(const FileSystemResponseValue& value) override;

  BlobParent*
  GetBlobParent(nsIDOMFile* aFile) const;

  nsresult mErrorValue;

  nsRefPtr<FileSystemBase> mFileSystem;
  nsRefPtr<FileSystemRequestParent> mRequestParent;
private:
  




  void
  HandleResult();

  




  FileSystemResponseValue
  GetRequestResult() const;

  




  void
  SetRequestResult(const FileSystemResponseValue& aValue);
};

} 
} 

#endif 
