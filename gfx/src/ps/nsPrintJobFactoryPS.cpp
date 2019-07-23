






































#include "nsDebug.h"
#include "nsPrintJobFactoryPS.h"
#include "nsIDeviceContextSpecPS.h"
#include "nsPrintJobPS.h"
#include "nsPSPrinters.h"











nsresult
nsPrintJobFactoryPS::CreatePrintJob(nsIDeviceContextSpecPS *aSpec,
        nsIPrintJobPS* &aPrintJob)
{
    NS_PRECONDITION(nsnull != aSpec, "aSpec is NULL");

    nsIPrintJobPS *newPJ;

    PRBool setting;
    aSpec->GetIsPrintPreview(setting);
    if (setting)
        newPJ = new nsPrintJobPreviewPS();
    else {
        aSpec->GetToPrinter(setting);
        if (!setting)
            newPJ = new nsPrintJobFilePS();
        else
#ifdef VMS
            newPJ = new nsPrintJobVMSCmdPS();
#else
        {
            const char *printerName;
            aSpec->GetPrinterName(&printerName);
            if (nsPSPrinterList::kTypeCUPS == nsPSPrinterList::GetPrinterType(
                        nsDependentCString(printerName)))
                newPJ = new nsPrintJobCUPS();
            else
                newPJ = new nsPrintJobPipePS();
        }
#endif
    }
    if (!newPJ)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = newPJ->Init(aSpec);
    if (NS_FAILED(rv))
        delete newPJ;
    else
        aPrintJob = newPJ;
    return rv;
}
