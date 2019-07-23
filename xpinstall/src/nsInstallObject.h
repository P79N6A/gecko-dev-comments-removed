




































#ifndef nsInstallObject_h__
#define nsInstallObject_h__

#include "prtypes.h"

class nsInstall;

class nsInstallObject 
{
    public:
        
        nsInstallObject(nsInstall* inInstall) {mInstall = inInstall; }
        virtual ~nsInstallObject() {}

        
        virtual PRInt32 Prepare() = 0;

        
        virtual PRInt32 Complete() = 0;

        
        virtual char* toString() = 0;

        
        virtual void Abort() = 0;

        
        virtual PRBool CanUninstall() = 0;
        virtual PRBool RegisterPackageNode() = 0;

    protected:
        nsInstall* mInstall;
};

#endif 
