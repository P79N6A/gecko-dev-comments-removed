





































#include "nscore.h"
#include "nsCUPSShim.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIServiceManager.h"
#include "nsPrintfCString.h"
#include "nsPSPrinters.h"
#include "nsReadableUtils.h"        
#include "nsCUPSShim.h"

#include "prlink.h"
#include "prenv.h"
#include "plstr.h"

#define NS_CUPS_PRINTER "CUPS/"
#define NS_CUPS_PRINTER_LEN (sizeof(NS_CUPS_PRINTER) - 1)


#define NS_POSTSCRIPT_DRIVER_NAME "PostScript/"

nsCUPSShim gCupsShim;


nsresult
nsPSPrinterList::Init()
{
    nsresult rv;

    mPrefSvc = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv))
        rv = mPrefSvc->GetBranch("print.", getter_AddRefs(mPref));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_NOT_INITIALIZED);

    
    PRBool useCups = PR_TRUE;
    rv = mPref->GetBoolPref("postscript.cups.enabled", &useCups);
    if (useCups && !gCupsShim.IsInitialized())
        gCupsShim.Init();
    return NS_OK;
}



PRBool
nsPSPrinterList::Enabled()
{
    const char *val = PR_GetEnv("MOZILLA_POSTSCRIPT_ENABLED");
    if (val && (val[0] == '0' || !PL_strcasecmp(val, "false")))
        return PR_FALSE;

    
    PRBool setting = PR_TRUE;
    mPref->GetBoolPref("postscript.enabled", &setting);
    return setting;
}



void
nsPSPrinterList::GetPrinterList(nsTArray<nsCString>& aList)
{
    aList.Clear();

    
    
    if (gCupsShim.IsInitialized()) {
        cups_dest_t *dests;

        int num_dests = (gCupsShim.mCupsGetDests)(&dests);
        if (num_dests) {
            for (int i = 0; i < num_dests; i++) {
                nsCAutoString fullName(NS_CUPS_PRINTER);
                fullName.Append(dests[i].name);
                if (dests[i].instance != NULL) {
                    fullName.Append("/");
                    fullName.Append(dests[i].instance);
                }
                if (dests[i].is_default)
                    aList.InsertElementAt(0, fullName);
                else
                    aList.AppendElement(fullName);
            }
        }
        (gCupsShim.mCupsFreeDests)(num_dests, dests);
    }

    
    
    
    
    
    
    aList.AppendElement(
            NS_LITERAL_CSTRING(NS_POSTSCRIPT_DRIVER_NAME "default"));

    nsXPIDLCString list;
    list.Assign(PR_GetEnv("MOZILLA_POSTSCRIPT_PRINTER_LIST"));
    if (list.IsEmpty())
        mPref->GetCharPref("printer_list", getter_Copies(list));
    if (!list.IsEmpty()) {
        
        
        char *state;

        for (char *name = PL_strtok_r(list.BeginWriting(), " ", &state);
                nsnull != name;
                name = PL_strtok_r(nsnull, " ", &state)
        ) {
            if (0 != strcmp(name, "default")) {
                nsCAutoString fullName(NS_POSTSCRIPT_DRIVER_NAME);
                fullName.Append(name);
                aList.AppendElement(fullName);
            }
        }
    }
}



nsPSPrinterList::PrinterType
nsPSPrinterList::GetPrinterType(const nsACString& aName)
{
    if (StringBeginsWith(aName, NS_LITERAL_CSTRING(NS_POSTSCRIPT_DRIVER_NAME)))
        return kTypePS;
    else if (StringBeginsWith(aName, NS_LITERAL_CSTRING(NS_CUPS_PRINTER)))
        return kTypeCUPS;
    else
        return kTypeUnknown;
}
