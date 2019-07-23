












































#include "nsPluginsDir.h"
#include "prlink.h"
#include "plstr.h"
#include "prmem.h"
#include "prprf.h"

#include "windows.h"
#include "winbase.h"

#include "nsString.h"





static char* GetKeyValue(wchar_t* verbuf, wchar_t* key)
{
	wchar_t     *buf = NULL;
	UINT        blen;

	::VerQueryValueW(verbuf,
					key,
					(void **)&buf, &blen);

	if(buf != NULL)
	{
        return strdup(NS_ConvertUTF16toUTF8(buf).get());
	}

	return nsnull;
}

static PRUint32 CalculateVariantCount(char* mimeTypes)
{
	PRUint32 variants = 1;

  if(mimeTypes == NULL)
    return 0;

	char* index = mimeTypes;
	while (*index)
	{
		if (*index == '|')
			variants++;

		++index;
	}
	return variants;
}

static char** MakeStringArray(PRUint32 variants, char* data)
{






  if((variants <= 0) || (data == NULL))
    return NULL;

  char ** array = (char **)PR_Calloc(variants, sizeof(char *));
  if(array == NULL)
    return NULL;

  char * start = data;

  for(PRUint32 i = 0; i < variants; i++)
  {
    char * p = PL_strchr(start, '|');
    if(p != NULL)
      *p = 0;

    array[i] = PL_strdup(start);

    if(p == NULL)
    { 
      
      
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
  if((variants == 0) || (array == NULL))
    return;

  for(PRUint32 i = 0; i < variants; i++)
  {
    if(array[i] != NULL)
    {
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
	if(filename)
		++filename;
	else
		filename = pathname;

	len = PL_strlen(filename);
	
	extension = PL_strrchr(filename, '.');
	if(extension)
	    ++extension;

	if(len > 5)
 	{
		if(!PL_strncasecmp(filename, "np", 2) && !PL_strcasecmp(extension, "dll"))
 			return PR_TRUE;
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

    nsAutoString temp;
    mPlugin->GetPath(temp);

    PRUnichar* index;
    PRUnichar* pluginFolderPath = _wcsdup(temp.get());
    
    index = wcsrchr(pluginFolderPath, '\\');
    *index = 0;
    
	BOOL restoreOrigDir = FALSE;
	PRUnichar aOrigDir[MAX_PATH + 1];
	DWORD dwCheck = ::GetCurrentDirectoryW(sizeof(aOrigDir), aOrigDir);
	NS_ASSERTION(dwCheck <= MAX_PATH + 1, "Error in Loading plugin");

	if (dwCheck <= MAX_PATH + 1)
  	{
		restoreOrigDir = ::SetCurrentDirectoryW(pluginFolderPath);
		NS_ASSERTION(restoreOrigDir, "Error in Loading plugin");
    }
    
	outLibrary = PR_LoadLibrary(NS_ConvertUTF16toUTF8(temp).get());
    
	if (restoreOrigDir)
	{
        BOOL bCheck = ::SetCurrentDirectoryW(aOrigDir);
		NS_ASSERTION(bCheck, "Error in Loading plugin");
    }
    
    free(pluginFolderPath);
    
	return NS_OK;
}




nsresult nsPluginFile::GetPluginInfo(nsPluginInfo& info)
{
    nsresult res = NS_OK;
	DWORD zerome, versionsize;
	PRUnichar* verbuf = nsnull;

	const PRUnichar* path;

    if (!mPlugin)
        return NS_ERROR_NULL_POINTER;

    nsAutoString temp;
    mPlugin->GetPath(temp);
    path = temp.get();
    
    versionsize = ::GetFileVersionInfoSizeW(path, &zerome);
	if (versionsize > 0)
		verbuf = (wchar_t *)PR_Malloc(versionsize);
	if(!verbuf)
		return NS_ERROR_OUT_OF_MEMORY;
    
	if(::GetFileVersionInfoW(path, NULL, versionsize, verbuf))
    {
        info.fName = GetKeyValue(verbuf, L"\\StringFileInfo\\040904E4\\ProductName");
		info.fDescription = GetKeyValue(verbuf, L"\\StringFileInfo\\040904E4\\FileDescription");

		char *mimeType = GetKeyValue(verbuf, L"\\StringFileInfo\\040904E4\\MIMEType");
		char *mimeDescription = GetKeyValue(verbuf, L"\\StringFileInfo\\040904E4\\FileOpenName");
		char *extensions = GetKeyValue(verbuf, L"\\StringFileInfo\\040904E4\\FileExtents");

		info.fVariantCount = CalculateVariantCount(mimeType);
		info.fMimeTypeArray = MakeStringArray(info.fVariantCount, mimeType);
		info.fMimeDescriptionArray = MakeStringArray(info.fVariantCount, mimeDescription);
		info.fExtensionArray = MakeStringArray(info.fVariantCount, extensions);

        
        info.fFileName = PL_strdup(NS_ConvertUTF16toUTF8(path).get());
        
        PL_strfree(mimeType);
        PL_strfree(mimeDescription);
        PL_strfree(extensions);
	}
	else
		res = NS_ERROR_FAILURE;


	PR_Free(verbuf);

  return res;
}

nsresult nsPluginFile::FreePluginInfo(nsPluginInfo& info)
{
  if(info.fName != NULL)
    PL_strfree(info.fName);

  if(info.fDescription != NULL)
    PL_strfree(info.fDescription);

  if(info.fMimeTypeArray != NULL)
    FreeStringArray(info.fVariantCount, info.fMimeTypeArray);

  if(info.fMimeDescriptionArray != NULL)
    FreeStringArray(info.fVariantCount, info.fMimeDescriptionArray);

  if(info.fExtensionArray != NULL)
    FreeStringArray(info.fVariantCount, info.fExtensionArray);

  if(info.fFileName != NULL)
    PL_strfree(info.fFileName);

  ZeroMemory((void *)&info, sizeof(info));

  return NS_OK;
}
