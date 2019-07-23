








































#ifndef nsInstallUninstall_h__
#define nsInstallUninstall_h__

#include "prtypes.h"
#include "nsString.h"
#include "nsInstallObject.h"
#include "nsInstall.h"

class nsInstallUninstall : public nsInstallObject 
{
    public:
          
        nsInstallUninstall( nsInstall* inInstall,
                            const nsString& regName,
                            PRInt32 *error);


        virtual ~nsInstallUninstall();

        PRInt32 Prepare();
        PRInt32 Complete();
        void  Abort();
        char* toString();

        PRBool CanUninstall();
        PRBool RegisterPackageNode();
	  
  
    private:
          
        nsString mRegName;        
        nsString mUIName;         

        
};

#endif 
