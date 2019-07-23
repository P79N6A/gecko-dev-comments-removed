






































#include "nsIFile.h"
#include "nsIGenericFactory.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsLiteralString.h"
#include "nsReadableUtils.h"
#include "nsIStringBundle.h"

#define INCL_WIN
#define INCL_DOS
#include <os2.h>


#include "nsRwsService.h"
#include "rwserr.h"
#include "nsOS2Uni.h"

#include "prenv.h"
#include <stdio.h>





#ifdef DEBUG
  #define RWS_DEBUG
#endif






#ifdef RWS_DEBUG
  #define ERRMSG(x,y)       { printf(y " failed - rc= %d\n", (int)x); }

  #define ERRBREAK(x,y)     if (x) { ERRMSG(x,y); break; }

  #define ERRPRINTF(x,y)    { printf(y "\n", x); }
#else
  #define ERRMSG(x,y)       ;

  #define ERRBREAK(x,y)     if (x) break;

  #define ERRPRINTF(x,y)    ;
#endif





static nsresult IsDescendedFrom(PRUint32 wpsFilePtr, const char *pszClassname);
static nsresult CreateFileForExtension(const char *aFileExt, nsACString& aPath);
static nsresult DeleteFileForExtension(const char *aPath);
static void     AssignNLSString(const PRUnichar *aKey, nsAString& _retval);
static nsresult AssignTitleString(const char *aTitle, nsAString& result);





static nsRwsService *sRwsInstance = 0;   
static PRBool        sInit = FALSE;      


static ULONG (* _System sRwsClientInit)(BOOL);
static ULONG (* _System sRwsGetTimeout)(PULONG, PULONG);
static ULONG (* _System sRwsSetTimeout)(ULONG);
static ULONG (* _System sRwsClientTerminate)(void);


static ULONG (* _System sRwsCall)(PRWSHDR*, ULONG, ULONG, ULONG, ULONG, ULONG, ...);
static ULONG (* _System sRwsCallIndirect)(PRWSBLD);
static ULONG (* _System sRwsFreeMem)(PRWSHDR);
static ULONG (* _System sRwsGetResult)(PRWSHDR, ULONG, PULONG);
static ULONG (* _System sRwsGetArgPtr)(PRWSHDR, ULONG, PRWSDSC*);





typedef struct _ExtInfo
{
  char       ext[8];
  PRUint32   icon;
  PRUint32   mini;
  PRUint32   handler;
  PRUnichar *title;
} ExtInfo;

#define kGrowBy         8
#define kMutexTimeout   500

class ExtCache
{
public:
  ExtCache();
  ~ExtCache();

  nsresult GetIcon(const char *aExt, PRBool aNeedMini, PRUint32 *oIcon);
  nsresult SetIcon(const char *aExt, PRBool aIsMini, PRUint32 aIcon);
  nsresult GetHandler(const char *aExt, PRUint32 *oHandle, nsAString& oTitle);
  nsresult SetHandler(const char *aExt, PRUint32 aHandle, nsAString& aTitle);
  void     EmptyCache();

protected:
  ExtInfo *FindExtension(const char *aExt, PRBool aSet = PR_FALSE);

  PRUint32 mPid;
  PRUint32 mMutex;
  PRUint32 mCount;
  PRUint32 mSize;
  ExtInfo *mExtInfo;
};





NS_IMPL_ISUPPORTS2(nsRwsService, nsIRwsService, nsIObserver)

nsRwsService::nsRwsService()
{
  mExtCache = new ExtCache();
  if (!mExtCache)
    ERRPRINTF("", "nsRwsService - unable to allocate mExtArray%s");
}

nsRwsService::~nsRwsService()
{
}







NS_IMETHODIMP
nsRwsService::IconFromExtension(const char *aExt, PRBool aNeedMini,
                                PRUint32 *_retval)
{
  if (!aExt || !*aExt || !_retval)
    return NS_ERROR_INVALID_ARG;

  nsresult rv = mExtCache->GetIcon(aExt, aNeedMini, _retval);
  if (NS_SUCCEEDED(rv))
    return rv;

  nsCAutoString path;
  rv = CreateFileForExtension(aExt, path);
  if (NS_SUCCEEDED(rv)) {
    rv = IconFromPath(path.get(), PR_FALSE, aNeedMini, _retval);
    DeleteFileForExtension(path.get());
    if (NS_SUCCEEDED(rv))
      mExtCache->SetIcon(aExt, aNeedMini, *_retval);
  }

  return rv;
}







