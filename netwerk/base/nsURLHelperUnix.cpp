






#include "nsURLHelper.h"
#include "nsEscape.h"
#include "nsIFile.h"
#include "nsNativeCharsetUtils.h"

nsresult 
net_GetURLSpecFromActualFile(nsIFile *aFile, nsACString &result)
{
    nsresult rv;
    nsAutoCString nativePath, ePath;
    nsAutoString path;

    rv = aFile->GetNativePath(nativePath);
    if (NS_FAILED(rv)) return rv;

    
    NS_CopyNativeToUnicode(nativePath, path);
    NS_CopyUnicodeToNative(path, ePath);

    
    if (nativePath == ePath)
        CopyUTF16toUTF8(path, ePath);
    else
        ePath = nativePath;
    
    nsAutoCString escPath;
    NS_NAMED_LITERAL_CSTRING(prefix, "file://");
        
    
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

    nsCOMPtr<nsIFile> localFile;
    rv = NS_NewNativeLocalFile(EmptyCString(), true, getter_AddRefs(localFile));
    if (NS_FAILED(rv))
      return rv;
    
    nsAutoCString directory, fileBaseName, fileExtension, path;

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

    localFile.forget(result);
    return NS_OK;
}
