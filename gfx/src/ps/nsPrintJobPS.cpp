






































#include "nscore.h"
#include "nsIDeviceContext.h"   
#include "nsIDeviceContextPS.h" 
#include "nsIDeviceContextSpecPS.h"
#include "nsPrintJobPS.h"
#include "nsPSPrinters.h"
#include "nsReadableUtils.h"

#include "prenv.h"
#include "prinit.h"
#include "prlock.h"
#include "prprf.h"

#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>





static PRStatus EnvLock();
static PRStatus EnvSetPrinter(nsCString&);
static void EnvClear();



nsIPrintJobPS::~nsIPrintJobPS() {}


nsresult
nsPrintJobPreviewPS::Init(nsIDeviceContextSpecPS *aSpec)
{
    return NS_OK;
}





nsPrintJobFilePS::nsPrintJobFilePS() : mDestHandle(nsnull) { }


nsPrintJobFilePS::~nsPrintJobFilePS()
{
    if (mDestHandle)
        fclose(mDestHandle);
}





nsresult
nsPrintJobFilePS::Init(nsIDeviceContextSpecPS *aSpec)
{
    NS_PRECONDITION(aSpec, "aSpec must not be NULL");
#ifdef DEBUG
    PRBool toPrinter;
    aSpec->GetToPrinter(toPrinter);
    NS_PRECONDITION(!toPrinter, "This print job is to a printer");
#endif
    const char *path;
    aSpec->GetPath(&path);
    mDestination = path;
    return NS_OK;
}






nsresult
nsPrintJobFilePS::StartSubmission(FILE **aHandle)
{
    NS_PRECONDITION(aHandle, "aHandle is NULL");
    NS_PRECONDITION(!mDestination.IsEmpty(), "No destination");
    NS_PRECONDITION(!mDestHandle, "Already have a destination handle");

    nsCOMPtr<nsILocalFile> destFile;
    nsresult rv = NS_NewNativeLocalFile(GetDestination(),
            PR_FALSE, getter_AddRefs(destFile));
    if (NS_SUCCEEDED(rv))
        rv = destFile->OpenANSIFileDesc("w", &mDestHandle);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_GFX_PRINTER_COULD_NOT_OPEN_FILE);
    NS_POSTCONDITION(mDestHandle,
            "OpenANSIFileDesc succeeded but no file handle");
    *aHandle = mDestHandle;
    return rv;
}





nsresult
nsPrintJobFilePS::FinishSubmission()
{
    NS_PRECONDITION(mDestHandle, "No destination file handle");

    fclose(mDestHandle);
    mDestHandle = nsnull;
    return NS_OK;
}


#ifdef VMS












nsresult
nsPrintJobVMSCmdPS::Init(nsIDeviceContextSpecPS *aSpec)
{
    NS_PRECONDITION(aSpec, "argument must not be NULL");
#ifdef DEBUG
    PRBool toPrinter;
    aSpec->GetToPrinter(toPrinter);
    NS_PRECONDITION(toPrinter, "This print job is not to a printer");
#endif

    
    const char *command;
    aSpec->GetCommand(&command);
    SetDestination(command);

    
    const char *printerName;
    aSpec->GetPrinterName(&printerName);
    if (printerName) {
        const char *slash = strchr(printerName, '/');
        if (slash)
            printerName = slash + 1;
        if (0 != strcmp(printerName, "default"))
            mPrinterName = printerName;
    }
    return NS_OK;
}







nsresult
nsPrintJobVMSCmdPS::StartSubmission(FILE **aHandle)
{
    NS_PRECONDITION(aHandle, "aHandle is NULL");
    NS_PRECONDITION(!GetDestination().IsEmpty(), "No destination");
    NS_PRECONDITION(!GetDestHandle(), "Already have a destination handle");

    
    FILE *printHandle = nsnull;
    nsresult rv = mTempFactory.CreateTempFile(
            getter_AddRefs(mTempFile), &printHandle, "w+");
    if (NS_SUCCEEDED(rv)) {
        SetDestHandle(printHandle);
        *aHandle = printHandle;
    }
    return rv;
}

