







































#include "StdAfx.h"
#include "nsIFile.h"
#include "nsILocalFile.h"
#include "nsServiceManagerUtils.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "MostRecentUrls.h"




CMostRecentUrls::CMostRecentUrls() :
        mNumURLs(0)
{
    for (int i=0;i<MAX_URLS;i++) {
        mURLs[i] = NULL;
    }

    FILE * fd = GetFD("r");
    if (fd) {
        char line[512];
        while (fgets(line, 512, fd)) {
            if (strlen(line) > 1) {
                line[strlen(line)-1] = 0;
                mURLs[mNumURLs++] = _strdup(line);
            }
        }
        fclose(fd);
    }

}

FILE * CMostRecentUrls::GetFD(const char * aMode) 
{
    FILE * fd = nsnull;
    nsCOMPtr<nsIFile> file;
    nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(file));
    if (NS_SUCCEEDED(rv)) {
        nsCOMPtr<nsILocalFile> local_file(do_QueryInterface(file));
        local_file->AppendNative(nsEmbedCString("urls.txt"));
        local_file->OpenANSIFileDesc(aMode, &fd);
    }

    return fd;
}

CMostRecentUrls::~CMostRecentUrls() 
{
    FILE * fd = GetFD("w");
    if (fd) {
        for (int i=0;i<MAX_URLS;i++) {
            if(mURLs[i])
                fprintf(fd, "%s\n", mURLs[i]);
        }
    fclose(fd);
    }

    for (int i=0;i<MAX_URLS;i++) {
        if(mURLs[i])
          free(mURLs[i]);
    }
}

char * CMostRecentUrls::GetURL(int aInx)
{
    if (aInx < mNumURLs) {
        return mURLs[aInx];
    }

    return NULL;
}

void CMostRecentUrls::AddURL(const char * aURL)
{
    char szTemp[512];
    strncpy(szTemp, aURL, sizeof(szTemp));
    szTemp[sizeof(szTemp) - 1] = '\0';

    
    int i = 0;
    for (; i<MAX_URLS-1; i++)
    {
        if(mURLs[i])
        {
            if(strcmpi(mURLs[i], szTemp) == 0)
                break; 
        }
    }

    
    

    
    for (; i>0; i--)
    {
        if(mURLs[i])
          free(mURLs[i]);

        if(mURLs[i-1])  
            mURLs[i] = _strdup(mURLs[i-1]);
    }

    
    if(mURLs[0])
        free(mURLs[0]);
    mURLs[0] = _strdup(szTemp);
}
