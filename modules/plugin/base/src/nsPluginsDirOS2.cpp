





































#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>

#include "nsPluginsDir.h"
#include "prlink.h"
#include "plstr.h"
#include "prmem.h"
#include "nsPluginDefs.h"

#include "nsString.h"



static char *LoadRCDATAString( HMODULE hMod, ULONG resid)
{
   APIRET rc;
   ULONG  ulSize = 0;
   char  *string = 0;

   rc = DosQueryResourceSize( hMod, RT_RCDATA, resid, &ulSize);

   if( rc == NO_ERROR)
   {
      char *readOnlyString = 0;
      rc = DosGetResource( hMod, RT_RCDATA, resid, (void**) &readOnlyString);

      
      if( readOnlyString[ ulSize - 1] != '\0')
         ulSize++;

      if( rc == NO_ERROR)
      {
         
         string = (char*) PR_Malloc( ulSize);
         memcpy( string, readOnlyString, ulSize - 1);
         string[ ulSize - 1] = '\0';

         DosFreeResource( readOnlyString);
      }
   }

   return string;
}

static PRUint32 CalculateVariantCount(char* mimeTypes)
{
  PRUint32 variants = 1;

  if(mimeTypes == nsnull)
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
  if((variants <= 0) || (data == nsnull))
    return nsnull;

  char ** array = (char **)PR_Calloc(variants, sizeof(char *));
  if(array == nsnull)
    return nsnull;

  char * start = data;
  for(PRUint32 i = 0; i < variants; i++)
  {
    char * p = PL_strchr(start, '|');
    if(p != nsnull)
      *p = 0;

    array[i] = PL_strdup(start);
    start = ++p;
  }
  return array;
}

static void FreeStringArray(PRUint32 variants, char ** array)
{
  if((variants == 0) || (array == nsnull))
    return;

  for(PRUint32 i = 0; i < variants; i++)
  {
    if(array[i] != nsnull)
    {
      PL_strfree(array[i]);
      array[i] = nsnull;
    }
  }
  PR_Free(array);
}



PRBool nsPluginsDir::IsPluginFile(nsIFile* file)
{
    nsCAutoString leaf;
    if (NS_FAILED(file->GetNativeLeafName(leaf)))
        return PR_FALSE;

    const char *leafname = leaf.get();
    
    if( nsnull != leafname)
    {
      int len = strlen( leafname);
      if( len > 6 &&                 
          (0 == strnicmp( &(leafname[len - 4]), ".dll", 4)) &&
          (0 == strnicmp( leafname, "np", 2)))
      {
        return PR_TRUE;
      }
    }
    return PR_FALSE;
}



nsPluginFile::nsPluginFile(nsIFile* file)
: mPlugin(file)
{}

nsPluginFile::~nsPluginFile()
{}


nsresult nsPluginFile::LoadPlugin( PRLibrary *&outLibrary)
{
    if (!mPlugin)
      return NS_ERROR_NULL_POINTER;
   
    nsCAutoString temp;
    mPlugin->GetNativePath(temp);

    outLibrary = PR_LoadLibrary(temp.get());
    return outLibrary == nsnull ? NS_ERROR_FAILURE : NS_OK;
}


nsresult nsPluginFile::GetPluginInfo( nsPluginInfo &info)
{
   nsresult   rc = NS_ERROR_FAILURE;
   HMODULE    hPlug = 0; 
   char       failure[ CCHMAXPATH] = "";
   APIRET     ret;

   const char* path;
   nsCAutoString temp;
   mPlugin->GetNativePath(temp);
   path = temp.get();
   ret = DosLoadModule( failure, CCHMAXPATH, path, &hPlug);

   while( ret == NO_ERROR)
   {
      info.fPluginInfoSize = sizeof( nsPluginInfo);

      info.fName = LoadRCDATAString( hPlug, NS_INFO_ProductName);

      
      info.fDescription = LoadRCDATAString( hPlug, NS_INFO_FileDescription);

      char * mimeType = LoadRCDATAString( hPlug, NS_INFO_MIMEType);
      if( nsnull == mimeType) break;

      char * mimeDescription = LoadRCDATAString( hPlug, NS_INFO_FileOpenName);
      if( nsnull == mimeDescription) break;

      char * extensions = LoadRCDATAString( hPlug, NS_INFO_FileExtents);
      if( nsnull == extensions) break;

      info.fVariantCount = CalculateVariantCount(mimeType);

      info.fMimeTypeArray = MakeStringArray(info.fVariantCount, mimeType);
      if( info.fMimeTypeArray == nsnull) break;

      info.fMimeDescriptionArray = MakeStringArray(info.fVariantCount, mimeDescription);
      if( nsnull == info.fMimeDescriptionArray) break;

      info.fExtensionArray = MakeStringArray(info.fVariantCount, extensions);
      if( nsnull == info.fExtensionArray) break;

      info.fFileName = PL_strdup(path);

      rc = NS_OK;
      break;
   }

   if( 0 != hPlug)
      DosFreeModule( hPlug);

   return rc;
}

nsresult nsPluginFile::FreePluginInfo(nsPluginInfo& info)
{
   if(info.fName != nsnull)
     PL_strfree(info.fName);
 
   if(info.fDescription != nsnull)
     PL_strfree(info.fDescription);
 
   if(info.fMimeTypeArray != nsnull)
     FreeStringArray(info.fVariantCount, info.fMimeTypeArray);
 
   if(info.fMimeDescriptionArray != nsnull)
     FreeStringArray(info.fVariantCount, info.fMimeDescriptionArray);
 
   if(info.fExtensionArray != nsnull)
     FreeStringArray(info.fVariantCount, info.fExtensionArray);
 
   memset((void *)&info, 0, sizeof(info));
 
   return NS_OK;
}
