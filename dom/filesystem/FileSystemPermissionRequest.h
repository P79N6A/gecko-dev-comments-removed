





#ifndef mozilla_dom_FileSystemPermissionRequest_h
#define mozilla_dom_FileSystemPermissionRequest_h

#include "PCOMContentPermissionRequestChild.h"
#include "nsAutoPtr.h"
#include "nsContentPermissionHelper.h"
#include "nsIRunnable.h"

class nsCString;
class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class FileSystemTaskBase;

class FileSystemPermissionRequest MOZ_FINAL
  : public nsIContentPermissionRequest
  , public nsIRunnable
  , public PCOMContentPermissionRequestChild
{
public:
  
  static void
  RequestForTask(FileSystemTaskBase* aTask);

  

  virtual void
  IPDLRelease() MOZ_OVERRIDE;

  bool
  Recv__delete__(const bool& aAllow,
    const InfallibleTArray<PermissionChoice>& aChoices) MOZ_OVERRIDE;

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSICONTENTPERMISSIONREQUEST
  NS_DECL_NSIRUNNABLE
private:
  FileSystemPermissionRequest(FileSystemTaskBase* aTask);

  virtual
  ~FileSystemPermissionRequest();

  nsCString mPermissionType;
  nsCString mPermissionAccess;
  nsRefPtr<FileSystemTaskBase> mTask;
  nsCOMPtr<nsPIDOMWindow> mWindow;
  nsCOMPtr<nsIPrincipal> mPrincipal;
};

} 
} 

#endif 
