





#ifndef mozilla_dom_FileSystemPermissionRequest_h
#define mozilla_dom_FileSystemPermissionRequest_h

#include "nsAutoPtr.h"
#include "nsIRunnable.h"
#include "nsIContentPermissionPrompt.h"
#include "nsString.h"

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class FileSystemTaskBase;

class FileSystemPermissionRequest final
  : public nsIContentPermissionRequest
  , public nsIRunnable
{
public:
  
  static void
  RequestForTask(FileSystemTaskBase* aTask);

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSICONTENTPERMISSIONREQUEST
  NS_DECL_NSIRUNNABLE
private:
  explicit FileSystemPermissionRequest(FileSystemTaskBase* aTask);

  virtual
  ~FileSystemPermissionRequest();

  nsCString mPermissionType;
  nsCString mPermissionAccess;
  nsRefPtr<FileSystemTaskBase> mTask;
  nsCOMPtr<nsPIDOMWindow> mWindow;
  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsCOMPtr<nsIContentPermissionRequester> mRequester;
};

} 
} 

#endif 