NS_IMETHODIMP
nsRwsService::IconFromPath(const char *aPath, PRBool aAbstract,
                           PRBool aNeedMini, PRUint32 *_retval)
{
  if (!aPath || !*aPath || !_retval)
    return NS_ERROR_INVALID_ARG;

  PRUint32  rwsType;

  if (aAbstract)
    rwsType = (aNeedMini ? RWSC_OFTITLE_OMINI : RWSC_OFTITLE_OICON);
  else
    rwsType = (aNeedMini ? RWSC_OPATH_OMINI : RWSC_OPATH_OICON);

  return RwsConvert(rwsType, (PRUint32)aPath, _retval);
}





NS_IMETHODIMP
nsRwsService::IconFromHandle(PRUint32 aHandle, PRBool aNeedMini,
                             PRUint32 *_retval)
{
  if (!aHandle || !_retval)
    return NS_ERROR_INVALID_ARG;

  return RwsConvert( (aNeedMini ? RWSC_OHNDL_OMINI : RWSC_OHNDL_OICON),
                     aHandle, _retval);
}





NS_IMETHODIMP
nsRwsService::TitleFromHandle(PRUint32 aHandle, nsAString& _retval)
{
  if (!aHandle)
    return NS_ERROR_INVALID_ARG;

  return RwsConvert(RWSC_OHNDL_OTITLE, aHandle, _retval);
}








NS_IMETHODIMP
nsRwsService::HandlerFromExtension(const char *aExt, PRUint32 *aHandle,
                                   nsAString& _retval)
{
  if (!aExt || !*aExt || !aHandle)
    return NS_ERROR_INVALID_ARG;

  nsresult rv = mExtCache->GetHandler(aExt, aHandle, _retval);
  if (NS_SUCCEEDED(rv))
    return rv;

  nsCAutoString path;
  rv = CreateFileForExtension(aExt, path);
  if (NS_SUCCEEDED(rv)) {
    rv = HandlerFromPath(path.get(), aHandle, _retval);
    DeleteFileForExtension(path.get());
    if (NS_SUCCEEDED(rv))
      mExtCache->SetHandler(aExt, *aHandle, _retval);
  }

  return rv;
}








