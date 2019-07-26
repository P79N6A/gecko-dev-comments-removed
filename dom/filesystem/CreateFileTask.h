





#ifndef mozilla_dom_CreateFileTask_h
#define mozilla_dom_CreateFileTask_h

#include "mozilla/dom/FileSystemTaskBase.h"
#include "nsAutoPtr.h"

class nsIDOMBlob;
class nsIInputStream;

namespace mozilla {
namespace dom {

class Promise;

class CreateFileTask MOZ_FINAL
  : public FileSystemTaskBase
{
public:
  CreateFileTask(FileSystemBase* aFileSystem,
                 const nsAString& aPath,
                 nsIDOMBlob* aBlobData,
                 InfallibleTArray<uint8_t>& aArrayData,
                 bool replace);
  CreateFileTask(FileSystemBase* aFileSystem,
                 const FileSystemCreateFileParams& aParam,
                 FileSystemRequestParent* aParent);

  virtual
  ~CreateFileTask();

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

  virtual nsresult
  Work() MOZ_OVERRIDE;

  virtual void
  HandlerCallback() MOZ_OVERRIDE;

private:
  void
  GetOutputBufferSize() const;

  static uint32_t sOutputBufferSize;
  nsRefPtr<Promise> mPromise;
  nsString mTargetRealPath;
  nsCOMPtr<nsIDOMBlob> mBlobData;
  nsCOMPtr<nsIInputStream> mBlobStream;
  InfallibleTArray<uint8_t> mArrayData;
  bool mReplace;
  nsCOMPtr<nsIDOMFile> mTargetFile;
};

} 
} 

#endif 
