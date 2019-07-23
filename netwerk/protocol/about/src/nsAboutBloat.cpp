




































#include "nsAboutBloat.h"
#include "nsIIOService.h"
#include "nsIServiceManager.h"
#include "nsStringStream.h"
#include "nsXPIDLString.h"
#include "nsIURI.h"
#include "prtime.h"
#include "nsCOMPtr.h"
#include "nsIFileStreams.h"
#include "nsNetUtil.h"
#include "nsDirectoryServiceDefs.h"
#include "nsILocalFile.h"
#include "nsTraceRefcntImpl.h"

#ifdef XP_MAC
extern "C" void GC_gcollect(void);
#else
static void GC_gcollect() {}
#endif

NS_IMPL_ISUPPORTS1(nsAboutBloat, nsIAboutModule)

NS_IMETHODIMP
nsAboutBloat::NewChannel(nsIURI *aURI, nsIChannel **result)
{
    NS_ENSURE_ARG_POINTER(aURI);
    nsresult rv;
    nsCAutoString path;
    rv = aURI->GetPath(path);
    if (NS_FAILED(rv)) return rv;

    nsTraceRefcntImpl::StatisticsType statType = nsTraceRefcntImpl::ALL_STATS;
    PRBool clear = PR_FALSE;
    PRBool leaks = PR_FALSE;

    PRInt32 pos = path.Find("?");
    if (pos > 0) {
        nsCAutoString param;
        (void)path.Right(param, path.Length() - (pos+1));
        if (param.EqualsLiteral("new"))
            statType = nsTraceRefcntImpl::NEW_STATS;
        else if (param.EqualsLiteral("clear"))
            clear = PR_TRUE;
        else if (param.EqualsLiteral("leaks"))
            leaks = PR_TRUE;
    }

    nsCOMPtr<nsIInputStream> inStr;
    if (clear) {
        nsTraceRefcntImpl::ResetStatistics();

        const char* msg = "Bloat statistics cleared.";
        rv = NS_NewCStringInputStream(getter_AddRefs(inStr), nsDependentCString(msg));
        if (NS_FAILED(rv)) return rv;
    }
    else if (leaks) {
        
        GC_gcollect();
    	
        const char* msg = "Memory leaks dumped.";
        rv = NS_NewCStringInputStream(getter_AddRefs(inStr), nsDependentCString(msg));
        if (NS_FAILED(rv)) return rv;
    }
    else {
        nsCOMPtr<nsIFile> file;
        rv = NS_GetSpecialDirectory(NS_OS_CURRENT_PROCESS_DIR, 
                                    getter_AddRefs(file));       
        if (NS_FAILED(rv)) return rv;

        rv = file->AppendNative(NS_LITERAL_CSTRING("bloatlogs"));
        if (NS_FAILED(rv)) return rv;

        PRBool exists;
        rv = file->Exists(&exists);
        if (NS_FAILED(rv)) return rv;

        if (!exists) {
            
            
            
            rv = file->Create(nsIFile::DIRECTORY_TYPE, 0755);
            if (NS_FAILED(rv)) return rv;
        }

        nsCAutoString dumpFileName;
        if (statType == nsTraceRefcntImpl::ALL_STATS)
            dumpFileName.AssignLiteral("all-");
        else
            dumpFileName.AssignLiteral("new-");
        PRExplodedTime expTime;
        PR_ExplodeTime(PR_Now(), PR_LocalTimeParameters, &expTime);
        char time[128];
        PR_FormatTimeUSEnglish(time, 128, "%Y-%m-%d-%H%M%S.txt", &expTime);
        dumpFileName += time;
        rv = file->AppendNative(dumpFileName);
        if (NS_FAILED(rv)) return rv;

        FILE* out;
        nsCOMPtr<nsILocalFile> lfile = do_QueryInterface(file);
        if (lfile == nsnull)
            return NS_ERROR_FAILURE;
        rv = lfile->OpenANSIFileDesc("w", &out);
        if (NS_FAILED(rv)) return rv;

        rv = nsTraceRefcntImpl::DumpStatistics(statType, out);
        ::fclose(out);
        if (NS_FAILED(rv)) return rv;

        rv = NS_NewLocalFileInputStream(getter_AddRefs(inStr), file);
        if (NS_FAILED(rv)) return rv;
    }

    nsIChannel* channel;
    rv = NS_NewInputStreamChannel(&channel, aURI, inStr,
                                  NS_LITERAL_CSTRING("text/plain"),
                                  NS_LITERAL_CSTRING("utf-8"));
    if (NS_FAILED(rv)) return rv;

    *result = channel;
    return rv;
}

NS_IMETHODIMP
nsAboutBloat::GetURIFlags(nsIURI *aURI, PRUint32 *result)
{
    *result = 0;
    return NS_OK;
}

NS_METHOD
nsAboutBloat::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsAboutBloat* about = new nsAboutBloat();
    if (about == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(about);
    nsresult rv = about->QueryInterface(aIID, aResult);
    NS_RELEASE(about);
    return rv;
}