nsresult
nsPrintJobVMSCmdPS::FinishSubmission()
{
    NS_PRECONDITION(GetDestHandle(), "No destination file handle");
    NS_PRECONDITION(!GetDestination().IsEmpty(), "No destination");

    
    fclose(GetDestHandle());
    SetDestHandle(nsnull);

    
    nsCAutoString printFileName;
    nsresult rv = mTempFile->GetNativePath(printFileName);
    if (NS_SUCCEEDED(rv)) {
        nsCAutoString cmd(GetDestination());
        cmd += " "; cmd += printFileName; cmd += ".";

        
        if (PR_SUCCESS != EnvLock())
            return NS_ERROR_OUT_OF_MEMORY;
        if (!mPrinterName.IsEmpty())
            EnvSetPrinter(mPrinterName);

        
        int presult = system(cmd.get());

        
        EnvClear();
        mTempFile->Remove(PR_FALSE);

        rv = (!WIFEXITED(presult) || (EXIT_SUCCESS != WEXITSTATUS(presult)))
            ? NS_ERROR_GFX_PRINTER_CMD_FAILURE : NS_OK;
    }
    return rv;
}


#else   










nsPrintJobPipePS::~nsPrintJobPipePS()
{
    if (GetDestHandle()) {
        pclose(GetDestHandle());
        SetDestHandle(nsnull);
    }
}






nsresult
nsPrintJobPipePS::Init(nsIDeviceContextSpecPS *aSpec)
{
    NS_PRECONDITION(aSpec, "argument must not be NULL");
#ifdef DEBUG
    PRBool toPrinter;
    aSpec->GetToPrinter(toPrinter);
    NS_PRECONDITION(toPrinter, "Wrong class for this print job");
#endif

    
    const char *command;
    aSpec->GetCommand(&command);
    SetDestination(command);

    
    const char *printerName;
    aSpec->GetPrinterName(&printerName);
    if (printerName) {
        const char *slash = strchr(printerName, '/');
        if (slash)
            printerName = slash + 1;
        if (0 != strcmp(printerName, "default"))
            mPrinterName = printerName;
    }
    return NS_OK;
}






nsresult
nsPrintJobPipePS::StartSubmission(FILE **aHandle)
{
    NS_PRECONDITION(aHandle, "aHandle is NULL");
    NS_PRECONDITION(!GetDestination().IsEmpty(), "No destination");
    NS_PRECONDITION(!GetDestHandle(), "Already have a destination handle");

    if (PR_SUCCESS != EnvLock())
        return NS_ERROR_OUT_OF_MEMORY;  
    if (!mPrinterName.IsEmpty())
        EnvSetPrinter(mPrinterName);

    FILE *destPipe = popen(GetDestination().get(), "w");
    EnvClear();
    if (!destPipe)
        return NS_ERROR_GFX_PRINTER_CMD_FAILURE;
    SetDestHandle(destPipe);
    *aHandle = destPipe;
    return NS_OK;
}

nsresult
nsPrintJobPipePS::FinishSubmission()
{
    NS_PRECONDITION(GetDestHandle(), "No destination file handle");
    NS_PRECONDITION(!GetDestination().IsEmpty(), "No destination");

    int presult = pclose(GetDestHandle());
    SetDestHandle(nsnull);
    if (!WIFEXITED(presult) || (EXIT_SUCCESS != WEXITSTATUS(presult)))
        return NS_ERROR_GFX_PRINTER_CMD_FAILURE;
    return NS_OK;
}








nsresult
nsPrintJobCUPS::Init(nsIDeviceContextSpecPS *aSpec)
{
    NS_PRECONDITION(aSpec, "argument must not be NULL");
#ifdef DEBUG
    PRBool toPrinter;
    aSpec->GetToPrinter(toPrinter);
    NS_PRECONDITION(toPrinter, "Wrong class for this print job");
#endif

    NS_ENSURE_TRUE(mCups.Init(), NS_ERROR_NOT_INITIALIZED);

    
    const char *printerName = nsnull;
    aSpec->GetPrinterName(&printerName);
    NS_ENSURE_TRUE(printerName, NS_ERROR_GFX_PRINTER_NAME_NOT_FOUND);

    const char *slash = strchr(printerName, '/');
    mPrinterName = slash ? slash + 1 : printerName;
    mJobTitle.SetIsVoid(PR_TRUE);
    return NS_OK;
}

nsresult
nsPrintJobCUPS::SetNumCopies(int aNumCopies)
{
    mNumCopies.Truncate();
    if (aNumCopies > 1)
        mNumCopies.AppendInt(aNumCopies);
    return NS_OK;
}






