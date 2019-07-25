











































#include "nsIconChannel.h"
#include "nsIIconURI.h"
#include "nsReadableUtils.h"
#include "nsMemory.h"
#include "nsNetUtil.h"
#include "nsILocalFile.h"
#include "nsIFileURL.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIRwsService.h"

#define INCL_PM
#include <os2.h>




#define FIRSTPEL(x)  (0xF & (x >> 4))
#define SECONDPEL(x) (0xF & x)


#define ALIGNEDBPR(cx,bits) ((( ((cx)*(bits)) + 31) / 32) * 4)


#define UNALIGNEDBPR(cx,bits) (( ((cx)*(bits)) + 7) / 8)


static  HPOINTER GetIcon(nsCString& file, bool fExists,
                         bool fMini, bool *fWpsIcon);
static  void DestroyIcon(HPOINTER hIcon, bool fWpsIcon);

void    ConvertColorBitMap(PRUint8 *inBuf, PBITMAPINFO2 pBMInfo,
                           PRUint8 *outBuf, bool fShrink);
void    ConvertMaskBitMap(PRUint8 *inBuf, PBITMAPINFO2 pBMInfo,
                          PRUint8 *outBuf, bool fShrink);




static bool sUseRws = true;




nsIconChannel::nsIconChannel()
{
}

nsIconChannel::~nsIconChannel()
{}

NS_IMPL_THREADSAFE_ISUPPORTS4(nsIconChannel, 
                              nsIChannel,
                              nsIRequest,
                              nsIRequestObserver,
                              nsIStreamListener)

nsresult nsIconChannel::Init(nsIURI* uri)
{
  NS_ASSERTION(uri, "no uri");
  mUrl = uri;
  mOriginalURI = uri;
  nsresult rv;
  mPump = do_CreateInstance(NS_INPUTSTREAMPUMP_CONTRACTID, &rv);
  return rv;
}




NS_IMETHODIMP nsIconChannel::GetName(nsACString &result)
{
  return mUrl->GetSpec(result);
}

NS_IMETHODIMP nsIconChannel::IsPending(bool *result)
{
  return mPump->IsPending(result);
}

NS_IMETHODIMP nsIconChannel::GetStatus(nsresult *status)
{
  return mPump->GetStatus(status);
}

NS_IMETHODIMP nsIconChannel::Cancel(nsresult status)
{
  return mPump->Cancel(status);
}

NS_IMETHODIMP nsIconChannel::Suspend(void)
{
  return mPump->Suspend();
}

NS_IMETHODIMP nsIconChannel::Resume(void)
{
  return mPump->Resume();
}

NS_IMETHODIMP nsIconChannel::GetLoadGroup(nsILoadGroup* *aLoadGroup)
{
  *aLoadGroup = mLoadGroup;
  NS_IF_ADDREF(*aLoadGroup);
  return NS_OK;
}

NS_IMETHODIMP nsIconChannel::SetLoadGroup(nsILoadGroup* aLoadGroup)
{
  mLoadGroup = aLoadGroup;
  return NS_OK;
}

NS_IMETHODIMP nsIconChannel::GetLoadFlags(PRUint32 *aLoadAttributes)
{
  return mPump->GetLoadFlags(aLoadAttributes);
}

NS_IMETHODIMP nsIconChannel::SetLoadFlags(PRUint32 aLoadAttributes)
{
  return mPump->SetLoadFlags(aLoadAttributes);
}




NS_IMETHODIMP nsIconChannel::GetOriginalURI(nsIURI* *aURI)
{
  *aURI = mOriginalURI;
  NS_ADDREF(*aURI);
  return NS_OK;
}

NS_IMETHODIMP nsIconChannel::SetOriginalURI(nsIURI* aURI)
{
  NS_ENSURE_ARG_POINTER(aURI);
  mOriginalURI = aURI;
  return NS_OK;
}

NS_IMETHODIMP nsIconChannel::GetURI(nsIURI* *aURI)
{
  *aURI = mUrl;
  NS_IF_ADDREF(*aURI);
  return NS_OK;
}

NS_IMETHODIMP
nsIconChannel::Open(nsIInputStream **_retval)
{
  return MakeInputStream(_retval, PR_FALSE);
}