NS_IMETHODIMP
nsRwsService::HandlerFromPath(const char *aPath, PRUint32 *aHandle,
                              nsAString& _retval)
{
  if (!aPath || !*aPath || !aHandle)
    return NS_ERROR_INVALID_ARG;

  nsresult  rv = NS_ERROR_FAILURE;
  PRWSHDR   pHdr = 0;
  PRUint32  rc;

  _retval.Truncate();
  *aHandle = 0;

  
  
  do {
    
    rc = sRwsCall(&pHdr,
                  RWSP_MNAM, (ULONG)"wpQueryDefaultView",
                  RWSR_ASIS,  0, 1,
                  RWSI_OPATH, 0, (ULONG)aPath);
    ERRBREAK(rc, "wpQueryDefaultView")

    PRUint32 defView = sRwsGetResult(pHdr, 0, 0);
    if (defView == (PRUint32)-1)
      break;

    
    
    if (defView == 2)
      defView = 4;

    
    
    PRUint32 wpsFilePtr = sRwsGetResult(pHdr, 1, 0);

    
    sRwsFreeMem(pHdr);
    pHdr = 0;

    
    
    if (defView < 0x6500) {
      rc = sRwsCall(&pHdr,
                    RWSP_MNAM, (ULONG)"wpQueryAssociatedProgram",
                    RWSR_OHNDL,  0, 6,
                    RWSI_ASIS,   0, wpsFilePtr,
                    RWSI_ASIS,   0, defView,
                    RWSI_PULONG, 0, 0,
                    RWSI_PBUF,  32, 0,
                    RWSI_ASIS,   0, 32,
                    RWSI_ASIS,   0, (ULONG)-1);

      
      
      if (rc) {
        if (rc == RWSSRV_FUNCTIONFAILED) {
          *aHandle = 0;
          AssignNLSString(NS_LITERAL_STRING("wpsDefaultOS2").get(), _retval);
          rv = NS_OK;
        }
        else
          ERRMSG(rc, "wpQueryAssociatedProgram")
        break;
      }

      
      
      PRWSDSC pRtn;
      rc = sRwsGetArgPtr(pHdr, 0, &pRtn);
      ERRBREAK(rc, "GetArgPtr")
      *aHandle = *((PRUint32*)pRtn->pget);
      PRUint32 wpsPgmPtr = pRtn->value;

      
      sRwsFreeMem(pHdr);
      pHdr = 0;

      
      
      rc = sRwsCall(&pHdr,
                    RWSP_CONV, 0,
                    RWSR_ASIS, 0, 1,
                    RWSC_OBJ_OTITLE, 0, wpsPgmPtr);
      ERRBREAK(rc, "convert pgm object to title")

      
      
      char *pszTitle = (char*)sRwsGetResult(pHdr, 1, 0);
      if (pszTitle != (char*)-1)
        rv = AssignTitleString(pszTitle, _retval);

      break;
    }

    
    

    
    switch (defView) {
      case 0xbc2b: {
        rv = IsDescendedFrom(wpsFilePtr, "MMImage");
        if (NS_SUCCEEDED(rv))
          AssignNLSString(NS_LITERAL_STRING("mmImageViewerOS2").get(), _retval);
        break;
      }

      case 0xbc0d:    
      case 0xbc21:    
      case 0xbc17:    
      case 0xbbef:    
      case 0xbbe5: {  
        rv = IsDescendedFrom(wpsFilePtr, "MMAudio");
        if (NS_SUCCEEDED(rv)) {
          AssignNLSString(NS_LITERAL_STRING("mmAudioPlayerOS2").get(), _retval);
          break;
        }

        rv = IsDescendedFrom(wpsFilePtr, "MMVideo");
        if (NS_SUCCEEDED(rv)) {
          AssignNLSString(NS_LITERAL_STRING("mmVideoPlayerOS2").get(), _retval);
          break;
        }

        rv = IsDescendedFrom(wpsFilePtr, "MMMIDI");
        if (NS_SUCCEEDED(rv))
          AssignNLSString(NS_LITERAL_STRING("mmMidiPlayerOS2").get(), _retval);

        break;
      }

      case 0x7701: {
        rv = IsDescendedFrom(wpsFilePtr, "TSArcMgr");
        if (NS_SUCCEEDED(rv))
          AssignNLSString(NS_LITERAL_STRING("odZipFolderOS2").get(), _retval);
        break;
      }

      
      
      case 0xb742:
      case 0xa742: {
        rv = IsDescendedFrom(wpsFilePtr, "TSEnhDataFile");
        if (NS_SUCCEEDED(rv))
          AssignNLSString(NS_LITERAL_STRING("odTextViewOS2").get(), _retval);
        break;
      }
    } 

    if (NS_SUCCEEDED(rv))
      break;

    
    

    
    rc = sRwsCall(&pHdr,
                  RWSP_CONV, 0,
                  RWSR_ASIS, 0, 1,
                  RWSC_OBJ_CNAME, 0, wpsFilePtr);
    ERRBREAK(rc, "convert object to classname")

    char *pszTitle = (char*)sRwsGetResult(pHdr, 1, 0);
    if (pszTitle == (char*)-1)
      break;

    nsAutoChar16Buffer buffer;
    PRInt32 bufLength;
    rv = MultiByteToWideChar(0, pszTitle, strlen(pszTitle),
                             buffer, bufLength);
    if (NS_FAILED(rv))
      break;

    nsAutoString classViewer;
    AssignNLSString(NS_LITERAL_STRING("classViewerOS2").get(), classViewer);
    int pos = -1;
    if ((pos = classViewer.Find("%S")) > -1)
      classViewer.Replace(pos, 2, buffer.Elements());
    _retval.Assign(classViewer);
    rv = NS_OK;
  } while (0);

  
  sRwsFreeMem(pHdr);
  return rv;
}







