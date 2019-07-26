





#ifndef mozilla_dom_File_h
#define mozilla_dom_File_h

#include "mozilla/Attributes.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDOMFile.h"

namespace mozilla {
namespace dom {

class LockedFile;

class File : public nsDOMFileCC
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(File, nsDOMFileCC)

  
  File(const nsAString& aName, const nsAString& aContentType,
       uint64_t aLength, nsIFile* aFile, LockedFile* aLockedFile);

  
  File(const nsAString& aName, const nsAString& aContentType,
       uint64_t aLength, nsIFile* aFile, LockedFile* aLockedFile,
       FileInfo* aFileInfo);

  
  NS_IMETHOD
  GetMozFullPathInternal(nsAString& aFullPath) MOZ_OVERRIDE;

  NS_IMETHOD
  GetInternalStream(nsIInputStream** aStream) MOZ_OVERRIDE;

protected:
  
  File(const File* aOther, uint64_t aStart, uint64_t aLength,
       const nsAString& aContentType);

  virtual ~File();

  virtual already_AddRefed<nsIDOMBlob>
  CreateSlice(uint64_t aStart, uint64_t aLength,
              const nsAString& aContentType) MOZ_OVERRIDE;

  virtual bool
  IsStoredFile() const MOZ_OVERRIDE
  {
    return mStoredFile;
  }

  virtual bool
  IsWholeFile() const MOZ_OVERRIDE
  {
    return mWholeFile;
  }

  virtual bool
  IsSnapshot() const MOZ_OVERRIDE
  {
    return true;
  }

private:
  nsCOMPtr<nsIFile> mFile;
  nsRefPtr<LockedFile> mLockedFile;

  bool mWholeFile;
  bool mStoredFile;
};

} 
} 

#endif 
