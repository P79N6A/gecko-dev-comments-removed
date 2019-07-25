






































#include "Omnijar.h"

#include "nsIFile.h"
#include "nsZipArchive.h"
#include "nsNetUtil.h"

namespace mozilla {

nsIFile *Omnijar::sPath[2] = { nsnull, nsnull };
PRBool Omnijar::sIsOmnijar[2] = { PR_FALSE, PR_FALSE };

#ifdef MOZ_ENABLE_LIBXUL
nsZipArchive *Omnijar::sReader[2] = { nsnull, nsnull };
#endif

static already_AddRefed<nsIFile>
ComputePath(nsIFile *aPath, PRBool &aIsOmnijar)
{
    PRBool isDir;
    aIsOmnijar = PR_FALSE;
    if (!aPath || NS_FAILED(aPath->IsDirectory(&isDir)) || !isDir)
        return nsnull;

    nsCOMPtr<nsIFile> path;
#ifdef MOZ_ENABLE_LIBXUL
    
    if (!isDir || NS_FAILED(aPath->Clone(getter_AddRefs(path))))
        return nsnull;

    if (NS_FAILED(path->AppendNative(NS_LITERAL_CSTRING("omni.jar"))))
        return nsnull;

    if (NS_FAILED(path->Exists(&aIsOmnijar)))
        return nsnull;
#endif

    if (!aIsOmnijar && NS_FAILED(aPath->Clone(getter_AddRefs(path))))
        return nsnull;

    return path.forget();
}

nsresult
Omnijar::SetBase(nsIFile *aGrePath, nsIFile *aAppPath)
{
    NS_ABORT_IF_FALSE(aGrePath || !aAppPath, "Omnijar::SetBase(NULL, something) call forbidden");

#ifdef MOZ_ENABLE_LIBXUL
    if (sReader[GRE]) {
        sReader[GRE]->CloseArchive();
        delete sReader[GRE];
    }
    if (sReader[APP]) {
        sReader[APP]->CloseArchive();
        delete sReader[APP];
    }
    sReader[APP] = sReader[GRE] = nsnull;
#endif

    nsresult rv;
    PRBool equals;
    if (aAppPath) {
        rv = aAppPath->Equals(aGrePath, &equals);
        NS_ENSURE_SUCCESS(rv, rv);
    } else {
        equals = PR_TRUE;
    }

    nsCOMPtr<nsIFile> grePath = ComputePath(aGrePath, sIsOmnijar[GRE]);
    nsCOMPtr<nsIFile> appPath = ComputePath(equals ? nsnull : aAppPath, sIsOmnijar[APP]);

    NS_IF_RELEASE(sPath[GRE]);
    sPath[GRE] = grePath;
    NS_IF_ADDREF(sPath[GRE]);

    NS_IF_RELEASE(sPath[APP]);
    sPath[APP] = appPath;
    NS_IF_ADDREF(sPath[APP]);

    return NS_OK;
}

already_AddRefed<nsIFile>
Omnijar::GetBase(Type aType)
{
    NS_ABORT_IF_FALSE(sPath[0], "Omnijar not initialized");

    if (!sIsOmnijar[aType]) {
        NS_IF_ADDREF(sPath[aType]);
        return sPath[aType];
    }

    nsCOMPtr<nsIFile> file, path;
    if (NS_FAILED(sPath[aType]->Clone(getter_AddRefs(file))))
        return nsnull;

    if (NS_FAILED(file->GetParent(getter_AddRefs(path))))
        return nsnull;
    return path.forget();
}

#ifdef MOZ_ENABLE_LIBXUL
nsZipArchive *
Omnijar::GetReader(Type aType)
{
    if (!sIsOmnijar[aType])
        return nsnull;

    if (sReader[aType])
        return sReader[aType];

    nsZipArchive* zipReader = new nsZipArchive();
    if (!zipReader)
        return nsnull;

    if (NS_FAILED(zipReader->OpenArchive(sPath[aType]))) {
        delete zipReader;
        return nsnull;
    }

    return (sReader[aType] = zipReader);
}

nsZipArchive *
Omnijar::GetReader(nsIFile *aPath)
{
    PRBool equals;
    nsresult rv;

    if (sIsOmnijar[GRE]) {
        rv = sPath[GRE]->Equals(aPath, &equals);
        if (NS_SUCCEEDED(rv) && equals)
            return GetReader(GRE);
    }
    if (sIsOmnijar[APP]) {
        rv = sPath[APP]->Equals(aPath, &equals);
        if (NS_SUCCEEDED(rv) && equals)
            return GetReader(APP);
    }
    return nsnull;
}
#endif

nsresult
Omnijar::GetURIString(Type aType, nsCString &result)
{
    NS_ABORT_IF_FALSE(sPath[0], "Omnijar not initialized");

    result = "";

    if ((aType == APP) && (!sPath[APP]))
        return NS_OK;

    nsCAutoString omniJarSpec;
    nsresult rv = NS_GetURLSpecFromActualFile(sPath[aType], omniJarSpec);
    NS_ENSURE_SUCCESS(rv, rv);

    if (sIsOmnijar[aType]) {
        result = "jar:";
        result += omniJarSpec;
        result += "!";
    } else {
        result = omniJarSpec;
    }
    result += "/";
    return NS_OK;
}

} 