NS_IMETHODIMP
nsRwsService::MenuFromPath(const char *aPath, PRBool aAbstract)
{
  if (!aPath || !*aPath)
    return NS_ERROR_INVALID_ARG;

  nsresult  rv = NS_ERROR_FAILURE;
  PRWSHDR   pHdr = 0;
  PRUint32  type = (aAbstract ? RWSI_OFTITLE : RWSI_OPATH);
  PRUint32  rc;
  POINTL    ptl;
  HWND      hTgt = 0;

  
  if (WinQueryMsgPos(0, &ptl)) {
    hTgt = WinQueryFocus(HWND_DESKTOP);
    if (hTgt)
      WinMapWindowPoints(HWND_DESKTOP, hTgt, &ptl, 1);
  }

  
  
  
  if (hTgt)
      rc = sRwsCall(&pHdr,
                    RWSP_CMD,  RWSCMD_POPUPMENU,
                    RWSR_ASIS, 0, 3,
                    type,      0, (ULONG)aPath,
                    RWSI_ASIS, 0, hTgt,
                    RWSI_PBUF, sizeof(POINTL), (ULONG)&ptl);
  else
      rc = sRwsCall(&pHdr,
                    RWSP_CMD,  RWSCMD_POPUPMENU,
                    RWSR_ASIS, 0, 3,
                    type,      0, (ULONG)aPath,
                    RWSI_ASIS, 0, 0,
                    RWSI_ASIS, 0, 0);

  if (rc)
    ERRMSG(rc, "RWSCMD_POPUPMENU")
  else
    rv = NS_OK;

  
  sRwsFreeMem(pHdr);
  return rv;
}






NS_IMETHODIMP
nsRwsService::OpenWithAppHandle(const char *aFilePath, PRUint32 aAppHandle)
{
  if (!aFilePath || !*aFilePath || !aAppHandle)
    return NS_ERROR_INVALID_ARG;

  nsresult  rv = NS_ERROR_FAILURE;
  PRWSHDR   pHdr = 0;
  PRUint32  rc;

  rc = sRwsCall(&pHdr,
                RWSP_CMD,   RWSCMD_OPENUSING,
                RWSR_ASIS,  0, 2,
                RWSI_OPATH, 0, aFilePath,
                RWSI_OHNDL, 0, aAppHandle);
  if (rc)
    ERRMSG(rc, "RWSCMD_OPENUSING")
  else
    rv = NS_OK;

  sRwsFreeMem(pHdr);
  return rv;
}






NS_IMETHODIMP
nsRwsService::OpenWithAppPath(const char *aFilePath, const char *aAppPath)
{
  if (!aFilePath || !*aFilePath || !aAppPath || !*aAppPath)
    return NS_ERROR_INVALID_ARG;

  nsresult  rv = NS_ERROR_FAILURE;
  PRWSHDR   pHdr = 0;
  PRUint32  rc;

  rc = sRwsCall(&pHdr,
                RWSP_CMD,   RWSCMD_OPENUSING,
                RWSR_ASIS,  0, 2,
                RWSI_OPATH, 0, aFilePath,
                RWSI_OPATH, 0, aAppPath);
  if (rc)
    ERRMSG(rc, "RWSCMD_OPENUSING")
  else
    rv = NS_OK;

  sRwsFreeMem(pHdr);
  return rv;
}









nsresult
nsRwsService::RwsConvert(PRUint32 type, PRUint32 value, PRUint32 *result)
{
  nsresult  rv = NS_ERROR_FAILURE;
  PRWSHDR   pHdr = 0;

  *result = 0;
  PRUint32 rc = sRwsCall(&pHdr,
                         RWSP_CONV, 0,
                         RWSR_ASIS, 0, 1,
                         type,      0, value);

  if (rc)
    ERRMSG(rc, "RwsConvert to ULONG")
  else {
    *result = sRwsGetResult(pHdr, 1, 0);
    if (*result == (PRUint32)-1)
      *result = 0;
    else
      rv = NS_OK;
  }

  sRwsFreeMem(pHdr);
  return rv;
}