NS_IMETHODIMP nsIconChannel::AsyncOpen(nsIStreamListener *aListener, nsISupports *ctxt)
{
  nsCOMPtr<nsIInputStream> inStream;
  nsresult rv = MakeInputStream(getter_AddRefs(inStream), PR_TRUE);
  if (NS_FAILED(rv))
    return rv;

  
  rv = mPump->Init(inStream, PRInt64(-1), PRInt64(-1), 0, 0, PR_FALSE);
  if (NS_FAILED(rv))
    return rv;

  rv = mPump->AsyncRead(this, ctxt);
  if (NS_SUCCEEDED(rv)) {
    
    mListener = aListener;
    
    if (mLoadGroup)
      mLoadGroup->AddRequest(this, nsnull);
  }
  return rv;
}

nsresult nsIconChannel::ExtractIconInfoFromUrl(nsIFile ** aLocalFile, PRUint32 * aDesiredImageSize, nsACString &aContentType, nsACString &aFileExtension)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsIMozIconURI> iconURI (do_QueryInterface(mUrl, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  iconURI->GetImageSize(aDesiredImageSize);
  iconURI->GetContentType(aContentType);
  iconURI->GetFileExtension(aFileExtension);

  nsCOMPtr<nsIURL> url;
  rv = iconURI->GetIconURL(getter_AddRefs(url));
  if (NS_FAILED(rv) || !url) return NS_OK;

  nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(url, &rv);
  if (NS_FAILED(rv) || !fileURL) return NS_OK;

  nsCOMPtr<nsIFile> file;
  rv = fileURL->GetFile(getter_AddRefs(file));
  if (NS_FAILED(rv) || !file) return NS_OK;

  *aLocalFile = file;
  NS_IF_ADDREF(*aLocalFile);
  return NS_OK;
}








nsresult nsIconChannel::MakeInputStream(nsIInputStream **_retval,
                                        bool nonBlocking)
{

  
  nsCOMPtr<nsIFile> localFile;
  PRUint32 desiredImageSize;
  nsXPIDLCString contentType;
  nsCAutoString filePath;
  nsresult rv = ExtractIconInfoFromUrl(getter_AddRefs(localFile),
                                       &desiredImageSize, contentType,
                                       filePath);
  NS_ENSURE_SUCCESS(rv, rv);

  
  bool fileExists = false;
  if (localFile) {
    localFile->GetNativePath(filePath);
    localFile->Exists(&fileExists);
  }

  
  bool fWpsIcon = false;
  HPOINTER hIcon = GetIcon(filePath, fileExists,
                           desiredImageSize <= 16, &fWpsIcon);
  if (hIcon == NULLHANDLE)
    return NS_ERROR_FAILURE;

  
  POINTERINFO IconInfo;
  if (!WinQueryPointerInfo(hIcon, &IconInfo)) {
    DestroyIcon(hIcon, fWpsIcon);
    return NS_ERROR_FAILURE;
  }

  
  
  bool fShrink = FALSE;
  if (desiredImageSize <= 16) {
    if (IconInfo.hbmMiniPointer) {
      IconInfo.hbmColor = IconInfo.hbmMiniColor;
      IconInfo.hbmPointer = IconInfo.hbmMiniPointer;
    } else {
      fShrink = TRUE;
    }
  }

  
  PBITMAPINFO2  pBMInfo = 0;
  PRUint8*      pInBuf  = 0;
  PRUint8*      pOutBuf = 0;
  HDC           hdc     = 0;
  HPS           hps     = 0;

  
  
  
  do {
    rv = NS_ERROR_FAILURE;

    
    
    BITMAPINFOHEADER2  BMHeader;
    BMHeader.cbFix = sizeof(BMHeader);
    if (!IconInfo.hbmColor ||
        !GpiQueryBitmapInfoHeader(IconInfo.hbmColor, &BMHeader) ||
        BMHeader.cBitCount == 1)
      break;

    
    PRUint32 cbBMInfo = sizeof(BITMAPINFO2) + (sizeof(RGB2) * 255);
    pBMInfo = (PBITMAPINFO2)nsMemory::Alloc(cbBMInfo);
    if (!pBMInfo)
      break;

    
    PRUint32 cbInRow = ALIGNEDBPR(BMHeader.cx, BMHeader.cBitCount);
    PRUint32 cbInBuf = cbInRow * BMHeader.cy;
    pInBuf = (PRUint8*)nsMemory::Alloc(cbInBuf);
    if (!pInBuf)
      break;
    memset(pInBuf, 0, cbInBuf);

    
    PRUint32 cxOut    = fShrink ? BMHeader.cx / 2 : BMHeader.cx;
    PRUint32 cyOut    = fShrink ? BMHeader.cy / 2 : BMHeader.cy;
    PRUint32 cbOutBuf = 2 + ALIGNEDBPR(cxOut, 32) * cyOut;
    pOutBuf = (PRUint8*)nsMemory::Alloc(cbOutBuf);
    if (!pOutBuf)
      break;
    memset(pOutBuf, 0, cbOutBuf);

    
    DEVOPENSTRUC dop = {NULL, "DISPLAY", NULL, NULL, NULL, NULL, NULL, NULL, NULL};
    hdc = DevOpenDC((HAB)0, OD_MEMORY, "*", 5L, (PDEVOPENDATA)&dop, NULLHANDLE);
    if (!hdc)
      break;

    SIZEL sizel = {0,0};
    hps = GpiCreatePS((HAB)0, hdc, &sizel, GPIA_ASSOC | PU_PELS | GPIT_MICRO);
    if (!hps)
      break;

    
    memset(pBMInfo, 0, cbBMInfo);
    *((PBITMAPINFOHEADER2)pBMInfo) = BMHeader;
    GpiSetBitmap(hps, IconInfo.hbmColor);
    if (GpiQueryBitmapBits(hps, 0L, (LONG)BMHeader.cy,
                           (BYTE*)pInBuf, pBMInfo) <= 0)
      break;

    
    
    PRUint8* outPtr = pOutBuf;
    *outPtr++ = (PRUint8)cxOut;
    *outPtr++ = (PRUint8)cyOut;

    
    pBMInfo->cbImage = cbInBuf;
    ConvertColorBitMap(pInBuf, pBMInfo, outPtr, fShrink);

    
    
    outPtr = pOutBuf+2;

    
    BMHeader.cbFix = sizeof(BMHeader);
    if (!GpiQueryBitmapInfoHeader(IconInfo.hbmPointer, &BMHeader))
      break;

    
    cbInRow = ALIGNEDBPR(BMHeader.cx, BMHeader.cBitCount);
    if ((cbInRow * BMHeader.cy) > cbInBuf)  
    {
      cbInBuf = cbInRow * BMHeader.cy;
      nsMemory::Free(pInBuf);
      pInBuf = (PRUint8*)nsMemory::Alloc(cbInBuf);
      memset(pInBuf, 0, cbInBuf);
    }

    
    memset(pBMInfo, 0, cbBMInfo);
    *((PBITMAPINFOHEADER2)pBMInfo) = BMHeader;
    GpiSetBitmap(hps, IconInfo.hbmPointer);
    if (GpiQueryBitmapBits(hps, 0L, (LONG)BMHeader.cy,
                           (BYTE*)pInBuf, pBMInfo) <= 0)
      break;

    
    pBMInfo->cbImage = cbInBuf;
    ConvertMaskBitMap(pInBuf, pBMInfo, outPtr, fShrink);

    
    nsCOMPtr<nsIInputStream> inStream;
    nsCOMPtr<nsIOutputStream> outStream;
    rv = NS_NewPipe(getter_AddRefs(inStream), getter_AddRefs(outStream),
                    cbOutBuf, cbOutBuf, nonBlocking);
    if (NS_FAILED(rv))
      break;

    
    PRUint32 written;
    rv = outStream->Write(reinterpret_cast<const char*>(pOutBuf),
                          cbOutBuf, &written);
    if (NS_FAILED(rv))
      break;

    
    NS_ADDREF(*_retval = inStream);
  } while (0);

  
  if (pOutBuf)
    nsMemory::Free(pOutBuf);
  if (pInBuf)
    nsMemory::Free(pInBuf);
  if (pBMInfo)
    nsMemory::Free(pBMInfo);
  if (hps) {
    GpiAssociate(hps, NULLHANDLE);
    GpiDestroyPS(hps);
  }
  if (hdc)
    DevCloseDC(hdc);
  if (hIcon)
    DestroyIcon(hIcon, fWpsIcon);

  return rv;
}





static HPOINTER GetIcon(nsCString& file, bool fExists,
                        bool fMini, bool *fWpsIcon)
{
  HPOINTER hRtn = 0;
  *fWpsIcon = PR_FALSE;

  if (file.IsEmpty()) {
    
    file.Append("pmwrlw");
  }

  
  if (sUseRws) {
    nsCOMPtr<nsIRwsService> rwsSvc(do_GetService("@mozilla.org/rwsos2;1"));
    if (!rwsSvc)
      sUseRws = PR_FALSE;
    else {
      if (fExists) {
        rwsSvc->IconFromPath(file.get(), PR_FALSE, fMini, (PRUint32*)&hRtn);
      } else {
        const char *ptr = file.get();
        if (*ptr == '.')
          ptr++;
        rwsSvc->IconFromExtension(ptr, fMini, (PRUint32*)&hRtn);
      }
    }
  }

  
  if (hRtn) {
    *fWpsIcon = PR_TRUE;
    return hRtn;
  }

  
  if (fExists)
    return WinLoadFileIcon(file.get(), FALSE);

  
  
  if (file.First() == '.')
    file.Insert("moztmp", 0);

  nsCOMPtr<nsIFile> tempPath;
  if (NS_FAILED(NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(tempPath))) ||
      NS_FAILED(tempPath->AppendNative(file)))
    return 0;

  nsCAutoString pathStr;
  tempPath->GetNativePath(pathStr);
  FILE* fp = fopen(pathStr.get(), "wb+");
  if (fp) {
    fclose(fp);
    hRtn = WinLoadFileIcon(pathStr.get(), FALSE);
    remove(pathStr.get());
  }

  return hRtn;
}



