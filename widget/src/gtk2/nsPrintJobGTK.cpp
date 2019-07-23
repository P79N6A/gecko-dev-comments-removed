






































#include "nscore.h"
#include "nsIDeviceContext.h"   
#include "nsIDeviceContextSpec.h"
#include "nsDeviceContextSpecG.h"

#include "nsPrintJobGTK.h"
#include "nsPSPrinters.h"
#include "nsReadableUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIFileStreams.h"

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





nsIPrintJobGTK::~nsIPrintJobGTK()
{
    if (mSpoolFile)
        mSpoolFile->Remove(PR_FALSE);
}

nsresult
nsIPrintJobGTK::GetSpoolFile(nsILocalFile **aFile)
{
    if (!mSpoolFile)
        return NS_ERROR_NOT_INITIALIZED;
    *aFile = mSpoolFile;
    NS_ADDREF(*aFile);
    return NS_OK;
}








nsresult
nsPrintJobPreviewGTK::InitSpoolFile(PRUint32 aPermissions)
{
    nsCOMPtr<nsIFile> spoolFile;
    nsresult rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR,
                                         getter_AddRefs(spoolFile));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_GFX_PRINTER_COULD_NOT_OPEN_FILE);

    spoolFile->AppendNative(NS_LITERAL_CSTRING("tmp.prn"));
    
    rv = spoolFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, aPermissions);
    if (NS_FAILED(rv))
        return NS_ERROR_GFX_PRINTER_COULD_NOT_OPEN_FILE;
    mSpoolFile = do_QueryInterface(spoolFile, &rv);
    if (NS_FAILED(rv))
        spoolFile->Remove(PR_FALSE);

    return rv;
}

nsresult
nsPrintJobPreviewGTK::Init(nsDeviceContextSpecGTK *aSpec)
{
#ifdef DEBUG
    PRBool isPreview;
    aSpec->GetIsPrintPreview(isPreview);
    NS_PRECONDITION(isPreview, "This print job is to a printer");
#endif
    return InitSpoolFile(0600);
}







nsresult
nsPrintJobFileGTK::Init(nsDeviceContextSpecGTK *aSpec)
{
    NS_PRECONDITION(aSpec, "aSpec must not be NULL");
#ifdef DEBUG
    PRBool toPrinter;
    aSpec->GetToPrinter(toPrinter);
    NS_PRECONDITION(!toPrinter, "This print job is to a printer");
#endif

    
    nsresult rv = InitSpoolFile(0666);
    if (NS_SUCCEEDED(rv)) {
        const char *path;
        aSpec->GetPath(&path);
        rv = NS_NewNativeLocalFile(nsDependentCString(path), PR_FALSE,
                                   getter_AddRefs(mDestFile));
    }
    return rv;
}

nsresult
nsPrintJobFileGTK::Submit()
{
    
    nsAutoString destLeafName;
    nsresult rv = mDestFile->GetLeafName(destLeafName);
    if (NS_SUCCEEDED(rv)) {
        nsCOMPtr<nsIFile> mDestDir;
        rv = mDestFile->GetParent(getter_AddRefs(mDestDir));
        if (NS_SUCCEEDED(rv)) {
            rv = mSpoolFile->MoveTo(mDestDir, destLeafName);
        }
    }
    return rv;
}











nsresult
nsPrintJobPipeGTK::Init(nsDeviceContextSpecGTK *aSpec)
{
    NS_PRECONDITION(aSpec, "argument must not be NULL");
#ifdef DEBUG
    PRBool toPrinter;
    aSpec->GetToPrinter(toPrinter);
    NS_PRECONDITION(toPrinter, "Wrong class for this print job");
#endif

    
    nsresult rv = InitSpoolFile(0600);
    if (NS_FAILED(rv))
        return rv;

    
    const char *command;
    aSpec->GetCommand(&command);
    mCommand = command;

    
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



static nsresult
popenPrintCommand(FILE **aPipe, nsCString &aPrinter, nsCString &aCommand)
{
    
    if (PR_SUCCESS != EnvLock())
        return NS_ERROR_OUT_OF_MEMORY;  

    if (!aPrinter.IsEmpty())
        EnvSetPrinter(aPrinter);

    
    *aPipe = popen(aCommand.get(), "w");
    EnvClear();
    return (*aPipe) ? NS_OK : NS_ERROR_GFX_PRINTER_CMD_FAILURE;
}




static nsresult
CopySpoolToCommand(nsIFileInputStream *aSource, FILE *aDest)
{
    nsresult rv;
    PRUint32 count;
    do {
        char buf[256];
        count = 0;
        rv = aSource->Read(buf, sizeof buf, &count);
        fwrite(buf, 1, count, aDest);
    } while (NS_SUCCEEDED(rv) && count);
    return rv;
}
        





nsresult
nsPrintJobPipeGTK::Submit()
{
    NS_PRECONDITION(mSpoolFile, "No spool file");

    
    nsCOMPtr<nsIFileInputStream> spoolStream =
        do_CreateInstance("@mozilla.org/network/file-input-stream;1");
    if (!spoolStream)
        return NS_ERROR_OUT_OF_MEMORY;
    nsresult rv = spoolStream->Init(mSpoolFile, -1, -1,
        nsIFileInputStream::DELETE_ON_CLOSE|nsIFileInputStream::CLOSE_ON_EOF);
    NS_ENSURE_SUCCESS(rv, rv);
    
    
    FILE *destPipe;
    rv = popenPrintCommand(&destPipe, mPrinterName, mCommand);
    if (NS_SUCCEEDED(rv)) {
        rv = CopySpoolToCommand(spoolStream, destPipe);
        int presult = pclose(destPipe);
        if (NS_SUCCEEDED(rv)) {
            if (!WIFEXITED(presult) || (EXIT_SUCCESS != WEXITSTATUS(presult)))
                rv = NS_ERROR_GFX_PRINTER_CMD_FAILURE;
        }
    }            
    spoolStream->Close();
    return rv;
}















nsresult
nsPrintJobCUPS::Init(nsDeviceContextSpecGTK *aSpec)
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
    
    
    int fd;
    char buf[FILENAME_MAX];

    fd = (mCups.mCupsTempFd)(buf, sizeof buf);
    
    
    NS_ENSURE_TRUE(fd > 0, NS_ERROR_GFX_PRINTER_COULD_NOT_OPEN_FILE);
    close(fd);
    
    nsresult rv = NS_NewNativeLocalFile(nsDependentCString(buf), PR_FALSE,
                                        getter_AddRefs(mSpoolFile));
    if (NS_FAILED(rv)) {
        unlink(buf);
        return NS_ERROR_GFX_PRINTER_COULD_NOT_OPEN_FILE;
    }
    mSpoolName = buf;
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
nsPrintJobCUPS::Submit()
{
    NS_ENSURE_TRUE(mCups.IsInitialized(), NS_ERROR_NOT_INITIALIZED);
    NS_PRECONDITION(!mSpoolName.IsEmpty(), "No spool file");

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
                                            mSpoolName.get(), title, 
                                            dest->num_options, dest->options);
    }
    (mCups.mCupsFreeDests)(num_dests, dests);

    
    
    
    if (dest == NULL)
        return NS_ERROR_GFX_PRINTER_NAME_NOT_FOUND;
    else
        return (result < 0x0300) ? NS_OK : NS_ERROR_GFX_PRINTER_CMD_FAILURE;
}





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
