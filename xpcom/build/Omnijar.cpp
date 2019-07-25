





































#include "Omnijar.h"

#include "nsILocalFile.h"
#include "nsXULAppAPI.h"
#include "nsZipArchive.h"

static nsILocalFile* sOmnijarPath = nsnull;
static nsZipArchive* sOmnijarReader = nsnull;

nsILocalFile*
mozilla::OmnijarPath()
{
    return sOmnijarPath;
}

nsZipArchive*
mozilla::OmnijarReader()
{
    return sOmnijarReader;
}

void
mozilla::SetOmnijar(nsILocalFile* aPath)
{
    NS_IF_RELEASE(sOmnijarPath);
    if (sOmnijarReader) {
        sOmnijarReader->CloseArchive();
        delete sOmnijarReader;
        sOmnijarReader = nsnull;
    }

    if (!aPath) {
        return;
    }

    nsZipArchive* zipReader = new nsZipArchive();
    if (!zipReader) {
        return;
    }

    if (NS_FAILED(zipReader->OpenArchive(aPath))) {
        delete zipReader;
        return;
    }

    sOmnijarReader = zipReader;
    sOmnijarPath = aPath;
    NS_ADDREF(sOmnijarPath);
}

