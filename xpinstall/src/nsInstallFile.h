








































#ifndef nsInstallFile_h__
#define nsInstallFile_h__

#include "prtypes.h"
#include "nsString.h"

#include "nsInstallObject.h"

#include "nsInstall.h"
#include "nsInstallVersion.h"



#define INSTALL_NO_COMPARE        0x1
#define INSTALL_IF_NEWER          0x2
#define INSTALL_IF_EQUAL_OR_NEWER 0x4


class nsInstallFile : public nsInstallObject 
{
    public:

      









        nsInstallFile(  nsInstall* inInstall,
                        const nsString& inVRName,
                        const nsString& inVInfo,
                        const nsString& inJarLocation,
                        nsInstallFolder *folderSpec,
                        const nsString& inPartialPath,
                        PRInt32 mode,
                        PRBool  bRegister,
                        PRInt32 *error);

        virtual ~nsInstallFile();

        PRInt32 Prepare();
        PRInt32 Complete();
        void  Abort();
        char* toString();

        PRBool CanUninstall();
        PRBool RegisterPackageNode();


    private:

        
        nsString*         mVersionInfo;	  
        
        nsString*         mJarLocation;	      
        nsCOMPtr<nsIFile> mExtractedFile;	  
        nsCOMPtr<nsIFile> mFinalFile;	      

        nsString*   mVersionRegistryName; 

        PRBool      mReplaceFile;    
        PRBool      mRegister;       
        PRUint32    mFolderCreateCount; 
        
        PRInt32    mMode;            


        PRInt32     CompleteFileMove();
        void        CreateAllFolders(nsInstall *inInstall, nsIFile *inFolderPath, PRInt32 *error);
    

};

#endif 
