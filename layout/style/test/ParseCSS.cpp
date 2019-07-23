













































#include "nsXPCOM.h"
#include "nsCOMPtr.h"

#include "nsILocalFile.h"
#include "nsNetUtil.h"

#include "nsContentCID.h"
#include "nsICSSLoader.h"
#include "nsICSSStyleSheet.h"

static NS_DEFINE_CID(kCSSLoaderCID, NS_CSS_LOADER_CID);

static already_AddRefed<nsIURI>
FileToURI(const char *aFilename, nsresult *aRv = 0)
{
    nsCOMPtr<nsILocalFile> lf(do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, aRv));
    NS_ENSURE_TRUE(lf, nsnull);
    
    lf->InitWithNativePath(nsDependentCString(aFilename));

    nsIURI *uri = nsnull;
    nsresult rv = NS_NewFileURI(&uri, lf);
    if (aRv)
        *aRv = rv;
    return uri;
}

static int
ParseCSSFile(nsIURI *aSheetURI)
{
    nsCOMPtr<nsICSSLoader> loader(do_CreateInstance(kCSSLoaderCID));
    nsCOMPtr<nsICSSStyleSheet> sheet;
    loader->LoadSheetSync(aSheetURI, getter_AddRefs(sheet));
    NS_ASSERTION(sheet, "sheet load failed");
    


    if (!sheet)
        return -1;
    PRBool complete;
    sheet->GetComplete(complete);
    NS_ASSERTION(complete, "synchronous load did not complete");
    if (!complete)
        return -2;
    return 0;
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "%s [FILE]...\n", argv[0]);
    }
    nsresult rv = NS_InitXPCOM2(nsnull, nsnull, nsnull);
    if (NS_FAILED(rv))
        return (int)rv;

    int res = 0;
    for (int i = 1; i < argc; ++i) {
        const char *filename = argv[i];

        printf("\nParsing %s.\n", filename);

        nsCOMPtr<nsIURI> uri = FileToURI(filename, &rv);
        if (rv == NS_ERROR_OUT_OF_MEMORY) {
            fprintf(stderr, "Out of memory.\n");
            return 1;
        }
        if (uri)
            res = ParseCSSFile(uri);
    }

    NS_ShutdownXPCOM(nsnull);

    return res;
}
