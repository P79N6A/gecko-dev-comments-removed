






































#include "nsBuildID.h"

#include "nsEmbedString.h"
#include "nsXPCOMPrivate.h"
#include "nsXPCOMGlue.h"
#include "nsILocalFile.h"
#include "nsIDirectoryService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsCOMPtr.h"
#include "nsMemory.h"

#include "nspr.h"
#include "plstr.h"

#ifdef XP_WIN32
#include <windows.h>
#include <stdlib.h>
#include <mbstring.h>
#elif defined(XP_OS2)
#define INCL_DOS
#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include "prenv.h"
#elif defined(XP_MACOSX)
#include <Processes.h>
#include <CFBundle.h>
#elif defined(XP_UNIX)
#include <unistd.h>
#include <stdlib.h>
#include <sys/param.h>
#include "prenv.h"
#elif defined(XP_BEOS)
#include <stdlib.h>
#include <image.h>
#endif

#include <sys/stat.h>

#include "nsGREDirServiceProvider.h"

static PRBool GRE_GetCurrentProcessDirectory(char* buffer);





NS_IMPL_ISUPPORTS1(nsGREDirServiceProvider, nsIDirectoryServiceProvider)
  




NS_IMETHODIMP
nsGREDirServiceProvider::GetFile(const char *prop, PRBool *persistent, nsIFile **_retval)
{
  *_retval = nsnull;
  *persistent = PR_TRUE;

  
  
  
  
  
  
  
  
  
  if(strcmp(prop, NS_GRE_DIR) == 0)
  {
    nsILocalFile* lfile = nsnull;
    nsresult rv = GRE_GetGREDirectory(&lfile);
    *_retval = lfile;
    return rv;
  }

  return NS_ERROR_FAILURE;
}





PRBool
GRE_GetCurrentProcessDirectory(char* buffer)
{
    *buffer = '\0';

#ifdef XP_WIN
    DWORD bufLength = ::GetModuleFileName(0, buffer, MAXPATHLEN);
    if (bufLength == 0 || bufLength == MAXPATHLEN)
        return PR_FALSE;
    
    unsigned char* lastSlash = _mbsrchr((unsigned char*) buffer, '\\');
    if (lastSlash) {
        *(lastSlash) = '\0';
        return PR_TRUE;
    }

#elif defined(XP_MACOSX)
    
    CFBundleRef appBundle = CFBundleGetMainBundle();
    if (appBundle != nsnull)
    {
        CFURLRef bundleURL = CFBundleCopyExecutableURL(appBundle);
        if (bundleURL != nsnull)
        {
            CFURLRef parentURL = CFURLCreateCopyDeletingLastPathComponent(kCFAllocatorDefault, bundleURL);
            if (parentURL)
            {
                CFStringRef path = CFURLCopyFileSystemPath(parentURL, kCFURLPOSIXPathStyle);
                if (path)
                {
                    CFStringGetCString(path, buffer, MAXPATHLEN, kCFStringEncodingUTF8);
                    CFRelease(path);
                }
                CFRelease(parentURL);
            }
            CFRelease(bundleURL);
        }
    }
    if (*buffer) return PR_TRUE;

#elif defined(XP_UNIX)

    
    
    
    

    
    
    
    
    
    
    
    
    
#ifdef MOZ_DEFAULT_MOZILLA_FIVE_HOME
    if (getenv("MOZILLA_FIVE_HOME") == nsnull)
    {
        putenv("MOZILLA_FIVE_HOME=" MOZ_DEFAULT_MOZILLA_FIVE_HOME);
    }
#endif

    char *moz5 = getenv("MOZILLA_FIVE_HOME");

    if (moz5 && *moz5)
    {
        if (!realpath(moz5, buffer))
            strcpy(buffer, moz5);

        return PR_TRUE;
    }
    else
    {
#if defined(DEBUG)
        static PRBool firstWarning = PR_TRUE;

        if(firstWarning) {
            
            printf("Warning: MOZILLA_FIVE_HOME not set.\n");
            firstWarning = PR_FALSE;
        }
#endif 

        
        if (getcwd(buffer, MAXPATHLEN))
        {
            return PR_TRUE;
        }
    }

#elif defined(XP_OS2)
    PPIB ppib;
    PTIB ptib;
    char* p;
    DosGetInfoBlocks( &ptib, &ppib);
    DosQueryModuleName( ppib->pib_hmte, MAXPATHLEN, buffer);
    p = strrchr( buffer, '\\'); 
    if (p) {
      *p  = '\0';
      return PR_TRUE;
    }

#elif defined(XP_BEOS)

    int32 cookie = 0;
    image_info info;
    char *lastSlash;
    *buffer = 0;
    if (get_next_image_info(0, &cookie, &info) == B_OK)
    {
      strcpy(buffer, info.name);
      if ((lastSlash = strrchr(buffer, '/')) != 0)
      {
        *lastSlash = '\0';
        return PR_TRUE;
      }
    }

#endif
    
  return PR_FALSE;
}






static char sXPCOMPath[MAXPATHLEN] = "";

extern "C" char const *
GRE_GetXPCOMPath()
{
  
  if (*sXPCOMPath)
    return sXPCOMPath;

  char buffer[MAXPATHLEN];
    
  
  
  
  if (GRE_GetCurrentProcessDirectory(buffer)) {
    PRUint32 pathlen = strlen(buffer);
    strcpy(buffer + pathlen, XPCOM_FILE_PATH_SEPARATOR XPCOM_DLL);

    struct stat libStat;
    int statResult = stat(buffer, &libStat);
        
    if (statResult != -1) {
      
      strcpy(sXPCOMPath, buffer);
      return sXPCOMPath;
    }
  }

  static const GREVersionRange version = {
    GRE_BUILD_ID, PR_TRUE,
    GRE_BUILD_ID, PR_TRUE
  };

  GRE_GetGREPathWithProperties(&version, 1,
                               nsnull, 0,
                               sXPCOMPath, MAXPATHLEN);
  if (*sXPCOMPath)
    return sXPCOMPath;

  return nsnull;
}

extern "C" nsresult
GRE_GetGREDirectory(nsILocalFile* *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  nsresult rv = NS_ERROR_FAILURE;

  
  

  const char *pGREDir = GRE_GetXPCOMPath();
  if(!pGREDir)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsILocalFile> xpcomPath;
  nsEmbedCString leaf(pGREDir);
  rv = NS_NewNativeLocalFile(leaf, PR_TRUE, getter_AddRefs(xpcomPath));

  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIFile> directory;
  rv = xpcomPath->GetParent(getter_AddRefs(directory));
  if (NS_FAILED(rv))
    return rv;

  return CallQueryInterface(directory, _retval);
}