static void DestroyIcon(HPOINTER hIcon, bool fWpsIcon)
{
  if (fWpsIcon)
    WinDestroyPointer(hIcon);
  else
    WinFreeFileIcon(hIcon);

  return;
}









void ConvertColorBitMap(PRUint8 *inBuf, PBITMAPINFO2 pBMInfo,
                        PRUint8 *outBuf, bool fShrink)
{
  PRUint32 next  = fShrink ? 2 : 1;
  PRUint32 bprIn = ALIGNEDBPR(pBMInfo->cx, pBMInfo->cBitCount);
  PRUint8 *pIn   = inBuf + (pBMInfo->cy - 1) * bprIn;
  PRUint8 *pOut  = outBuf;
  PRGB2    pColorTable = &pBMInfo->argbColor[0];

  if (pBMInfo->cBitCount == 4) {
    PRUint32  ubprIn = UNALIGNEDBPR(pBMInfo->cx, pBMInfo->cBitCount);
    PRUint32  padIn  = bprIn - ubprIn;

    for (PRUint32 row = pBMInfo->cy; row > 0; row -= next) {
      for (PRUint32 ndx = 0; ndx < ubprIn; ndx++, pIn++) {
        pOut = 4 + (PRUint8*)memcpy(pOut, &pColorTable[FIRSTPEL(*pIn)], 3);
        if (!fShrink) {
          pOut = 4 + (PRUint8*)memcpy(pOut, &pColorTable[SECONDPEL(*pIn)], 3);
        }
      }
      pIn -= ((next + 1) * bprIn) - padIn;
    }
  } else if (pBMInfo->cBitCount == 8) {
    for (PRUint32 row = pBMInfo->cy; row > 0; row -= next) {
      for (PRUint32 ndx = 0; ndx < bprIn; ndx += next, pIn += next) {
        pOut = 4 + (PRUint8*)memcpy(pOut, &pColorTable[*pIn], 3);
      }
      pIn -= (next + 1) * bprIn;
    }
  } else if (pBMInfo->cBitCount == 24) {
    PRUint32 next3 = next * 3;
    for (PRUint32 row = pBMInfo->cy; row > 0; row -= next) {
      for (PRUint32 ndx = 0; ndx < bprIn; ndx += next3, pIn += next3) {
        pOut = 4 + (PRUint8*)memcpy(pOut, pIn, 3);
      }
      pIn -= (next + 1) * bprIn;
    }
  }
}











