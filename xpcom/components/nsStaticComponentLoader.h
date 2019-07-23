






































#ifndef nsStaticModuleLoader_h__
#define nsStaticModuleLoader_h__

#include "nsXPCOM.h"
#include "pldhash.h"
#include "nsTArray.h"

class nsIModule;
struct StaticModuleInfo;
class nsComponentManagerImpl;
struct DeferredModule;

typedef void (*StaticLoaderCallback)(const char               *key,
                                     nsIModule*                module,
                                     nsTArray<DeferredModule> &deferred);

class nsStaticModuleLoader
{
public:
    nsStaticModuleLoader() :
        mFirst(nsnull)
    { }

    ~nsStaticModuleLoader() { }

    nsresult Init(nsStaticModuleInfo const *aStaticModules,
                  PRUint32 aModuleCount);

    void ReleaseModules() {
        if (mInfoHash.ops)
            PL_DHashTableFinish(&mInfoHash);
    }

    void EnumerateModules(StaticLoaderCallback      cb,
                          nsTArray<DeferredModule> &deferred);
    nsresult GetModuleFor(const char *key, nsIModule* *aResult);

private:
    PLDHashTable                  mInfoHash;
    static PLDHashTableOps        sInfoHashOps;
    StaticModuleInfo             *mFirst;
};

#endif