nsresult
nsRwsService::RwsConvert(PRUint32 type, PRUint32 value, nsAString& result)
{
  nsresult  rv = NS_ERROR_FAILURE;
  PRWSHDR   pHdr = 0;

  result.Truncate();
  PRUint32 rc = sRwsCall(&pHdr,
                         RWSP_CONV, 0,
                         RWSR_ASIS, 0, 1,
                         type,      0, value);

  if (rc)
    ERRMSG(rc, "RwsConvert to string")
  else {
    char *string = (char*)sRwsGetResult(pHdr, 1, 0);
    if (string != (char*)-1)
      rv = AssignTitleString(string, result);
  }

  sRwsFreeMem(pHdr);
  return rv;
}









NS_IMETHODIMP
nsRwsService::Observe(nsISupports *aSubject, const char *aTopic,
                      const PRUnichar *aSomeData)
{
  if (strcmp(aTopic, "quit-application") == 0) {
    PRUint32 rc = sRwsClientTerminate();
    if (rc)
        ERRMSG(rc, "RwsClientTerminate");

    if (mExtCache)
      mExtCache->EmptyCache();
  }

  return NS_OK;
}







static nsresult IsDescendedFrom(PRUint32 wpsFilePtr, const char *pszClassname)
{
  PRWSHDR   pHdr = 0;
  nsresult  rv = NS_ERROR_FAILURE;

  PRUint32 rc = sRwsCall(&pHdr,
                         RWSP_MNAMI, (ULONG)"somIsA",
                         RWSR_ASIS,  0, 2,
                         RWSI_ASIS,  0, wpsFilePtr,
                         RWSI_CNAME, 0, pszClassname);

  if (rc)
    ERRMSG(rc, "somIsA")
  else
    if (sRwsGetResult(pHdr, 0, 0) == TRUE)
      rv = NS_OK;

  sRwsFreeMem(pHdr);
  return rv;
}





static nsresult CreateFileForExtension(const char *aFileExt,
                                       nsACString& aPath)
{
  nsresult rv = NS_ERROR_FAILURE;
  aPath.Truncate();

  nsCOMPtr<nsIFile> tempFile;
  rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(tempFile));
  if (NS_FAILED(rv))
    return rv;

  nsCAutoString pathStr(NS_LITERAL_CSTRING("nsrws."));
  if (*aFileExt == '.')
    aFileExt++;
  pathStr.Append(aFileExt);

  rv = tempFile->AppendNative(pathStr);
  if (NS_FAILED(rv))
    return rv;

  tempFile->GetNativePath(pathStr);

  FILE *fp = fopen(pathStr.get(), "wb+");
  if (fp) {
    fclose(fp);
    aPath.Assign(pathStr);
    rv = NS_OK;
  }

  return rv;
}





static nsresult DeleteFileForExtension(const char *aPath)
{
  if (!aPath || !*aPath)
    return NS_ERROR_INVALID_ARG;

  remove(aPath);
  return NS_OK;
}






static void AssignNLSString(const PRUnichar *aKey, nsAString& result)
{
  nsresult      rv;
  nsXPIDLString title;

  do {
    nsCOMPtr<nsIStringBundleService> bundleSvc =
      do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
    if (NS_FAILED(rv))
      break;

    nsCOMPtr<nsIStringBundle> bundle;
    rv = bundleSvc->CreateBundle(
      "chrome://mozapps/locale/downloads/unknownContentType.properties",
      getter_AddRefs(bundle));
    if (NS_FAILED(rv))
      break;

    
    rv = bundle->GetStringFromName(aKey, getter_Copies(title));
    if (NS_FAILED(rv))
      rv = bundle->GetStringFromName(NS_LITERAL_STRING("wpsDefaultOS2").get(),
                                     getter_Copies(title));
  } while (0);

  if (NS_SUCCEEDED(rv))
    result.Assign(title);
  else
    result.Assign(NS_LITERAL_STRING("WPS Default"));
}






