








































#include "nsCRT.h"
#include "prmem.h"
#include "prprf.h"
#include "VerReg.h"
#include "ScheduledTasks.h"
#include "nsInstallLogComment.h"
#include "nsInstallResources.h"

#include "nsInstall.h"
#include "nsIDOMInstallVersion.h"
#include "nsNativeCharsetUtils.h"

nsInstallLogComment::nsInstallLogComment( nsInstall* inInstall,
                                          const nsAString& inFileOpCommand,
                                          const nsAString& inComment,
                                          PRInt32 *error)

: nsInstallObject(inInstall)
{
    MOZ_COUNT_CTOR(nsInstallLogComment);

    *error = nsInstall::SUCCESS;
    if (inInstall == NULL) 
    {
        *error = nsInstall::INVALID_ARGUMENTS;
        return;
    }
    
    mFileOpCommand = inFileOpCommand;
    mComment       = inComment;
}


nsInstallLogComment::~nsInstallLogComment()
{
    MOZ_COUNT_DTOR(nsInstallLogComment);
}


PRInt32 nsInstallLogComment::Prepare()
{
    
    return nsInstall::SUCCESS;
}

PRInt32 nsInstallLogComment::Complete()
{
    
    return nsInstall::SUCCESS;
}

void nsInstallLogComment::Abort()
{
}

char* nsInstallLogComment::toString()
{
    char* buffer = new char[1024];
    char* rsrcVal = nsnull;
    
    if (buffer == nsnull || !mInstall)
        return nsnull;

    rsrcVal = mInstall->GetResourcedString(mFileOpCommand);
    if (rsrcVal)
    {
        nsCAutoString comment;
        if ( NS_SUCCEEDED( NS_CopyUnicodeToNative(mComment, comment) ) )
          PR_snprintf(buffer, 1024, rsrcVal, comment.get());
        nsCRT::free(rsrcVal);
    }

    return buffer;
}


PRBool
nsInstallLogComment::CanUninstall()
{
    return PR_FALSE;
}

PRBool
nsInstallLogComment::RegisterPackageNode()
{
    return PR_FALSE;
}

