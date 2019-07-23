







































#include "nsInstall.h"
#include "nsInstallUninstall.h"
#include "nsInstallResources.h"
#include "VerReg.h"
#include "nsCRT.h"
#include "prmem.h"
#include "ScheduledTasks.h"
#include "nsReadableUtils.h"
#include "nsILocalFile.h"

extern "C" NS_EXPORT PRInt32 SU_Uninstall(char *regPackageName);
REGERR su_UninstallProcessItem(char *component_path);

nsInstallUninstall::nsInstallUninstall( nsInstall* inInstall,
                                        const nsString& regName,
                                        PRInt32 *error)

: nsInstallObject(inInstall)
{
    MOZ_COUNT_CTOR(nsInstallUninstall);

    if (regName.IsEmpty()) 
    {
        *error = nsInstall::INVALID_ARGUMENTS;
        return;
    }
    
    mRegName.Assign(regName);

    char* userName = (char*)PR_Malloc(MAXREGPATHLEN);
    PRInt32 err = VR_GetUninstallUserName( const_cast<char*>(NS_ConvertUTF16toUTF8(regName).get()),
                                           userName, 
                                           MAXREGPATHLEN );
    
    mUIName.AssignWithConversion(userName);
    
    if (err != REGERR_OK)
    {
        *error = nsInstall::NO_SUCH_COMPONENT;
    }
    
    PR_FREEIF(userName);
    
}

nsInstallUninstall::~nsInstallUninstall()
{
    MOZ_COUNT_DTOR(nsInstallUninstall);
}


PRInt32 nsInstallUninstall::Prepare()
{
    
    return nsInstall::SUCCESS;
}

PRInt32 nsInstallUninstall::Complete()
{
    PRInt32 err = nsInstall::SUCCESS;

    if (mInstall == NULL) 
       return nsInstall::INVALID_ARGUMENTS;

    err = SU_Uninstall( const_cast<char*>(NS_ConvertUTF16toUTF8(mRegName).get()) );
    
    return err;
}

void nsInstallUninstall::Abort()
{
}

char* nsInstallUninstall::toString()
{
    char* buffer = new char[1024];
    char* rsrcVal = nsnull;

    if (buffer == nsnull || !mInstall)
        return buffer;
    
    char* temp = ToNewCString(mUIName);
    
    if (temp)
    {
        rsrcVal = mInstall->GetResourcedString(NS_LITERAL_STRING("Uninstall"));

        if (rsrcVal)
        {
            sprintf( buffer, rsrcVal, temp);
            nsCRT::free(rsrcVal);
        }
    }

    if (temp)
         NS_Free(temp);

    return buffer;
}


PRBool
nsInstallUninstall::CanUninstall()
{
    return PR_FALSE;
}

PRBool
nsInstallUninstall::RegisterPackageNode()
{
    return PR_FALSE;
}

extern "C" NS_EXPORT PRInt32 SU_Uninstall(char *regPackageName)
{
    REGERR status = REGERR_FAIL;
    char pathbuf[MAXREGPATHLEN+1] = {0};
    char sharedfilebuf[MAXREGPATHLEN+1] = {0};
    REGENUM state = 0;
    int32 length;
    int32 err;

    if (regPackageName == NULL)
        return REGERR_PARAM;

    if (pathbuf == NULL)
        return REGERR_PARAM;

    
    status = VR_Enum( regPackageName, &state, pathbuf, MAXREGPATHLEN );

    
    while (status == REGERR_OK)
    {
        char component_path[2*MAXREGPATHLEN+1] = {0};
        strcat(component_path, regPackageName);
        length = strlen(regPackageName);
        if (component_path[length - 1] != '/')
            strcat(component_path, "/");
        strcat(component_path, pathbuf);
        err = su_UninstallProcessItem(component_path);
        status = VR_Enum( regPackageName, &state, pathbuf, MAXREGPATHLEN );  
    }
    
    err = VR_Remove(regPackageName);
    
    
    state = 0;
    status = VR_UninstallEnumSharedFiles( regPackageName, &state, sharedfilebuf, MAXREGPATHLEN );
    while (status == REGERR_OK)
    {
        err = su_UninstallProcessItem(sharedfilebuf);
        err = VR_UninstallDeleteFileFromList(regPackageName, sharedfilebuf);
        status = VR_UninstallEnumSharedFiles( regPackageName, &state, sharedfilebuf, MAXREGPATHLEN );
    }

    err = VR_UninstallDeleteSharedFilesKey(regPackageName);
    err = VR_UninstallDestroy(regPackageName);
    return err;
}

REGERR su_UninstallProcessItem(char *component_path)
{
    int refcount;
    int err;
    char filepath[MAXREGPATHLEN];
    nsCOMPtr<nsILocalFile> nsLFPath;
    nsCOMPtr<nsIFile> nsFPath;

    err = VR_GetPath(component_path, sizeof(filepath), filepath);
    if ( err == REGERR_OK )
    {
        NS_NewNativeLocalFile(nsDependentCString(filepath), PR_TRUE, getter_AddRefs(nsLFPath));
        nsFPath = nsLFPath;
        err = VR_GetRefCount(component_path, &refcount);  
        if ( err == REGERR_OK )
        {
            --refcount;
            if (refcount > 0)
                err = VR_SetRefCount(component_path, refcount);  
            else 
            {
                err = VR_Remove(component_path);
                DeleteFileNowOrSchedule(nsFPath);
            }
        }
        else
        {
            
            err = VR_Remove(component_path);
            DeleteFileNowOrSchedule(nsFPath);
        }
    }
    return err;
}

