





































#include "Omnijar.h"

#include "nsILocalFile.h"
#include "nsXULAppAPI.h"
#include "nsZipArchive.h"

static nsILocalFile* sOmnijarPath = nsnull;
static nsZipArchive* sOmnijarReader = nsnull;

static void
SetupReader()
{
    if (!sOmnijarPath) {
        return;
    }

    nsZipArchive* zipReader = new nsZipArchive();
    if (!zipReader) {
        NS_IF_RELEASE(sOmnijarPath);
        return;
    }

    if (NS_FAILED(zipReader->OpenArchive(sOmnijarPath))) {
        delete zipReader;
        NS_IF_RELEASE(sOmnijarPath);
        return;
    }

    sOmnijarReader = zipReader;
}

nsILocalFile*
mozilla::OmnijarPath()
{
    if (!sOmnijarReader)
        SetupReader();

    return sOmnijarPath;
}

nsZipArchive*
mozilla::OmnijarReader()
{
    if (!sOmnijarReader)
        SetupReader();

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

    sOmnijarPath = aPath;
    NS_IF_ADDREF(sOmnijarPath);
}

