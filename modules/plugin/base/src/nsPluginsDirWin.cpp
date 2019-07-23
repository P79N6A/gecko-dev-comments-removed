












































#include "nsPluginsDir.h"
#include "prlink.h"
#include "plstr.h"
#include "prmem.h"
#include "prprf.h"

#include "windows.h"
#include "winbase.h"

#include "nsString.h"



static char* GetKeyValue(TCHAR* verbuf, TCHAR* key)
{
  TCHAR *buf = NULL;
  UINT blen;

  ::VerQueryValue(verbuf, key, (void **)&buf, &blen);

  if (buf) {
#ifdef UNICODE
    
    
    return PL_strdup(NS_ConvertUTF16toUTF8(buf).get());
#else
    return PL_strdup(buf);
#endif
  }

  return nsnull;
}

static char* GetVersion(TCHAR* verbuf)
{
  VS_FIXEDFILEINFO *fileInfo;
  UINT fileInfoLen;

  ::VerQueryValue(verbuf, TEXT("\\"), (void **)&fileInfo, &fileInfoLen);

  if (fileInfo) {
    return PR_smprintf("%ld.%ld.%ld.%ld",
                       HIWORD(fileInfo->dwFileVersionMS),
                       LOWORD(fileInfo->dwFileVersionMS),
                       HIWORD(fileInfo->dwFileVersionLS),
                       LOWORD(fileInfo->dwFileVersionLS));
  }

  return nsnull;
}

static PRUint32 CalculateVariantCount(char* mimeTypes)
{
  PRUint32 variants = 1;

  if (!mimeTypes)
    return 0;

  char* index = mimeTypes;
  while (*index) {
    if (*index == '|')
      variants++;

    ++index;
  }
  return variants;
}

static char** MakeStringArray(PRUint32 variants, char* data)
{
  
  
  
  
  

  if ((variants <= 0) || !data)
    return NULL;

  char ** array = (char **)PR_Calloc(variants, sizeof(char *));
  if (!array)
    return NULL;

  char * start = data;

  for (PRUint32 i = 0; i < variants; i++) {
    char * p = PL_strchr(start, '|');
    if (p)
      *p = 0;

    array[i] = PL_strdup(start);

    if (!p) {
      
      
      while(++i < variants)
        array[i] = PL_strdup("");

      break;
    }

    start = ++p;
  }
  return array;
}

static void FreeStringArray(PRUint32 variants, char ** array)
{
  if ((variants == 0) || !array)
    return;

  for (PRUint32 i = 0; i < variants; i++) {
    if (array[i]) {
      PL_strfree(array[i]);
      array[i] = NULL;
    }
  }
  PR_Free(array);
}



PRBool nsPluginsDir::IsPluginFile(nsIFile* file)
{
  PRBool ret = PR_FALSE;
  nsCAutoString path;
  if (NS_FAILED(file->GetNativePath(path)))
    return PR_FALSE;

  const char *pathname = path.get();
  const char* filename;
  char* extension;
  PRUint32 len;
  
  filename = PL_strrchr(pathname, '\\');
  if (filename)
    ++filename;
  else
    filename = pathname;

  len = PL_strlen(filename);
  
  extension = PL_strrchr(filename, '.');
  if (extension)
    ++extension;

  if (len > 5) {
    if (!PL_strncasecmp(filename, "np", 2) && !PL_strcasecmp(extension, "dll")) {
      
      if (!PL_strncasecmp(filename, "npoji", 5))
        return PR_FALSE;
      return PR_TRUE;
    }
  }
  return ret;
}



nsPluginFile::nsPluginFile(nsIFile* file)
: mPlugin(file)
{
  
}

nsPluginFile::~nsPluginFile()
{
  
}