void
nsPrintJobCUPS::SetJobTitle(const PRUnichar *aTitle)
{
    if (aTitle) {
        LossyCopyUTF16toASCII(aTitle, mJobTitle);
    }
}


nsresult
nsPrintJobCUPS::StartSubmission(FILE **aHandle)
{
    NS_ENSURE_TRUE(mCups.IsInitialized(), NS_ERROR_NOT_INITIALIZED);

    int fd;
    char buf[FILENAME_MAX];

    fd = (mCups.mCupsTempFd)(buf, sizeof buf);
    
    
    NS_ENSURE_TRUE(fd > 0, NS_ERROR_GFX_PRINTER_COULD_NOT_OPEN_FILE);

    SetDestHandle(fdopen(fd, "r+"));
    if (!GetDestHandle()) {
        close(fd);
        return NS_ERROR_GFX_PRINTER_COULD_NOT_OPEN_FILE;
    }
    SetDestination(buf);
    *aHandle = GetDestHandle();
    return NS_OK;
}


nsresult
nsPrintJobCUPS::FinishSubmission()
{
    NS_ENSURE_TRUE(mCups.IsInitialized(), NS_ERROR_NOT_INITIALIZED);
    NS_PRECONDITION(GetDestHandle(), "No destination file handle");
    NS_PRECONDITION(!GetDestination().IsEmpty(), "No destination");

    fclose(GetDestHandle());
    SetDestHandle(nsnull);

    nsCStringArray printer(3);
    printer.ParseString(mPrinterName.get(),"/");

    cups_dest_t *dests, *dest;
    int num_dests = (mCups.mCupsGetDests)(&dests);
    
    if (printer.Count() == 1) {
        dest = (mCups.mCupsGetDest)(printer.CStringAt(0)->get(), NULL, num_dests, dests);
    } else {
        dest = (mCups.mCupsGetDest)(printer.CStringAt(0)->get(), 
                                    printer.CStringAt(1)->get(), num_dests, dests);
    }

    
    int result=0;
    if (dest != NULL) {
        if (!mNumCopies.IsEmpty())
            dest->num_options = (mCups.mCupsAddOption)("copies",
                                                       mNumCopies.get(),
                                                       dest->num_options,
                                                       &dest->options);
        const char *title = mJobTitle.IsVoid() ?
            "Untitled Document" : mJobTitle.get();
        result = (mCups.mCupsPrintFile)(printer.CStringAt(0)->get(),
                                            GetDestination().get(), title, 
                                            dest->num_options, dest->options);
    }
    (mCups.mCupsFreeDests)(num_dests, dests);
    unlink(GetDestination().get());

    
    
    
    if (dest == NULL)
        return NS_ERROR_GFX_PRINTER_NAME_NOT_FOUND;
    else
        return (result < 0x0300) ? NS_OK : NS_ERROR_GFX_PRINTER_CMD_FAILURE;
}


#endif  






static PRLock *EnvLockObj;
static PRCallOnceType EnvLockOnce;


static PRStatus
EnvLockInit()
{
    EnvLockObj = PR_NewLock();
    return EnvLockObj ? PR_SUCCESS : PR_FAILURE;
}









static PRStatus
EnvLock()
{
    if (PR_FAILURE == PR_CallOnce(&EnvLockOnce, EnvLockInit))
        return PR_FAILURE;
    PR_Lock(EnvLockObj);
    return PR_SUCCESS;
}


static char *EnvPrinterString;
static const char EnvPrinterName[] = { "MOZ_PRINTER_NAME" };








static PRStatus
EnvSetPrinter(nsCString& aPrinter)
{
    
    char *newVar = PR_smprintf("%s=%s", EnvPrinterName, aPrinter.get());
    if (!newVar)
        return PR_FAILURE;

    
    PR_SetEnv(newVar);
    if (EnvPrinterString)
        PR_smprintf_free(EnvPrinterString);
    EnvPrinterString = newVar;

    return PR_SUCCESS;
}





static void
EnvClear()
{
    if (EnvPrinterString) {
        


        PR_SetEnv(EnvPrinterName);
        if (!PR_GetEnv(EnvPrinterName)) {
            
            PR_smprintf_free(EnvPrinterString);
            EnvPrinterString = nsnull;
        }
    }
    PR_Unlock(EnvLockObj);
}
