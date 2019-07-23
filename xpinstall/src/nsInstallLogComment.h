








































#ifndef nsInstallLogComment_h__
#define nsInstallLogComment_h__

#include "prtypes.h"
#include "nsString.h"
#include "nsInstallObject.h"
#include "nsInstall.h"

class nsInstallLogComment : public nsInstallObject 
{
    public:

        nsInstallLogComment(nsInstall* inInstall,
                            const      nsAString&,
                            const      nsAString&,
                            PRInt32    *error);

        virtual ~nsInstallLogComment();

        PRInt32 Prepare();
        PRInt32 Complete();
        void    Abort();
        char*   toString();
        PRBool  CanUninstall();
        PRBool  RegisterPackageNode();
	    
  
    private:

        
        nsString mFileOpCommand;
        nsString mComment;
        
};

#endif 
