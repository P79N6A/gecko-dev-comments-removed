






#include "nsIFile.h"
#include "mozilla/ModuleUtils.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsLiteralString.h"
#include "nsReadableUtils.h"
#include "nsIStringBundle.h"
#include "mozilla/Services.h"

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





static nsresult IsDescendedFrom(uint32_t wpsFilePtr, const char *pszClassname);
static nsresult CreateFileForExtension(const char *aFileExt, nsACString& aPath);
static nsresult DeleteFileForExtension(const char *aPath);
static void     AssignNLSString(const PRUnichar *aKey, nsAString& _retval);
static nsresult AssignTitleString(const char *aTitle, nsAString& result);





static nsRwsService *sRwsInstance = 0;   
static bool          sInit = FALSE;      


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
  uint32_t   icon;
  uint32_t   mini;
  uint32_t   handler;
  PRUnichar *title;
} ExtInfo;

#define kGrowBy         8
#define kMutexTimeout   500

class ExtCache
{
public:
  ExtCache();
  ~ExtCache();

  nsresult GetIcon(const char *aExt, bool aNeedMini, uint32_t *oIcon);
  nsresult SetIcon(const char *aExt, bool aIsMini, uint32_t aIcon);
  nsresult GetHandler(const char *aExt, uint32_t *oHandle, nsAString& oTitle);
  nsresult SetHandler(const char *aExt, uint32_t aHandle, nsAString& aTitle);
  void     EmptyCache();

protected:
  ExtInfo *FindExtension(const char *aExt, bool aSet = false);

