








































#include "nsURLHelper.h"
#include "nsEscape.h"
#include "nsILocalFile.h"
#include "nsNativeCharsetUtils.h"

nsresult
net_GetURLSpecFromFile(nsIFile *aFile, nsACString &result)
{
    

    nsresult rv;
    nsAutoString path;

    
    rv = aFile->GetPath(path);
    if (NS_FAILED(rv)) return rv;

    nsCAutoString escPath;
    NS_NAMED_LITERAL_CSTRING(prefix, "file://");
        
    
    NS_ConvertUTF16toUTF8 ePath(path);
    if (NS_EscapeURL(ePath.get(), -1, esc_Directory+esc_Forced, escPath))
        escPath.Insert(prefix, 0);
    else
        escPath.Assign(prefix + ePath);

    
    
    escPath.ReplaceSubstring(";", "%3b");

    
    
    
    
    
    if (escPath.Last() != '/') {
        PRBool dir;
        rv = aFile->IsDirectory(&dir);
        if (NS_SUCCEEDED(rv) && dir)
            escPath += '/';
    }
    
    result = escPath;
    return NS_OK;
}

nsresult
net_GetFileFromURLSpec(const nsACString &aURL, nsIFile **result)
{
    
    

    nsresult rv;

    nsCOMPtr<nsILocalFile> localFile;
    rv = NS_NewNativeLocalFile(EmptyCString(), PR_TRUE, getter_AddRefs(localFile));
    if (NS_FAILED(rv))
      return rv;
    
    nsCAutoString directory, fileBaseName, fileExtension, path;

    rv = net_ParseFileURL(aURL, directory, fileBaseName, fileExtension);
    if (NS_FAILED(rv)) return rv;

    if (!directory.IsEmpty())
        NS_EscapeURL(directory, esc_Directory|esc_AlwaysCopy, path);
    if (!fileBaseName.IsEmpty())
        NS_EscapeURL(fileBaseName, esc_FileBaseName|esc_AlwaysCopy, path);
    if (!fileExtension.IsEmpty()) {
        path += '.';
        NS_EscapeURL(fileExtension, esc_FileExtension|esc_AlwaysCopy, path);
    }
    
    NS_UnescapeURL(path);
    if (path.Length() != strlen(path.get()))
        return NS_ERROR_FILE_INVALID_PATH;

    if (IsUTF8(path)) {
        
        
        if (NS_IsNativeUTF8())
            rv = localFile->InitWithNativePath(path);
        else
            rv = localFile->InitWithPath(NS_ConvertUTF8toUTF16(path));
            
            
            
            
    }
    else 
        
        rv = localFile->InitWithNativePath(path);

    if (NS_FAILED(rv)) return rv;

    NS_ADDREF(*result = localFile);
    return NS_OK;
}
