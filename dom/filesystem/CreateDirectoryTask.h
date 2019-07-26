





#ifndef mozilla_dom_CreateDirectoryTask_h
#define mozilla_dom_CreateDirectoryTask_h

#include "mozilla/dom/FileSystemTaskBase.h"
#include "nsAutoPtr.h"

class nsCString;
class nsString;

namespace mozilla {
namespace dom {

class Directory;
class FileSystemBase;
class FileSystemCreateDirectoryParams;
class Promise;

class CreateDirectoryTask MOZ_FINAL
  : public FileSystemTaskBase
{
public:
  CreateDirectoryTask(FileSystemBase* aFileSystem,
                      const nsAString& aPath);
  CreateDirectoryTask(FileSystemBase* aFileSystem,
                      const FileSystemCreateDirectoryParams& aParam,
                      FileSystemRequestParent* aParent);

  virtual
  ~CreateDirectoryTask();

  already_AddRefed<Promise>
  GetPromise();

  virtual void
  GetPermissionAccessType(nsCString& aAccess) const MOZ_OVERRIDE;

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
};

} 
} 

#endif 
