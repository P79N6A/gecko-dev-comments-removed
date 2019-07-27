





#ifndef mozilla_Omnijar_h
#define mozilla_Omnijar_h

#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIFile.h"
#include "nsZipArchive.h"

class nsIURI;

namespace mozilla {

class Omnijar
{
private:
  





  static nsIFile* sPath[2];

  


  static nsZipArchive* sReader[2];

  


  static bool sInitialized;

public:
  enum Type
  {
    GRE = 0,
    APP = 1
  };

  



  static inline bool IsInitialized() { return sInitialized; }

  






  static void Init(nsIFile* aGrePath = nullptr, nsIFile* aAppPath = nullptr);

  


  static void CleanUp();

  




  static inline already_AddRefed<nsIFile> GetPath(Type aType)
  {
    MOZ_ASSERT(IsInitialized(), "Omnijar not initialized");
    nsCOMPtr<nsIFile> path = sPath[aType];
    return path.forget();
  }

  



  static inline bool HasOmnijar(Type aType)
  {
    MOZ_ASSERT(IsInitialized(), "Omnijar not initialized");
    return !!sPath[aType];
  }

  



  static inline already_AddRefed<nsZipArchive> GetReader(Type aType)
  {
    MOZ_ASSERT(IsInitialized(), "Omnijar not initialized");
    nsRefPtr<nsZipArchive> reader = sReader[aType];
    return reader.forget();
  }

  



  static already_AddRefed<nsZipArchive> GetReader(nsIFile* aPath);

  






  static nsresult GetURIString(Type aType, nsACString& aResult);

private:
  


  static void InitOne(nsIFile* aPath, Type aType);
  static void CleanUpOne(Type aType);
}; 

} 

#endif
