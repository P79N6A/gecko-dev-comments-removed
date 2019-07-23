








































#include "nsURLHelper.h"
#include "nsEscape.h"
#include "nsILocalFile.h"
#include <windows.h>

nsresult
net_GetURLSpecFromActualFile(nsIFile *aFile, nsACString &result)
{
    nsresult rv;
    nsAutoString path;
  
    
    rv = aFile->GetPath(path);
    if (NS_FAILED(rv)) return rv;
  
    
    path.ReplaceChar(PRUnichar(0x5Cu), PRUnichar(0x2Fu));

    nsCAutoString escPath;

    
    
#ifdef WINCE  
    NS_NAMED_LITERAL_CSTRING(prefix, "file://");
#else  
    NS_NAMED_LITERAL_CSTRING(prefix, "file:///");
#endif  
    
    NS_ConvertUTF16toUTF8 ePath(path);
    if (NS_EscapeURL(ePath.get(), -1, esc_Directory+esc_Forced, escPath))
        escPath.Insert(prefix, 0);
    else
        escPath.Assign(prefix + ePath);

    
    
    
    escPath.ReplaceSubstring(";", "%3b");

    result = escPath;
    return NS_OK;
}

nsresult
net_GetFileFromURLSpec(const nsACString &aURL, nsIFile **result)
{
    nsresult rv;

    nsCOMPtr<nsILocalFile> localFile(
            do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv));
    if (NS_FAILED(rv)) {
        NS_ERROR("Only nsILocalFile supported right now");
        return rv;
    }

    localFile->SetFollowLinks(PR_TRUE);

    const nsACString *specPtr;

    nsCAutoString buf;
    if (net_NormalizeFileURL(aURL, buf))
        specPtr = &buf;
    else
        specPtr = &aURL;
    
    nsCAutoString directory, fileBaseName, fileExtension;
    
    rv = net_ParseFileURL(*specPtr, directory, fileBaseName, fileExtension);
    if (NS_FAILED(rv)) return rv;

    nsCAutoString path;

    if (!directory.IsEmpty()) {
        NS_EscapeURL(directory, esc_Directory|esc_AlwaysCopy, path);
        if (path.Length() > 2 && path.CharAt(2) == '|')
            path.SetCharAt(':', 2);
        path.ReplaceChar('/', '\\');
    }    
    if (!fileBaseName.IsEmpty())
        NS_EscapeURL(fileBaseName, esc_FileBaseName|esc_AlwaysCopy, path);
    if (!fileExtension.IsEmpty()) {
        path += '.';
        NS_EscapeURL(fileExtension, esc_FileExtension|esc_AlwaysCopy, path);
    }
    
    NS_UnescapeURL(path);
    if (path.Length() != strlen(path.get()))
        return NS_ERROR_FILE_INVALID_PATH;

#ifndef WINCE
    
    if (path.CharAt(0) == '\\')
        path.Cut(0, 1);
#endif

    if (IsUTF8(path))
        rv = localFile->InitWithPath(NS_ConvertUTF8toUTF16(path));
        
        
        
        
    else 
        
        rv = localFile->InitWithNativePath(path);

    if (NS_FAILED(rv)) return rv;

    NS_ADDREF(*result = localFile);
    return NS_OK;
}
