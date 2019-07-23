




































#ifndef nsNativeModuleLoader_h__
#define nsNativeModuleLoader_h__

#include "nsISupports.h"
#include "nsIModuleLoader.h"
#include "nsVoidArray.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"
#include "nsIModule.h"
#include "prlink.h"

class nsNativeModuleLoader : public nsIModuleLoader
{
 public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIMODULELOADER

    nsNativeModuleLoader() {}
    ~nsNativeModuleLoader() {}

    nsresult Init();

    void UnloadLibraries();

 private:
    struct NativeLoadData
    {
        NativeLoadData() : library(nsnull) { }

        nsCOMPtr<nsIModule>  module;
        PRLibrary           *library;
    };

    static PLDHashOperator
    ReleaserFunc(nsIHashable* aHashedFile, NativeLoadData &aLoadData, void*);

    static PLDHashOperator
    UnloaderFunc(nsIHashable* aHashedFile, NativeLoadData &aLoadData, void*);

    nsDataHashtable<nsHashableHashKey, NativeLoadData> mLibraries;
};

#endif 
