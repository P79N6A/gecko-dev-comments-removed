








































#ifndef nsInstallExecute_h__
#define nsInstallExecute_h__

#include "prtypes.h"
#include "nsString.h"

#include "nsInstallObject.h"

#include "nsInstall.h"
#include "nsIDOMInstallVersion.h"

PRInt32 xpi_PrepareProcessArguments(const char *aArgsString, char **aArgs, PRInt32 aArgsAvailable);


class nsInstallExecute : public nsInstallObject 
{
    public:
          
        nsInstallExecute( nsInstall* inInstall,
                          const nsString& inJarLocation,
                          const nsString& inArgs,
                          const PRBool inBlocking,
                          PRInt32 *error);


        virtual ~nsInstallExecute();

        PRInt32 Prepare();
        PRInt32 Complete();
        void  Abort();
        char* toString();

        PRBool CanUninstall();
        PRBool RegisterPackageNode();
	  
  
    private:
          
        nsString mJarLocation; 
        nsString mArgs;        
        
        nsCOMPtr<nsIFile> mExecutableFile;    
        PRBool mBlocking;
        PRUint32* mPid;

        PRInt32 NativeComplete(void);
        void NativeAbort(void);

};

#endif 