static nsresult AssignTitleString(const char *aTitle, nsAString& result)
{
  nsAutoChar16Buffer buffer;
  PRInt32 bufLength;

  
  if (NS_FAILED(MultiByteToWideChar(0, aTitle, strlen(aTitle),
                                      buffer, bufLength)))
    return NS_ERROR_FAILURE;

  PRUnichar *pSrc;
  PRUnichar *pDst;
  PRBool     fSkip;

  
  
  for (fSkip=PR_TRUE, pSrc=pDst=buffer.Elements(); *pSrc; pSrc++) {
    if (*pSrc == ' ' || *pSrc == '\r' || *pSrc == '\n' || *pSrc == '\t') {
      if (!fSkip)
        *pDst++ = ' ';
      fSkip = PR_TRUE;
    }
    else {
      if (pDst != pSrc)
        *pDst = *pSrc;
      pDst++;
      fSkip = PR_FALSE;
    }
  }

  
  if (fSkip && pDst > buffer.Elements())
    pDst--;

  *pDst = 0;
  result.Assign(buffer.Elements());
  return NS_OK;
}





ExtCache::ExtCache() : mCount(0), mSize(0), mExtInfo(0)
{
  PTIB  ptib;
  PPIB  ppib;

  
  DosGetInfoBlocks(&ptib, &ppib);
  mPid = ppib->pib_ulpid;

  PRUint32 rc = DosCreateMutexSem(0, (PHMTX)&mMutex, 0, 0);
  if (rc)
    ERRMSG(rc, "DosCreateMutexSem")
}

ExtCache::~ExtCache() {}





nsresult ExtCache::GetIcon(const char *aExt, PRBool aNeedMini,
                           PRUint32 *oIcon)
{
  PRUint32 rc = DosRequestMutexSem(mMutex, kMutexTimeout);
  if (rc) {
    ERRMSG(rc, "DosRequestMutexSem")
    return NS_ERROR_FAILURE;
  }

  ExtInfo *info = FindExtension(aExt);

  if (info) {
    if (aNeedMini)
      *oIcon = info->mini;
    else
      *oIcon = info->icon;
  }
  else
    *oIcon = 0;

  rc = DosReleaseMutexSem(mMutex);
  if (rc)
    ERRMSG(rc, "DosReleaseMutexSem")

  return (*oIcon ? NS_OK : NS_ERROR_FAILURE);
}





nsresult ExtCache::SetIcon(const char *aExt, PRBool aIsMini,
                           PRUint32 aIcon)
{
  PRUint32 rc = DosRequestMutexSem(mMutex, kMutexTimeout);
  if (rc) {
    ERRMSG(rc, "DosRequestMutexSem")
    return NS_ERROR_FAILURE;
  }

  ExtInfo *info = FindExtension(aExt, PR_TRUE);
  if (!info)
    return NS_ERROR_FAILURE;

  
  
  if (!WinSetPointerOwner(aIcon, mPid, FALSE)) {
    ERRPRINTF(info->ext, "WinSetPointerOwner failed for %s icon")
    return NS_ERROR_FAILURE;
  }

  if (aIsMini)
    info->mini = aIcon;
  else
    info->icon = aIcon;

  ERRPRINTF(info->ext, "ExtCache - added icon for %s")

  rc = DosReleaseMutexSem(mMutex);
  if (rc)
    ERRMSG(rc, "DosReleaseMutexSem")

  return NS_OK;
}





nsresult ExtCache::GetHandler(const char *aExt, PRUint32 *oHandle,
                              nsAString& oTitle)
{
  PRUint32 rc = DosRequestMutexSem(mMutex, kMutexTimeout);
  if (rc) {
    ERRMSG(rc, "DosRequestMutexSem")
    return NS_ERROR_FAILURE;
  }

  nsresult rv = NS_ERROR_FAILURE;
  ExtInfo *info = FindExtension(aExt);

  
  if (info && info->title) {
    oTitle.Assign(info->title);
    *oHandle = info->handler;
    rv = NS_OK;
  }

  rc = DosReleaseMutexSem(mMutex);
  if (rc)
    ERRMSG(rc, "DosReleaseMutexSem")

  return rv;
}





