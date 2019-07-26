




#ifndef nsNativeModuleLoader_h__
#define nsNativeModuleLoader_h__

#include "nsISupports.h"
#include "mozilla/ModuleLoader.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"
#include "mozilla/Module.h"
#include "prlink.h"

class nsNativeModuleLoader : public mozilla::ModuleLoader
{
 public:
    NS_DECL_ISUPPORTS_INHERITED

    nsNativeModuleLoader() {}
    ~nsNativeModuleLoader() {}

    virtual const mozilla::Module* LoadModule(mozilla::FileLocation &aFile) MOZ_OVERRIDE;

    nsresult Init();

    void UnloadLibraries();

 private:
    struct NativeLoadData
    {
        NativeLoadData()
            : module(nullptr)
            , library(nullptr)
        { }

        const mozilla::Module* module;
        PRLibrary* library;
    };

    static PLDHashOperator
    ReleaserFunc(nsIHashable* aHashedFile, NativeLoadData &aLoadData, void*);

    static PLDHashOperator
    UnloaderFunc(nsIHashable* aHashedFile, NativeLoadData &aLoadData, void*);

    nsDataHashtable<nsHashableHashKey, NativeLoadData> mLibraries;
};

#endif 
