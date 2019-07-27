





#ifndef mozilla_dom_MutableFile_h
#define mozilla_dom_MutableFile_h

#include "nsCOMPtr.h"
#include "nsString.h"

class nsIFile;
class nsIOfflineStorage;

namespace mozilla {
namespace dom {

class FileService;






class MutableFileBase
{
  friend class FileService;

public:
  NS_IMETHOD_(MozExternalRefCountType)
  AddRef() = 0;

  NS_IMETHOD_(MozExternalRefCountType)
  Release() = 0;

  virtual bool
  IsInvalid()
  {
    return false;
  }

  
  
  virtual nsIOfflineStorage*
  Storage() = 0;

  virtual already_AddRefed<nsISupports>
  CreateStream(bool aReadOnly);


protected:
  MutableFileBase();

  virtual ~MutableFileBase();

  nsCOMPtr<nsIFile> mFile;

  nsCString mStorageId;
  nsString mFileName;
};

} 
} 

#endif 