nsresult ExtCache::SetHandler(const char *aExt, PRUint32 aHandle,
                              nsAString& aTitle)
{
  PRUint32 rc = DosRequestMutexSem(mMutex, kMutexTimeout);
  if (rc) {
    ERRMSG(rc, "DosRequestMutexSem")
    return NS_ERROR_FAILURE;
  }

  nsresult rv = NS_ERROR_FAILURE;
  ExtInfo *info = FindExtension(aExt, PR_TRUE);

  
  if (info) {
    info->title = ToNewUnicode(aTitle);
    if (info->title) {
      info->handler = aHandle;
      rv = NS_OK;
      ERRPRINTF(info->ext, "ExtCache - added handler for %s")
    }
  }

  rc = DosReleaseMutexSem(mMutex);
  if (rc)
    ERRMSG(rc, "DosReleaseMutexSem")

  return rv;
}






ExtInfo *ExtCache::FindExtension(const char *aExt, PRBool aSet)
{
  
  if (*aExt == '.')
    aExt++;
  if (*aExt == 0)
    return 0;

  
  if (strlen(aExt) >= 8)
    return 0;

  
  char extUpper[16];
  strcpy(extUpper, aExt);
  
  
  
  if (WinUpper(0, 0, 0, extUpper) >= 8)
    return 0;

  
  
  if (aSet && mCount && !strcmp(extUpper, (&mExtInfo[mCount-1])->ext))
    return &mExtInfo[mCount-1];

  ExtInfo *info;
  PRUint32  ctr;

  
  for (ctr = 0, info = mExtInfo; ctr < mCount; ctr++, info++)
    if (!strcmp(extUpper, info->ext))
      return info;

  
  if (mCount >= mSize) {
    PRUint32 newSize = mSize + kGrowBy;
    info = (ExtInfo*) NS_Realloc(mExtInfo, newSize * sizeof(ExtInfo));
    if (!info)
      return 0;

    memset(&info[mSize], 0, kGrowBy * sizeof(ExtInfo));
    mExtInfo = info;
    mSize = newSize;
  }

  
  info = &mExtInfo[mCount++];
  strcpy(info->ext, extUpper);

  return info;
}






void ExtCache::EmptyCache()
{
  if (!mExtInfo)
    return;

  PRUint32 rc = DosRequestMutexSem(mMutex, kMutexTimeout);
  if (rc) {
    ERRMSG(rc, "DosRequestMutexSem")
    return;
  }

  PRUint32  saveMutex = mMutex;
  mMutex = 0;

  PRUint32 ctr;
  ExtInfo *info;

  for (ctr = 0, info = mExtInfo; ctr < mCount; ctr++, info++) {

    ERRPRINTF(info->ext, "ExtCache - deleting entry for %s")

    if (info->icon) {
      if (WinSetPointerOwner(info->icon, mPid, TRUE) == FALSE ||
          WinDestroyPointer(info->icon) == FALSE)
        ERRPRINTF(info->ext, "unable to destroy icon for %s")
    }

    if (info->mini) {
      if (WinSetPointerOwner(info->mini, mPid, TRUE) == FALSE ||
          WinDestroyPointer(info->mini) == FALSE)
        ERRPRINTF(info->ext, "unable to destroy mini for %s")
    }

    if (info->title)
      NS_Free(info->title);
  }

  mCount = 0;
  mSize = 0;
  NS_Free(mExtInfo);
  mExtInfo = 0;

  rc = DosReleaseMutexSem(saveMutex);
  if (rc)
    ERRMSG(rc, "DosReleaseMutexSem")
  rc = DosCloseMutexSem(saveMutex);
  if (rc)
    ERRMSG(rc, "DosCloseMutexSem")
}










