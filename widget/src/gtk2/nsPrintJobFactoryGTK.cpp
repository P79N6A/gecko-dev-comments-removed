






































#include "nsDebug.h"
#include "nsPrintJobFactoryGTK.h"
#include "nsIDeviceContextSpec.h"
#include "nsDeviceContextSpecG.h"
#include "nsPrintJobGTK.h"
#include "nsPSPrinters.h"











nsresult
nsPrintJobFactoryGTK::CreatePrintJob(nsDeviceContextSpecGTK *aSpec,
        nsIPrintJobGTK* &aPrintJob)
{
    NS_PRECONDITION(nsnull != aSpec, "aSpec is NULL");

    nsIPrintJobGTK *newPJ;

    PRBool setting;
    aSpec->GetIsPrintPreview(setting);
    if (setting)
        newPJ = new nsPrintJobPreviewGTK();
    else {
        aSpec->GetToPrinter(setting);
        if (!setting)
            newPJ = new nsPrintJobFileGTK();
        else
        {
            const char *printerName;
            aSpec->GetPrinterName(&printerName);
            if (nsPSPrinterList::kTypeCUPS == nsPSPrinterList::GetPrinterType(
                        nsDependentCString(printerName)))
                newPJ = new nsPrintJobCUPS();
            else
                newPJ = new nsPrintJobPipeGTK();
        }
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
