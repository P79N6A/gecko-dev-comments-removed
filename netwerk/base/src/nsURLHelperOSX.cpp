









































#include "nsURLHelper.h"
#include "nsEscape.h"
#include "nsILocalFile.h"
#include "nsVoidArray.h"
#include "nsReadableUtils.h"
#include <Files.h>

static nsCStringArray *gVolumeList = nsnull;

static PRBool pathBeginsWithVolName(const nsACString& path, nsACString& firstPathComponent)
{
  
  
  
  
  
  if (!gVolumeList) {
    gVolumeList = new nsCStringArray;
    if (!gVolumeList) {
      return PR_FALSE; 
    }
  }

  
  if (!gVolumeList->Count()) {
    OSErr err;
    ItemCount volumeIndex = 1;
    
    do {
      HFSUniStr255 volName;
      FSRef rootDirectory;
      err = ::FSGetVolumeInfo(0, volumeIndex, NULL, kFSVolInfoNone, NULL, &volName, &rootDirectory);
      if (err == noErr) {
        NS_ConvertUTF16toUTF8 volNameStr(Substring((PRUnichar *)volName.unicode,
                                                   (PRUnichar *)volName.unicode + volName.length));
        gVolumeList->AppendCString(volNameStr);
        volumeIndex++;
      }
    } while (err == noErr);
  }
  
  
  nsACString::const_iterator start;
  path.BeginReading(start);
  start.advance(1); 
  nsACString::const_iterator directory_end;
  path.EndReading(directory_end);
  nsACString::const_iterator component_end(start);
  FindCharInReadable('/', component_end, directory_end);
  
  nsCAutoString flatComponent((Substring(start, component_end)));
  NS_UnescapeURL(flatComponent);
  PRInt32 foundIndex = gVolumeList->IndexOf(flatComponent);
  firstPathComponent = flatComponent;
  return (foundIndex != -1);
}

void
net_ShutdownURLHelperOSX()
{
  delete gVolumeList;
  gVolumeList = nsnull;
}

static nsresult convertHFSPathtoPOSIX(const nsACString& hfsPath, nsACString& posixPath)
{
  
  
  
  

  CFStringRef pathStrRef = CFStringCreateWithCString(NULL,
                              PromiseFlatCString(hfsPath).get(),
                              kCFStringEncodingMacRoman);
  if (!pathStrRef)
    return NS_ERROR_FAILURE;

  nsresult rv = NS_ERROR_FAILURE;
  CFURLRef urlRef = CFURLCreateWithFileSystemPath(NULL,
                              pathStrRef, kCFURLHFSPathStyle, true);
  if (urlRef) {
    UInt8 pathBuf[PATH_MAX];
    if (CFURLGetFileSystemRepresentation(urlRef, true, pathBuf, sizeof(pathBuf))) {
      posixPath = (char *)pathBuf;
      rv = NS_OK;
    }
  }
  CFRelease(pathStrRef);
  if (urlRef)
    CFRelease(urlRef);
  return rv;
}

static void SwapSlashColon(char *s)
{
  while (*s) {
    if (*s == '/')
      *s = ':';
    else if (*s == ':')
      *s = '/';
    s++;
  }
} 

nsresult
net_GetURLSpecFromFile(nsIFile *aFile, nsACString &result)
{
  
  
  nsresult rv;
  nsCAutoString ePath;

  
  rv = aFile->GetNativePath(ePath);
  if (NS_FAILED(rv))
    return rv;

  nsCAutoString escPath;
  NS_NAMED_LITERAL_CSTRING(prefix, "file://");
      
  
  if (NS_EscapeURL(ePath.get(), ePath.Length(), esc_Directory+esc_Forced, escPath))
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
  PRBool bHFSPath = PR_FALSE;

  rv = net_ParseFileURL(aURL, directory, fileBaseName, fileExtension);
  if (NS_FAILED(rv))
    return rv;

  if (!directory.IsEmpty()) {
    NS_EscapeURL(directory, esc_Directory|esc_AlwaysCopy, path);

    
    
    
    
    
    nsCAutoString possibleVolName;
    if (pathBeginsWithVolName(directory, possibleVolName)) {        
      
      
      
      
      FSRef testRef;
      possibleVolName.Insert("/", 0);
      if (::FSPathMakeRef((UInt8*)possibleVolName.get(), &testRef, nsnull) != noErr)
        bHFSPath = PR_TRUE;
    }

    if (bHFSPath) {
      
      
      
      path.ReplaceSubstring("%2F", ":");
      path.Cut(0, 1); 
      SwapSlashColon((char *)path.get());
      
      
    }
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

  if (bHFSPath)
    convertHFSPathtoPOSIX(path, path);

  
  rv = localFile->InitWithNativePath(path);
  if (NS_FAILED(rv))
    return rv;

  NS_ADDREF(*result = localFile);
  return NS_OK;
}
