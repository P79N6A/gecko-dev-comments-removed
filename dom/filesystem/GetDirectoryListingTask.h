





#ifndef mozilla_dom_GetDirectoryListing_h
#define mozilla_dom_GetDirectoryListing_h

#include "mozilla/dom/FileSystemTaskBase.h"
#include "mozilla/ErrorResult.h"
#include "nsAutoPtr.h"

namespace mozilla {
namespace dom {

class BlobImpl;

class GetDirectoryListingTask final
  : public FileSystemTaskBase
{
public:
  
  GetDirectoryListingTask(FileSystemBase* aFileSystem,
                          const nsAString& aTargetPath,
                          ErrorResult& aRv);
  GetDirectoryListingTask(FileSystemBase* aFileSystem,
                          const FileSystemGetDirectoryListingParams& aParam,
                          FileSystemRequestParent* aParent);

  virtual
  ~GetDirectoryListingTask();

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
  nsString mTargetRealPath;

  
  
  nsTArray<nsRefPtr<BlobImpl>> mTargetBlobImpls;
};

} 
} 

#endif 
