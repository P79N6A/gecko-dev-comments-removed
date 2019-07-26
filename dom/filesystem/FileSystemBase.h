





#ifndef mozilla_dom_FileSystemBase_h
#define mozilla_dom_FileSystemBase_h

#include "nsWeakReference.h"
#include "nsAutoPtr.h"
#include "nsString.h"

class nsPIDOMWindow; 

namespace mozilla {
namespace dom {







class FileSystemBase
  : public nsSupportsWeakReference
{
  NS_DECL_THREADSAFE_ISUPPORTS
public:

  
  static already_AddRefed<FileSystemBase>
  FromString(const nsAString& aString);

  FileSystemBase();

  
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

  virtual bool
  IsSafeFile(nsIFile* aFile) const;

  


  const nsCString&
  GetPermission() const
  {
    return mPermission;
  }

  bool
  IsTesting() const
  {
    return mIsTesting;
  }
protected:
  virtual ~FileSystemBase();

  
  nsString mString;

  
  nsCString mPermission;

  bool mIsTesting;
};

} 
} 

#endif 