  uint32_t mPid;
  uint32_t mMutex;
  uint32_t mCount;
  uint32_t mSize;
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
nsRwsService::IconFromExtension(const char *aExt, bool aNeedMini,
                                uint32_t *_retval)
{
  if (!aExt || !*aExt || !_retval)
    return NS_ERROR_INVALID_ARG;

  nsresult rv = mExtCache->GetIcon(aExt, aNeedMini, _retval);
  if (NS_SUCCEEDED(rv))
    return rv;

  nsAutoCString path;
  rv = CreateFileForExtension(aExt, path);
  if (NS_SUCCEEDED(rv)) {
    rv = IconFromPath(path.get(), false, aNeedMini, _retval);
    DeleteFileForExtension(path.get());
    if (NS_SUCCEEDED(rv))
      mExtCache->SetIcon(aExt, aNeedMini, *_retval);
  }

  return rv;
}







NS_IMETHODIMP
nsRwsService::IconFromPath(const char *aPath, bool aAbstract,
                           bool aNeedMini, uint32_t *_retval)
{
  if (!aPath || !*aPath || !_retval)
    return NS_ERROR_INVALID_ARG;

  uint32_t  rwsType;

  if (aAbstract)
    rwsType = (aNeedMini ? RWSC_OFTITLE_OMINI : RWSC_OFTITLE_OICON);
  else
    rwsType = (aNeedMini ? RWSC_OPATH_OMINI : RWSC_OPATH_OICON);

  return RwsConvert(rwsType, (uint32_t)aPath, _retval);
}





NS_IMETHODIMP
nsRwsService::IconFromHandle(uint32_t aHandle, bool aNeedMini,
                             uint32_t *_retval)
{
  if (!aHandle || !_retval)
    return NS_ERROR_INVALID_ARG;

  return RwsConvert( (aNeedMini ? RWSC_OHNDL_OMINI : RWSC_OHNDL_OICON),
                     aHandle, _retval);
}





NS_IMETHODIMP
nsRwsService::TitleFromHandle(uint32_t aHandle, nsAString& _retval)
{
  if (!aHandle)
    return NS_ERROR_INVALID_ARG;

  return RwsConvert(RWSC_OHNDL_OTITLE, aHandle, _retval);
}








NS_IMETHODIMP
nsRwsService::HandlerFromExtension(const char *aExt, uint32_t *aHandle,
                                   nsAString& _retval)
{
  if (!aExt || !*aExt || !aHandle)
    return NS_ERROR_INVALID_ARG;

  nsresult rv = mExtCache->GetHandler(aExt, aHandle, _retval);
  if (NS_SUCCEEDED(rv))
    return rv;

  nsAutoCString path;
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
nsRwsService::HandlerFromPath(const char *aPath, uint32_t *aHandle,
                              nsAString& _retval)
{
  if (!aPath || !*aPath || !aHandle)
    return NS_ERROR_INVALID_ARG;

  nsresult  rv = NS_ERROR_FAILURE;
  PRWSHDR   pHdr = 0;
  uint32_t  rc;

  _retval.Truncate();
  *aHandle = 0;

  
  
  do {
    
    rc = sRwsCall(&pHdr,
                  RWSP_MNAM, (ULONG)"wpQueryDefaultView",
                  RWSR_ASIS,  0, 1,
                  RWSI_OPATH, 0, (ULONG)aPath);
    ERRBREAK(rc, "wpQueryDefaultView")

    uint32_t defView = sRwsGetResult(pHdr, 0, 0);
    if (defView == (uint32_t)-1)
      break;

    
    
    if (defView == 2)
      defView = 4;

    
    
    uint32_t wpsFilePtr = sRwsGetResult(pHdr, 1, 0);

    
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
      *aHandle = *((uint32_t*)pRtn->pget);
      uint32_t wpsPgmPtr = pRtn->value;

      
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
    int32_t bufLength;
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
nsRwsService::MenuFromPath(const char *aPath, bool aAbstract)
{
  if (!aPath || !*aPath)
    return NS_ERROR_INVALID_ARG;

  nsresult  rv = NS_ERROR_FAILURE;
  PRWSHDR   pHdr = 0;
  uint32_t  type = (aAbstract ? RWSI_OFTITLE : RWSI_OPATH);
  uint32_t  rc;
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
nsRwsService::OpenWithAppHandle(const char *aFilePath, uint32_t aAppHandle)
{
  if (!aFilePath || !*aFilePath || !aAppHandle)
    return NS_ERROR_INVALID_ARG;

  nsresult  rv = NS_ERROR_FAILURE;
  PRWSHDR   pHdr = 0;
  uint32_t  rc;

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
  uint32_t  rc;

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
nsRwsService::RwsConvert(uint32_t type, uint32_t value, uint32_t *result)
{
  nsresult  rv = NS_ERROR_FAILURE;
  PRWSHDR   pHdr = 0;

  *result = 0;
  uint32_t rc = sRwsCall(&pHdr,
                         RWSP_CONV, 0,
                         RWSR_ASIS, 0, 1,
                         type,      0, value);

  if (rc)
    ERRMSG(rc, "RwsConvert to ULONG")
  else {
    *result = sRwsGetResult(pHdr, 1, 0);
    if (*result == (uint32_t)-1)
      *result = 0;
    else
      rv = NS_OK;
  }

  sRwsFreeMem(pHdr);
  return rv;
}








nsresult
nsRwsService::RwsConvert(uint32_t type, uint32_t value, nsAString& result)
{
  nsresult  rv = NS_ERROR_FAILURE;
  PRWSHDR   pHdr = 0;

  result.Truncate();
  uint32_t rc = sRwsCall(&pHdr,
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
    uint32_t rc = sRwsClientTerminate();
    if (rc)
        ERRMSG(rc, "RwsClientTerminate");

    if (mExtCache)
      mExtCache->EmptyCache();
  }

  return NS_OK;
}







static nsresult IsDescendedFrom(uint32_t wpsFilePtr, const char *pszClassname)
{
  PRWSHDR   pHdr = 0;
  nsresult  rv = NS_ERROR_FAILURE;

  uint32_t rc = sRwsCall(&pHdr,
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

  nsAutoCString pathStr(NS_LITERAL_CSTRING("nsrws."));
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
      mozilla::services::GetStringBundleService();
    if (!bundleSvc)
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
  int32_t bufLength;

  
  if (NS_FAILED(MultiByteToWideChar(0, aTitle, strlen(aTitle),
                                      buffer, bufLength)))
    return NS_ERROR_FAILURE;

  PRUnichar *pSrc;
  PRUnichar *pDst;
  bool       fSkip;

  
  
  for (fSkip=true, pSrc=pDst=buffer.Elements(); *pSrc; pSrc++) {
    if (*pSrc == ' ' || *pSrc == '\r' || *pSrc == '\n' || *pSrc == '\t') {
      if (!fSkip)
        *pDst++ = ' ';
      fSkip = true;
    }
    else {
      if (pDst != pSrc)
        *pDst = *pSrc;
      pDst++;
      fSkip = false;
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

  uint32_t rc = DosCreateMutexSem(0, (PHMTX)&mMutex, 0, 0);
  if (rc)
    ERRMSG(rc, "DosCreateMutexSem")
}

ExtCache::~ExtCache() {}





nsresult ExtCache::GetIcon(const char *aExt, bool aNeedMini,
                           uint32_t *oIcon)
{
  uint32_t rc = DosRequestMutexSem(mMutex, kMutexTimeout);
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





nsresult ExtCache::SetIcon(const char *aExt, bool aIsMini,
                           uint32_t aIcon)
{
  uint32_t rc = DosRequestMutexSem(mMutex, kMutexTimeout);
  if (rc) {
    ERRMSG(rc, "DosRequestMutexSem")
    return NS_ERROR_FAILURE;
  }

  ExtInfo *info = FindExtension(aExt, true);
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





nsresult ExtCache::GetHandler(const char *aExt, uint32_t *oHandle,
                              nsAString& oTitle)
{
  uint32_t rc = DosRequestMutexSem(mMutex, kMutexTimeout);
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





nsresult ExtCache::SetHandler(const char *aExt, uint32_t aHandle,
                              nsAString& aTitle)
{
  uint32_t rc = DosRequestMutexSem(mMutex, kMutexTimeout);
  if (rc) {
    ERRMSG(rc, "DosRequestMutexSem")
    return NS_ERROR_FAILURE;
  }

  nsresult rv = NS_ERROR_FAILURE;
  ExtInfo *info = FindExtension(aExt, true);

  
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






ExtInfo *ExtCache::FindExtension(const char *aExt, bool aSet)
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
  uint32_t  ctr;

  
  for (ctr = 0, info = mExtInfo; ctr < mCount; ctr++, info++)
    if (!strcmp(extUpper, info->ext))
      return info;

  
  if (mCount >= mSize) {
    uint32_t newSize = mSize + kGrowBy;
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

  uint32_t rc = DosRequestMutexSem(mMutex, kMutexTimeout);
  if (rc) {
    ERRMSG(rc, "DosRequestMutexSem")
    return;
  }

  uint32_t  saveMutex = mMutex;
  mMutex = 0;

  uint32_t ctr;
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

  
  
  
  

  uint32_t  rc = 1;

  
  ULONG  cbClass;
  if (!WinEnumObjectClasses(nullptr, &cbClass))
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

  
  
  uint32_t  currentTO;
  uint32_t  userTO;
  rc = sRwsGetTimeout((PULONG)&currentTO, (PULONG)&userTO);
  if (rc)
    ERRMSG(rc, "RwsGetTimeout")
  else
    if (userTO == 0) {
      rc = sRwsSetTimeout(2);
      if (rc)
        ERRMSG(rc, "RwsSetTimeout")
    }

  
  sRwsInstance = new nsRwsService();
  if (sRwsInstance == 0)
    return NS_ERROR_OUT_OF_MEMORY;

  *aClass = sRwsInstance;
  NS_ADDREF(*aClass);

  
  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (os)
    os->AddObserver(*aClass, "quit-application", false);

  return NS_OK;
}







NS_IMETHODIMP nsRwsServiceConstructor(nsISupports *aOuter, REFNSIID aIID,
                                       void **aResult)
{
  nsresult rv;
  nsRwsService *inst;
  *aResult = nullptr;

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


