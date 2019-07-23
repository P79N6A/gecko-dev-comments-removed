








































#ifndef nsRegisterItem_h__
#define nsRegisterItem_h__

#include "prtypes.h"
#include "nsString.h"
#include "nsInstall.h"
#include "nsInstallObject.h"


class nsRegisterItem : public nsInstallObject 
{
    public:

        nsRegisterItem( nsInstall* install,
                        nsIFile* chrome,
                        PRUint32 chromeType,
                        const char* path );

        virtual ~nsRegisterItem();

        PRInt32 Prepare();
        PRInt32 Complete();
        void  Abort();
        char* toString();

        PRBool CanUninstall();
        PRBool RegisterPackageNode();

    private:
        nsresult GetURLFromIFile(nsIFile *aFile, char **aOutURL);
        
        void LogError(const nsAString& aMessage, nsresult code);
        
        void LogErrorWithFilename(const nsAString& aMessage, nsresult code, nsILocalFile *localFile);

        nsCString mURL;
        nsCOMPtr<nsIFile> mChrome;
        PRUint32  mChromeType;
        nsCOMPtr<nsIFile> mProgDir;
        nsCString mPath; 
};

#endif 
