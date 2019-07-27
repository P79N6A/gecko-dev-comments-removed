





#ifndef mozilla_dom_FileSystemBase_h
#define mozilla_dom_FileSystemBase_h

#include "nsAutoPtr.h"
#include "nsString.h"

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class BlobImpl;
class Directory;

class FileSystemBase
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(FileSystemBase)
public:

  
  static already_AddRefed<FileSystemBase>
  FromString(const nsAString& aString);

  FileSystemBase();

  virtual void
  Shutdown();

  
  const nsString&
  ToString() const
  {
    return mString;
  }

  virtual nsPIDOMWindow*
  GetWindow() const;

  


  virtual already_AddRefed<nsIFile>
  GetLocalFile(const nsAString& aRealPath) const = 0;

  



  virtual const nsAString&
  GetRootName() const = 0;

  bool
  IsShutdown() const
  {
    return mShutdown;
  }

  virtual bool
  IsSafeFile(nsIFile* aFile) const;

  virtual bool
  IsSafeDirectory(Directory* aDir) const;

  




  virtual bool
  GetRealPath(BlobImpl* aFile, nsAString& aRealPath) const = 0;

  


  const nsCString&
  GetPermission() const
  {
    return mPermission;
  }

  bool
  RequiresPermissionChecks() const
  {
    return mRequiresPermissionChecks;
  }
protected:
  virtual ~FileSystemBase();

  
  nsString mString;

  bool mShutdown;

  
  nsCString mPermission;

  bool mRequiresPermissionChecks;
};

} 
} 

#endif 
