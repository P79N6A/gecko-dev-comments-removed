





#ifndef mozilla_dom_RemoveTask_h
#define mozilla_dom_RemoveTask_h

#include "mozilla/dom/FileSystemTaskBase.h"
#include "nsAutoPtr.h"
#include "mozilla/ErrorResult.h"

namespace mozilla {
namespace dom {

class FileImpl;
class Promise;

class RemoveTask final
  : public FileSystemTaskBase
{
public:
  RemoveTask(FileSystemBase* aFileSystem,
             const nsAString& aDirPath,
             FileImpl* aTargetFile,
             const nsAString& aTargetPath,
             bool aRecursive,
             ErrorResult& aRv);
  RemoveTask(FileSystemBase* aFileSystem,
             const FileSystemRemoveParams& aParam,
             FileSystemRequestParent* aParent);

  virtual
  ~RemoveTask();

  already_AddRefed<Promise>
  GetPromise();

  virtual void
  GetPermissionAccessType(nsCString& aAccess) const override;

protected:
  virtual FileSystemParams
  GetRequestParams(const nsString& aFileSystem) const override;

  virtual FileSystemResponseValue
  GetSuccessRequestResult() const override;

  virtual void
  SetSuccessRequestResult(const FileSystemResponseValue& aValue) override;

  virtual nsresult
  Work() override;

  virtual void
  HandlerCallback() override;

private:
  nsRefPtr<Promise> mPromise;
  nsString mDirRealPath;
  
  
  nsRefPtr<FileImpl> mTargetFileImpl;
  nsString mTargetRealPath;
  bool mRecursive;
  bool mReturnValue;
};

} 
} 

#endif 
