








































#include "nsIGenericFactory.h"
#include "nsXPCOM.h"
#include "nsIModule.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsMemory.h"
#include "nsAutoPtr.h"

#include "module_list.h"

#define NSGETMODULE(_name) _name##_NSGetModule

#define MODULE(_name) \
NSGETMODULE_ENTRY_POINT(_name) (nsIComponentManager*, nsIFile*, nsIModule**);

MODULES

#undef MODULE

#define MODULE(_name) NSGETMODULE(_name),

static const nsGetModuleProc kGetModules[] = {
    MODULES
};

#undef MODULE

class MetaModule : public nsIModule
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIMODULE

    MetaModule() { }
    nsresult Init(nsIComponentManager*, nsIFile*);

private:
    ~MetaModule() { }

    nsCOMArray<nsIModule> mModules;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(MetaModule, nsIModule)

nsresult
MetaModule::Init(nsIComponentManager* aCompMgr, nsIFile* aLocation)
{
    if (!mModules.SetCapacity(NS_ARRAY_LENGTH(kGetModules)))
        return NS_ERROR_OUT_OF_MEMORY;

    
    nsGetModuleProc const *end = kGetModules + NS_ARRAY_LENGTH(kGetModules);

    nsCOMPtr<nsIModule> module;
    for (nsGetModuleProc const *cur = kGetModules; cur < end; ++cur) {
        nsresult rv = (*cur)(aCompMgr, aLocation, getter_AddRefs(module));
        if (NS_SUCCEEDED(rv))
            mModules.AppendObject(module);
    }

    return NS_OK;
}

NS_IMETHODIMP
MetaModule::GetClassObject(nsIComponentManager* aCompMgr, REFNSCID aCID,
                           REFNSIID aIID, void **aResult)
{
    for (PRInt32 i = mModules.Count() - 1; i >= 0; --i) {
        nsresult rv = mModules[i]->GetClassObject(aCompMgr, aCID,
                                                  aIID, aResult);
        if (NS_SUCCEEDED(rv))
            return rv;
    }

    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
MetaModule::RegisterSelf(nsIComponentManager* aCompMgr, nsIFile* aLocation,
                         const char *aStr, const char *aType)
{
    for (PRInt32 i = mModules.Count() - 1; i >= 0; --i) {
        mModules[i]->RegisterSelf(aCompMgr, aLocation, aStr, aType);
    }
    return NS_OK;
}

NS_IMETHODIMP
MetaModule::UnregisterSelf(nsIComponentManager* aCompMgr, nsIFile* aLocation,
                           const char *aStr)
{
    for (PRInt32 i = mModules.Count() - 1; i >= 0; --i) {
        mModules[i]->UnregisterSelf(aCompMgr, aLocation, aStr);
    }
    return NS_OK;
}

NS_IMETHODIMP
MetaModule::CanUnload(nsIComponentManager* aCompMgr, PRBool *aResult)
{
    for (PRInt32 i = mModules.Count() - 1; i >= 0; --i) {
        nsresult rv = mModules[i]->CanUnload(aCompMgr, aResult);
        if (NS_FAILED(rv))
            return rv;

        
        if (!*aResult)
            return NS_OK;
    }

    return NS_OK;
}

extern "C" NS_EXPORT nsresult
NSGetModule(nsIComponentManager *servMgr,      
            nsIFile *location,                 
            nsIModule **result)                
{                                                                            
    nsRefPtr<MetaModule> mmodule = new MetaModule();
    nsresult rv = mmodule->Init(servMgr, location);
    if (NS_FAILED(rv))
        return rv;

    NS_ADDREF(*result = mmodule);
    return NS_OK;
}
