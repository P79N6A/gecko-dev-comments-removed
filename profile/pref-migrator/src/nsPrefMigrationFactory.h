




































#ifndef nsPrefMigrationFactory_h___
#define nsPrefMigrationFactory_h___

#include "nsPrefMigrationCIDs.h"
#include "nsIPrefMigration.h"
#include "nsCOMPtr.h"
#include "nsIModule.h"
#include "nsIGenericFactory.h"


class nsPrefMigrationModule : public nsIModule
{
public:
    nsPrefMigrationModule();
    virtual ~nsPrefMigrationModule();

    NS_DECL_ISUPPORTS

    NS_DECL_NSIMODULE

protected:
    nsresult Initialize();

    void Shutdown();

    PRBool mInitialized;
    nsCOMPtr<nsIGenericFactory> mFactory;
};


#endif
