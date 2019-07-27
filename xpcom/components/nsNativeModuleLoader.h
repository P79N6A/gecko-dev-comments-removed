





#ifndef nsNativeModuleLoader_h__
#define nsNativeModuleLoader_h__

#include "nsDataHashtable.h"
#include "nsHashKeys.h"
#include "prlink.h"

namespace mozilla {
class FileLocation;
}

class nsNativeModuleLoader final
{
public:
  const mozilla::Module* LoadModule(mozilla::FileLocation& aFile);

  nsresult Init();

  void UnloadLibraries();

private:
  struct NativeLoadData
  {
    NativeLoadData() : mModule(nullptr), mLibrary(nullptr) {}

    const mozilla::Module* mModule;
    PRLibrary* mLibrary;
  };

  static PLDHashOperator
  ReleaserFunc(nsIHashable* aHashedFile, NativeLoadData& aLoadData, void*);

  static PLDHashOperator
  UnloaderFunc(nsIHashable* aHashedFile, NativeLoadData& aLoadData, void*);

  nsDataHashtable<nsHashableHashKey, NativeLoadData> mLibraries;
};

#endif 
