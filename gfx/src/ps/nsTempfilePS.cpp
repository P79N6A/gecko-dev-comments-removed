







































#include "nsTempfilePS.h"
#include "nsCOMPtr.h"
#include "nsDirectoryServiceDefs.h"
#include "nsILocalFile.h"
#include "nsPrintfCString.h"
#include "prtime.h"


nsTempfilePS::nsTempfilePS()
{
    nsresult rv;
    
    
    rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(mTempDir));
    if (NS_FAILED(rv))
        return;

    
    
    LL_L2UI(mCount, PR_Now());

    
    rv = mTempDir->Append(
            NS_ConvertASCIItoUTF16(nsPrintfCString("%lx.tmp", mCount++)));
    if (NS_FAILED(rv)) {
        mTempDir = nsnull;
        return;
    }
    
    
    rv = mTempDir->CreateUnique(nsIFile::DIRECTORY_TYPE, 0700);
    if (NS_FAILED(rv))
        mTempDir = nsnull;
}

nsTempfilePS::~nsTempfilePS()
{
    if (nsnull != mTempDir)
        mTempDir->Remove(PR_TRUE);
}

nsresult
nsTempfilePS::CreateTempFile(nsILocalFile** aFile)
{
    NS_PRECONDITION(nsnull != aFile, "aFile argument is NULL");
    NS_ENSURE_TRUE(nsnull != mTempDir, NS_ERROR_NOT_INITIALIZED);

    
    nsresult rv;
    nsAutoString tmpdir;
    rv = mTempDir->GetPath(tmpdir);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsILocalFile> tmpfile;
    rv = NS_NewLocalFile(tmpdir, PR_FALSE, getter_AddRefs(tmpfile));
    NS_ENSURE_SUCCESS(rv, rv);
    NS_POSTCONDITION(nsnull != tmpfile,
            "NS_NewLocalFile succeeded but tmpfile is invalid");

    rv = tmpfile->Append(
            NS_ConvertASCIItoUTF16(nsPrintfCString("%lx.tmp", mCount++)));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = tmpfile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600);
    NS_ENSURE_SUCCESS(rv, rv);

    *aFile = tmpfile.get();
    NS_ADDREF(*aFile);
    return NS_OK;
}


nsresult
nsTempfilePS::CreateTempFile(nsILocalFile** aFile,
        FILE **aHandle, const char *aMode)
{
    NS_PRECONDITION(nsnull != aHandle, "aHandle is invalid");
    NS_PRECONDITION(nsnull != aMode, "aMode is invalid");

    nsresult rv = CreateTempFile(aFile);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_POSTCONDITION(nsnull != *aFile,
            "CreateTempFile() succeeded but *aFile is invalid");

    rv = (*aFile)->OpenANSIFileDesc(aMode, aHandle);
    if (NS_FAILED(rv)) {
        (*aFile)->Remove(PR_FALSE);
        NS_RELEASE(*aFile);
    }
    return rv;
}