void ConvertMaskBitMap(PRUint8 *inBuf, PBITMAPINFO2 pBMInfo,
                       PRUint8 *outBuf, bool fShrink)
{
  PRUint32 next   = (fShrink ? 2 : 1);
  PRUint32 bprIn  = ALIGNEDBPR(pBMInfo->cx, pBMInfo->cBitCount);
  PRUint32 padIn  = bprIn - UNALIGNEDBPR(pBMInfo->cx, pBMInfo->cBitCount);
  PRUint8 *pIn    = inBuf + (pBMInfo->cy - 1) * bprIn;
  PRUint8 *pOut   = outBuf + 3;

  
  for (PRUint32 row = pBMInfo->cy/2; row > 0; row -= next) {

    
    for (PRUint32 bits = pBMInfo->cx; bits; pIn++) {
      PRUint8 src = ~(*pIn);
      PRUint8 srcMask = 0x80;

      
      for ( ; srcMask && bits; srcMask >>= next, bits -= next, pOut += 4) {
        if (src & srcMask) {
          *pOut = 0xff;
        }
      }
    }

    
    
    pIn -= ((next + 1) * bprIn) - padIn;
  }
}



NS_IMETHODIMP nsIconChannel::GetContentType(nsACString &aContentType) 
{
  aContentType.AssignLiteral("image/icon");
  return NS_OK;
}

