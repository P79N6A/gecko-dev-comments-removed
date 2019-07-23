





































#include "nsIModule.h"
#include "nsIGenericFactory.h"

#include "nsPrefMigration.h"
#include "nsPrefMigrationFactory.h"
                          
static NS_IMETHODIMP  
CreateNewPrefMigration(nsISupports* aOuter, REFNSIID aIID, void **aResult)
{                                                  
    if (!aResult) {                                                  
        return NS_ERROR_NULL_POINTER;                             
    }                                                                
    if (aOuter) {                                                    
        *aResult = nsnull;                                           
        return NS_ERROR_NO_AGGREGATION;                              
    }                                                                
    nsPrefMigration* inst = nsPrefMigration::GetInstance();   
    if (inst == nsnull)
      return NS_ERROR_OUT_OF_MEMORY;
                               
    nsresult rv = inst->QueryInterface(aIID, aResult);                        
    if (NS_FAILED(rv)) {                                             
        *aResult = nsnull;                                           
    }                                                                
    return rv;                                                       
}

NS_GENERIC_FACTORY_CONSTRUCTOR(nsPrefConverter)

static const nsModuleComponentInfo components[] = 
{
    { "Profile Migration", 
      NS_PREFMIGRATION_CID,
      NS_PROFILEMIGRATION_CONTRACTID, 
      CreateNewPrefMigration },
    { "Pref Conversion",
      NS_PREFCONVERTER_CID,
      NS_PREFCONVERTER_CONTRACTID, nsPrefConverterConstructor}
};

NS_IMPL_NSGETMODULE(nsPrefMigrationModule, components)
