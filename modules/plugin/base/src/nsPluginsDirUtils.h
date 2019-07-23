




































#ifndef nsPluginsDirUtils_h___
#define nsPluginsDirUtils_h___

#include "nsPluginsDir.h"
#include "nsVoidArray.h"
#include "prmem.h"








static nsresult
ParsePluginMimeDescription(const char *mdesc, nsPluginInfo &info)
{
    nsresult rv = NS_ERROR_FAILURE;
    if (!mdesc || !*mdesc)
       return rv;

    char *mdescDup = PL_strdup(mdesc); 
    char anEmptyString[] = "";
    nsAutoVoidArray tmpMimeTypeArr;
    char delimiters[] = {':',':',';'};
    int mimeTypeVariantCount = 0;
    char *p = mdescDup; 
    while(p) {
        char *ptrMimeArray[] = {anEmptyString, anEmptyString, anEmptyString};

        
        
        
        
        
        
        
        
        
        

        char *s = p;
        int i;
        for (i = 0; i < (int) sizeof(delimiters) && (p = PL_strchr(s, delimiters[i])); i++) {
            ptrMimeArray[i] = s; 
            *p++ = 0; 
            s = p; 
        }
        if (i == 2)
           ptrMimeArray[i] = s;
        
        
        if (ptrMimeArray[0] != anEmptyString) {
            tmpMimeTypeArr.AppendElement((void*) ptrMimeArray[0]);
            tmpMimeTypeArr.AppendElement((void*) ptrMimeArray[1]);
            tmpMimeTypeArr.AppendElement((void*) ptrMimeArray[2]);
            mimeTypeVariantCount++;
        }
    }

    
    if (mimeTypeVariantCount) {
        info.fVariantCount         = mimeTypeVariantCount;
        
        info.fMimeTypeArray        = (char **)PR_Malloc(mimeTypeVariantCount * sizeof(char *));
        info.fMimeDescriptionArray = (char **)PR_Malloc(mimeTypeVariantCount * sizeof(char *));
        info.fExtensionArray       = (char **)PR_Malloc(mimeTypeVariantCount * sizeof(char *));

        int j,i;
        for (j = i = 0; i < mimeTypeVariantCount; i++) {
            
           
            info.fMimeTypeArray[i]        =  PL_strdup((char*) tmpMimeTypeArr.ElementAt(j++));
            info.fExtensionArray[i]       =  PL_strdup((char*) tmpMimeTypeArr.ElementAt(j++));
            info.fMimeDescriptionArray[i] =  PL_strdup((char*) tmpMimeTypeArr.ElementAt(j++));
        }
        rv = NS_OK;
    }
    if (mdescDup)
        PR_Free(mdescDup);
    return rv;
}

#endif 
