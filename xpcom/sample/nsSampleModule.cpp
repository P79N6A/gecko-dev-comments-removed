




































#include "nsIGenericFactory.h"
#include "nsIClassInfoImpl.h"

#include "nsSample.h"

























NS_GENERIC_FACTORY_CONSTRUCTOR(nsSampleImpl)








static NS_METHOD nsSampleRegistrationProc(nsIComponentManager *aCompMgr,
                                          nsIFile *aPath,
                                          const char *registryLocation,
                                          const char *componentType,
                                          const nsModuleComponentInfo *info)
{
    
    
    
    

    

    return NS_OK;
}

static NS_METHOD nsSampleUnregistrationProc(nsIComponentManager *aCompMgr,
                                            nsIFile *aPath,
                                            const char *registryLocation,
                                            const nsModuleComponentInfo *info)
{
    
    
    
    

    

    
    return NS_OK;
}


NS_DECL_CLASSINFO(nsSampleImpl)

static const nsModuleComponentInfo components[] =
{
  { "Sample Component", NS_SAMPLE_CID, NS_SAMPLE_CONTRACTID, nsSampleImplConstructor,
    nsSampleRegistrationProc ,
    nsSampleUnregistrationProc ,
    NULL ,
    NS_CI_INTERFACE_GETTER_NAME(nsSampleImpl),
    NULL ,
    &NS_CLASSINFO_NAME(nsSampleImpl)
  }
};










NS_IMPL_NSGETMODULE(nsSampleModule, components)
