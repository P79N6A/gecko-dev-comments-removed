





#ifndef mozilla_dom_GetFileOrDirectory_h
#define mozilla_dom_GetFileOrDirectory_h

#include "mozilla/dom/FileSystemTaskBase.h"
#include "nsAutoPtr.h"

class nsIDOMFile;
class nsString;

namespace mozilla {
namespace dom {

class FileSystemBase;
class FileSystemFile;
class FileSystemGetFileOrDirectoryParams;
class Promise;

class GetFileOrDirectoryTask MOZ_FINAL
  : public FileSystemTaskBase
{
public:
  
  GetFileOrDirectoryTask(FileSystemBase* aFileSystem,
                         const nsAString& aTargetPath,
                         bool aDirectoryOnly);
  GetFileOrDirectoryTask(FileSystemBase* aFileSystem,
                         const FileSystemGetFileOrDirectoryParams& aParam,
                         FileSystemRequestParent* aParent);

  virtual
  ~GetFileOrDirectoryTask();

  already_AddRefed<Promise>
  GetPromise();

protected:
  virtual FileSystemParams
  GetRequestParams(const nsString& aFileSystem) const MOZ_OVERRIDE;

  virtual FileSystemResponseValue
  GetSuccessRequestResult() const MOZ_OVERRIDE;

  virtual void
  SetSuccessRequestResult(const FileSystemResponseValue& aValue) MOZ_OVERRIDE;

  virtual void
  Work() MOZ_OVERRIDE;

  virtual void
  HandlerCallback() MOZ_OVERRIDE;

private:
  nsRefPtr<Promise> mPromise;
  nsString mTargetRealPath;
  
  bool mIsDirectory;
  nsCOMPtr<nsIDOMFile> mTargetFile;
};

} 
} 

#endif 