static nsresult nsRwsServiceInit(nsRwsService **aClass)
{
  
  if (sInit) {
    *aClass = sRwsInstance;
    if (*aClass == 0)
      return NS_ERROR_NOT_AVAILABLE;

    NS_ADDREF(*aClass);
    return NS_OK;
  }

  sInit = TRUE;
  *aClass = 0;

  
  if (PR_GetEnv("MOZ_NO_RWS"))
    return NS_ERROR_NOT_AVAILABLE;

  char      errBuf[16];
  HMODULE   hmod;

  
  
  
  

  PRUint32  rc = 1;

  
  ULONG  cbClass;
  if (!WinEnumObjectClasses(NULL, &cbClass))
    return NS_ERROR_NOT_AVAILABLE;

  char *pBuf = (char*)NS_Alloc(cbClass + CCHMAXPATH);
  if (!pBuf)
    return NS_ERROR_OUT_OF_MEMORY;

  POBJCLASS pClass = (POBJCLASS)&pBuf[CCHMAXPATH];
  if (!WinEnumObjectClasses(pClass, &cbClass)) {
    NS_Free(pBuf);
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  while (pClass) {
    if (!strcmp(pClass->pszClassName, RWSCLASSNAME))
      break;
    pClass = pClass->pNext;
  }

  
  
  if (pClass && pClass->pszModName[1] == ':') {
    strcpy(pBuf, pClass->pszModName);
    char *ptr = strrchr(pBuf, '\\');
    if (ptr) {
      strcpy(ptr+1, RWSCLIDLL);
      rc = DosLoadModule(errBuf, sizeof(errBuf), pBuf, &hmod);
    }
  }
  NS_Free(pBuf);

  
  if (rc)
    rc = DosLoadModule(errBuf, sizeof(errBuf), RWSCLIMOD, &hmod);

  
  if (rc) {
    ERRPRINTF(RWSCLIDLL, "nsRwsServiceInit - unable to locate %s");
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  if (DosQueryProcAddr(hmod, ORD_RWSCALL,        0, (PFN*)&sRwsCall) ||
      DosQueryProcAddr(hmod, ORD_RWSCALLINDIRECT,0, (PFN*)&sRwsCallIndirect) ||
      DosQueryProcAddr(hmod, ORD_RWSFREEMEM,     0, (PFN*)&sRwsFreeMem) ||
      DosQueryProcAddr(hmod, ORD_RWSGETRESULT,   0, (PFN*)&sRwsGetResult) ||
      DosQueryProcAddr(hmod, ORD_RWSGETARGPTR,   0, (PFN*)&sRwsGetArgPtr) ||
      DosQueryProcAddr(hmod, ORD_RWSCLIENTINIT,  0, (PFN*)&sRwsClientInit) ||
      DosQueryProcAddr(hmod, ORD_RWSGETTIMEOUT,  0, (PFN*)&sRwsGetTimeout) ||
      DosQueryProcAddr(hmod, ORD_RWSSETTIMEOUT,  0, (PFN*)&sRwsSetTimeout) ||
      DosQueryProcAddr(hmod, ORD_RWSCLIENTTERMINATE, 0, (PFN*)&sRwsClientTerminate)) {
    DosFreeModule(hmod);
    ERRPRINTF("", "nsRwsServiceInit - DosQueryProcAddr failed%s")
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  rc = sRwsClientInit(TRUE);
  if (rc) {
    ERRMSG(rc, "RwsClientInit")
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  
  PRUint32  currentTO;
  PRUint32  userTO;
  rc = sRwsGetTimeout((PULONG)&currentTO, (PULONG)&userTO);
  if (rc)
    ERRMSG(rc, "RwsGetTimeout")
  else
    if (userTO == 0) {
      rc = sRwsSetTimeout(2);
      if (rc)
        ERRMSG(rc, "RwsSetTimeout")
    }

  
  NS_NEWXPCOM(sRwsInstance, nsRwsService);
  if (sRwsInstance == 0)
    return NS_ERROR_OUT_OF_MEMORY;

  *aClass = sRwsInstance;
  NS_ADDREF(*aClass);

  
  nsCOMPtr<nsIObserverService> os = do_GetService("@mozilla.org/observer-service;1");
  if (os)
    os->AddObserver(*aClass, "quit-application", PR_FALSE);

  return NS_OK;
}







NS_IMETHODIMP nsRwsServiceConstructor(nsISupports *aOuter, REFNSIID aIID,
                                       void **aResult)
{
  nsresult rv;
  nsRwsService *inst;
  *aResult = NULL;

  if (aOuter) {
    rv = NS_ERROR_NO_AGGREGATION;
    return rv;
  }

  rv = nsRwsServiceInit(&inst);
  if (NS_FAILED(rv)) {
    ERRPRINTF(rv, "==>> nsRwsServiceInit failed - rv= %x");
    return rv;
  }

  rv = inst->QueryInterface(aIID, aResult);
  NS_RELEASE(inst);

  return rv;
}