nsresult nsPluginFile::LoadPlugin(PRLibrary* &outLibrary)
{
  
  if (!mPlugin)
    return NS_ERROR_NULL_POINTER;

  nsCAutoString temp;
  mPlugin->GetNativePath(temp);

#ifndef WINCE
  char* index;
  char* pluginFolderPath = PL_strdup(temp.get());

  index = PL_strrchr(pluginFolderPath, '\\');
  if (!index) {
    PL_strfree(pluginFolderPath);
    return NS_ERROR_FILE_INVALID_PATH;
  }
  *index = 0;

  BOOL restoreOrigDir = FALSE;
  char aOrigDir[MAX_PATH + 1];
  DWORD dwCheck = GetCurrentDirectoryA(sizeof(aOrigDir), aOrigDir);
  NS_ASSERTION(dwCheck <= MAX_PATH + 1, "Error in Loading plugin");

  if (dwCheck <= MAX_PATH + 1) {
    restoreOrigDir = SetCurrentDirectoryA(pluginFolderPath);
    NS_ASSERTION(restoreOrigDir, "Error in Loading plugin");
  }
#endif

  outLibrary = PR_LoadLibrary(temp.get());

#ifndef WINCE    
  if (restoreOrigDir) {
    BOOL bCheck = SetCurrentDirectoryA(aOrigDir);
    NS_ASSERTION(bCheck, "Error in Loading plugin");
  }

  PL_strfree(pluginFolderPath);
#endif

  return NS_OK;
}




nsresult nsPluginFile::GetPluginInfo(nsPluginInfo& info)
{
  nsresult rv = NS_OK;
  DWORD zerome, versionsize;
  TCHAR* verbuf = nsnull;

  const TCHAR* path;

  if (!mPlugin)
    return NS_ERROR_NULL_POINTER;

  nsCAutoString fullPath;
  if (NS_FAILED(rv = mPlugin->GetNativePath(fullPath)))
    return rv;

  nsCAutoString fileName;
  if (NS_FAILED(rv = mPlugin->GetNativeLeafName(fileName)))
    return rv;

#ifdef UNICODE
  NS_ConvertASCIItoUTF16 utf16Path(fullPath);
  path = utf16Path.get();
  versionsize = ::GetFileVersionInfoSizeW((TCHAR*)path, &zerome);
#else
  path = fullPath.get();
  versionsize = ::GetFileVersionInfoSize((TCHAR*)path, &zerome);
#endif

  if (versionsize > 0)
    verbuf = (TCHAR*)PR_Malloc(versionsize);
  if (!verbuf)
    return NS_ERROR_OUT_OF_MEMORY;

#ifdef UNICODE
  if (::GetFileVersionInfoW((LPWSTR)path, NULL, versionsize, verbuf))
#else
  if (::GetFileVersionInfo(path, NULL, versionsize, verbuf))
#endif
  {
    info.fName = GetKeyValue(verbuf, TEXT("\\StringFileInfo\\040904E4\\ProductName"));
    info.fDescription = GetKeyValue(verbuf, TEXT("\\StringFileInfo\\040904E4\\FileDescription"));

    char *mimeType = GetKeyValue(verbuf, TEXT("\\StringFileInfo\\040904E4\\MIMEType"));
    char *mimeDescription = GetKeyValue(verbuf, TEXT("\\StringFileInfo\\040904E4\\FileOpenName"));
    char *extensions = GetKeyValue(verbuf, TEXT("\\StringFileInfo\\040904E4\\FileExtents"));

    info.fVariantCount = CalculateVariantCount(mimeType);
    info.fMimeTypeArray = MakeStringArray(info.fVariantCount, mimeType);
    info.fMimeDescriptionArray = MakeStringArray(info.fVariantCount, mimeDescription);
    info.fExtensionArray = MakeStringArray(info.fVariantCount, extensions);
    info.fFullPath = PL_strdup(fullPath.get());
    info.fFileName = PL_strdup(fileName.get());
    info.fVersion = GetVersion(verbuf);

    PL_strfree(mimeType);
    PL_strfree(mimeDescription);
    PL_strfree(extensions);
  }
  else {
    rv = NS_ERROR_FAILURE;
  }

  PR_Free(verbuf);

  return rv;
}

nsresult nsPluginFile::FreePluginInfo(nsPluginInfo& info)
{
  if (info.fName)
    PL_strfree(info.fName);

  if (info.fDescription)
    PL_strfree(info.fDescription);

  if (info.fMimeTypeArray)
    FreeStringArray(info.fVariantCount, info.fMimeTypeArray);

  if (info.fMimeDescriptionArray)
    FreeStringArray(info.fVariantCount, info.fMimeDescriptionArray);

  if (info.fExtensionArray)
    FreeStringArray(info.fVariantCount, info.fExtensionArray);

  if (info.fFullPath)
    PL_strfree(info.fFullPath);

  if (info.fFileName)
    PL_strfree(info.fFileName);

  if (info.fVersion)
    PR_smprintf_free(info.fVersion);

  ZeroMemory((void *)&info, sizeof(info));

  return NS_OK;
}
