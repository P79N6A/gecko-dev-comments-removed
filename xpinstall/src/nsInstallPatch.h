




































#ifndef nsInstallPatch_h__
#define nsInstallPatch_h__

#include "prtypes.h"
#include "nsString.h"

#include "nsInstallObject.h"

#include "nsInstall.h"
#include "nsInstallFolder.h"
#include "nsInstallVersion.h"


class nsInstallPatch : public nsInstallObject 
{
    public:

        nsInstallPatch( nsInstall* inInstall,
                        const nsString& inVRName,
                        const nsString& inVInfo,
                        const nsString& inJarLocation,
                        nsInstallFolder* folderSpec,
                        const nsString& inPartialPath,
                        PRInt32 *error);

        nsInstallPatch( nsInstall* inInstall,
                        const nsString& inVRName,
                        const nsString& inVInfo,
                        const nsString& inJarLocation,
                        PRInt32 *error);

        virtual ~nsInstallPatch();

        PRInt32 Prepare();
        PRInt32 Complete();
        void  Abort();
        char* toString();

        PRBool CanUninstall();
        PRBool RegisterPackageNode();
	  
  
    private:
        
        
        nsInstallVersion        *mVersionInfo;
        
        nsCOMPtr<nsIFile>       mTargetFile;
        nsCOMPtr<nsIFile>       mPatchFile;
        nsCOMPtr<nsIFile>       mPatchedFile;

        nsString                *mJarLocation;
        nsString                *mRegistryName;
        
       

        PRInt32  NativePatch(nsIFile *sourceFile, nsIFile *patchfile, nsIFile **newFile);
        void*    HashFilePath(nsIFile* aPath);
};

#endif 
