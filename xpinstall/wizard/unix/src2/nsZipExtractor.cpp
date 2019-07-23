






































#include "nsZipExtractor.h"

#define STANDALONE 1
#include "zipstub.h"
#include "zipfile.h"

nsZipExtractor::nsZipExtractor(char *aSrc, char *aDest) :
    mSrc(NULL),
    mDest(NULL)
{
    if (aSrc)
        mSrc = strdup(aSrc);
    if (aDest)
        mDest = strdup(aDest);
}

nsZipExtractor::~nsZipExtractor()
{
    XI_IF_FREE(mSrc);
    XI_IF_FREE(mDest);
}

int
nsZipExtractor::Extract(nsComponent *aXPIEngine, int aTotal)
{
    DUMP("Extract");

    char apath[MAXPATHLEN]; 
    char bindir[512];
    char zpath[MAXPATHLEN]; 
    char epath[MAXPATHLEN]; 
    char *leaf = NULL, *lslash = NULL;
    struct stat dummy;
    int i, bFoundAll = FALSE, err = OK;
    PRInt32 zerr = ZIP_OK;
    PRUint32 len;
    void *hZip = NULL, *hFind = NULL;

    if (!aXPIEngine || !(aXPIEngine->GetArchive()))
        return E_PARAM;

    len=snprintf(apath,sizeof(apath),"%s/%s", mSrc, aXPIEngine->GetArchive());
    if ( len >= sizeof(apath) )
        return E_PARAM; 

    if (-1 == stat(apath, &dummy))
        return E_NO_DOWNLOAD;

    

    zerr = ZIP_OpenArchive(apath, &hZip);
    if (zerr != ZIP_OK) return E_EXTRACTION;
    hFind = ZIP_FindInit(hZip, (const char *) NULL);
    if (!hFind)
    {
        err = E_EXTRACTION;
        goto au_revoir;
    }

    

    i = 0;
    while (!bFoundAll)
    {
        memset(zpath, 0, MAXPATHLEN);
        zerr = ZIP_FindNext(hFind, zpath, MAXPATHLEN);
        if (zerr == ZIP_ERR_FNF)
        {
            bFoundAll = true;
            break;
        }
        if (zerr != ZIP_OK)
        {
            err = E_EXTRACTION;
            goto au_revoir;
        }

        

        lslash = strrchr(zpath, '/');
        if (lslash == (zpath + strlen(zpath) - 1))
            continue;

        if (!lslash)
            leaf = zpath;
        else
            leaf = lslash + 1;
        if (!leaf)
            continue;

        

        if (gCtx->opt->mMode != nsXIOptions::MODE_SILENT)
            nsInstallDlg::MajorProgressCB(leaf, i,
                aTotal, nsInstallDlg::ACT_EXTRACT);

        len=snprintf(epath,sizeof(epath),"%s/%s", mDest, zpath);
        if ( len >= sizeof(apath) )
        {
            err = E_PARAM; 
            goto au_revoir;
        }
        err = DirCreateRecursive(epath);
        if (err != OK) goto au_revoir;

        zerr = ZIP_ExtractFile(hZip, zpath, epath);
        if (zerr != ZIP_OK)
        {
            err = E_EXTRACTION;
            goto au_revoir;
        }

        i++;
    }

    len=snprintf(bindir,sizeof(bindir),"%s/%s", mDest, TMP_EXTRACT_SUBDIR);
    if ( len >= sizeof(bindir) )
    {
        err = E_PARAM; 
        goto au_revoir;
    }

    if (-1 == stat(bindir, &dummy))
        err = E_EXTRACTION;

au_revoir:
    

    if (hFind)
        ZIP_FindFree(hFind);
    if (hZip)
        ZIP_CloseArchive(&hZip);
    return err;
}

int
nsZipExtractor::DirCreateRecursive(char *aPath)
{
    PRUint32 len;
    int err = OK;
    char *slash = NULL;
    char currdir[MAXPATHLEN];
    struct stat dummy;

    if (!aPath || !mDest)
        return E_PARAM;

    slash = aPath + strlen(mDest);
    if (*slash != '/')
        return E_INVALID_PTR;

    while (slash)
    {
        len = slash - aPath;
        if (len >= sizeof(currdir)) return E_PARAM; 

        snprintf(currdir,len+1,"%s",aPath);

        if (-1 == stat(currdir, &dummy))
        {
            if (-1 == mkdir(currdir, 0755))
                return E_MKDIR_FAIL;
        }

        slash = strchr(slash+1, '/');
    }

    return err;
}