NS_IMETHODIMP
nsIconChannel::SetContentType(const nsACString &aContentType)
{
  
  
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsIconChannel::GetContentCharset(nsACString &aContentCharset) 
{
  aContentCharset.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
nsIconChannel::SetContentCharset(const nsACString &aContentCharset)
{
  
  
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsIconChannel::GetContentDisposition(PRUint32 *aContentDisposition)
{
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsIconChannel::GetContentDispositionFilename(nsAString &aContentDispositionFilename)
{
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsIconChannel::GetContentDispositionHeader(nsACString &aContentDispositionHeader)
{
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP nsIconChannel::GetContentLength(PRInt32 *aContentLength)
{
  *aContentLength = mContentLength;
  return NS_OK;
}

NS_IMETHODIMP nsIconChannel::SetContentLength(PRInt32 aContentLength)
{
  NS_NOTREACHED("nsIconChannel::SetContentLength");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsIconChannel::GetOwner(nsISupports* *aOwner)
{
  *aOwner = mOwner.get();
  NS_IF_ADDREF(*aOwner);
  return NS_OK;
}

NS_IMETHODIMP nsIconChannel::SetOwner(nsISupports* aOwner)
{
  mOwner = aOwner;
  return NS_OK;
}

NS_IMETHODIMP nsIconChannel::GetNotificationCallbacks(nsIInterfaceRequestor* *aNotificationCallbacks)
{
  *aNotificationCallbacks = mCallbacks.get();
  NS_IF_ADDREF(*aNotificationCallbacks);
  return NS_OK;
}

NS_IMETHODIMP nsIconChannel::SetNotificationCallbacks(nsIInterfaceRequestor* aNotificationCallbacks)
{
  mCallbacks = aNotificationCallbacks;
  return NS_OK;
}

NS_IMETHODIMP nsIconChannel::GetSecurityInfo(nsISupports * *aSecurityInfo)
{
  *aSecurityInfo = nsnull;
  return NS_OK;
}


NS_IMETHODIMP nsIconChannel::OnStartRequest(nsIRequest* aRequest, nsISupports* aContext)
{
  if (mListener)
    return mListener->OnStartRequest(this, aContext);
  return NS_OK;
}

NS_IMETHODIMP nsIconChannel::OnStopRequest(nsIRequest* aRequest, nsISupports* aContext, nsresult aStatus)
{
  if (mListener) {
    mListener->OnStopRequest(this, aContext, aStatus);
    mListener = nsnull;
  }

  
  if (mLoadGroup)
    mLoadGroup->RemoveRequest(this, nsnull, aStatus);

  
  mCallbacks = nsnull;

  return NS_OK;
}



NS_IMETHODIMP nsIconChannel::OnDataAvailable(nsIRequest* aRequest,
                                             nsISupports* aContext,
                                             nsIInputStream* aStream,
                                             PRUint32 aOffset,
                                             PRUint32 aCount)
{
  if (mListener)
    return mListener->OnDataAvailable(this, aContext, aStream, aOffset, aCount);
  return NS_OK;
}



