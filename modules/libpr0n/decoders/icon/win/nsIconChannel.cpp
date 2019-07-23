









































#include "nsIconChannel.h"
#include "nsIIconURI.h"
#include "nsIServiceManager.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsMimeTypes.h"
#include "nsMemory.h"
#include "nsIStringStream.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsInt64.h"
#include "nsIFile.h"
#include "nsIFileURL.h"
#include "nsIMIMEService.h"
#include "nsCExternalHandlerService.h"
#include "nsDirectoryServiceDefs.h"


#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

struct ICONFILEHEADER {
  PRUint16 ifhReserved;
  PRUint16 ifhType;
  PRUint16 ifhCount;
};

struct ICONENTRY {
  PRInt8 ieWidth;
  PRInt8 ieHeight;
  PRUint8 ieColors;
  PRUint8 ieReserved;
  PRUint16 iePlanes;
  PRUint16 ieBitCount;
  PRUint32 ieSizeImage;
  PRUint32 ieFileOffset;
};


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

  nsresult rv;
  mPump = do_CreateInstance(NS_INPUTSTREAMPUMP_CONTRACTID, &rv);
  return rv;
}




NS_IMETHODIMP nsIconChannel::GetName(nsACString &result)
{
  return mUrl->GetSpec(result);
}

NS_IMETHODIMP nsIconChannel::IsPending(PRBool *result)
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
  *aURI = mOriginalURI ? mOriginalURI : mUrl;
  NS_ADDREF(*aURI);
  return NS_OK;
}

NS_IMETHODIMP nsIconChannel::SetOriginalURI(nsIURI* aURI)
{
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

nsresult nsIconChannel::ExtractIconInfoFromUrl(nsIFile ** aLocalFile, PRUint32 * aDesiredImageSize, nsACString &aContentType, nsACString &aFileExtension)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsIMozIconURI> iconURI (do_QueryInterface(mUrl, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  iconURI->GetImageSize(aDesiredImageSize);
  iconURI->GetContentType(aContentType);
  iconURI->GetFileExtension(aFileExtension);

  nsCOMPtr<nsIURI> fileURI;
  rv = iconURI->GetIconFile(getter_AddRefs(fileURI));
  if (NS_FAILED(rv) || !fileURI) return NS_OK;

  nsCOMPtr<nsIFileURL>    fileURL = do_QueryInterface(fileURI, &rv);
  if (NS_FAILED(rv) || !fileURL) return NS_OK;

  nsCOMPtr<nsIFile> file;
  rv = fileURL->GetFile(getter_AddRefs(file));
  if (NS_FAILED(rv) || !file) return NS_OK;

  return file->Clone(aLocalFile);
}

NS_IMETHODIMP nsIconChannel::AsyncOpen(nsIStreamListener *aListener, nsISupports *ctxt)
{
  nsCOMPtr<nsIInputStream> inStream;
  nsresult rv = MakeInputStream(getter_AddRefs(inStream), PR_TRUE);
  if (NS_FAILED(rv))
    return rv;

  
  rv = mPump->Init(inStream, nsInt64(-1), nsInt64(-1), 0, 0, PR_FALSE);
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

static DWORD GetSpecialFolderIcon(nsIFile* aFile, int aFolder, SHFILEINFO* aSFI, UINT aInfoFlags)
{
  DWORD shellResult = 0;

  if (!aFile)
    return shellResult;

  char fileNativePath[MAX_PATH];
  nsCAutoString fileNativePathStr;
  aFile->GetNativePath(fileNativePathStr);
  ::GetShortPathName(fileNativePathStr.get(), fileNativePath, sizeof(fileNativePath));

  LPITEMIDLIST idList;
  HRESULT hr = ::SHGetSpecialFolderLocation(NULL, aFolder, &idList);
  if (SUCCEEDED(hr)) {
    char specialNativePath[MAX_PATH];
    ::SHGetPathFromIDList(idList, specialNativePath);
    ::GetShortPathName(specialNativePath, specialNativePath, sizeof(specialNativePath));
  
    if (nsDependentCString(fileNativePath).EqualsIgnoreCase(specialNativePath)) {
      aInfoFlags |= (SHGFI_PIDL | SHGFI_SYSICONINDEX);
      shellResult = ::SHGetFileInfo((LPCTSTR)(LPCITEMIDLIST)idList, 0, aSFI, 
                                    sizeof(SHFILEINFO), aInfoFlags);
      IMalloc* pMalloc;
      hr = ::SHGetMalloc(&pMalloc);
      if (SUCCEEDED(hr)) {
        pMalloc->Free(idList);
        pMalloc->Release();
      }
    }
  }
  return shellResult;
}

nsresult nsIconChannel::MakeInputStream(nsIInputStream** _retval, PRBool nonBlocking)
{
  nsXPIDLCString contentType;
  nsCAutoString filePath;
  nsCOMPtr<nsIFile> localFile; 
  PRUint32 desiredImageSize;
  nsresult rv = ExtractIconInfoFromUrl(getter_AddRefs(localFile), &desiredImageSize, contentType, filePath);
  NS_ENSURE_SUCCESS(rv, rv);

  
  SHFILEINFO      sfi;
  UINT infoFlags = SHGFI_ICON;
  
  PRBool fileExists = PR_FALSE;
 
  if (localFile)
  {
    rv = localFile->Normalize();
    NS_ENSURE_SUCCESS(rv, rv);

    localFile->GetNativePath(filePath);
    if (filePath.Length() < 2 || filePath[1] != ':')
      return NS_ERROR_MALFORMED_URI; 

    if (filePath.Last() == ':')
      filePath.Append('\\');
    else {
      localFile->Exists(&fileExists);
      if (!fileExists)
       localFile->GetNativeLeafName(filePath);
    }
  }

  if (!fileExists)
   infoFlags |= SHGFI_USEFILEATTRIBUTES;

  if (desiredImageSize > 16)
    infoFlags |= SHGFI_SHELLICONSIZE;
  else
    infoFlags |= SHGFI_SMALLICON;

  
  
  if (!fileExists && !contentType.IsEmpty())
  {
    nsCOMPtr<nsIMIMEService> mimeService (do_GetService(NS_MIMESERVICE_CONTRACTID, &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCAutoString fileExt;
    mimeService->GetPrimaryExtension(contentType, EmptyCString(), fileExt);
    
    
    
    filePath = NS_LITERAL_CSTRING(".") + fileExt;
  }

  rv = NS_ERROR_NOT_AVAILABLE;

  
  DWORD shellResult = GetSpecialFolderIcon(localFile, CSIDL_DESKTOP, &sfi, infoFlags);
  if (!shellResult) {
    
    shellResult = GetSpecialFolderIcon(localFile, CSIDL_PERSONAL, &sfi, infoFlags);
  }

  
  
  
  

  
  if (!shellResult)
    shellResult = ::SHGetFileInfo(filePath.get(), FILE_ATTRIBUTE_ARCHIVE, &sfi, sizeof(sfi), infoFlags);

  if (shellResult && sfi.hIcon)
  {
    
    ICONINFO iconInfo;
    if (GetIconInfo(sfi.hIcon, &iconInfo))
    {
      
      HDC hDC = CreateCompatibleDC(NULL); 
      BITMAPINFO maskInfo = {{sizeof(BITMAPINFOHEADER)}};
      if (GetDIBits(hDC, iconInfo.hbmMask, 0, 0, NULL, &maskInfo, DIB_RGB_COLORS) &&
          maskInfo.bmiHeader.biSizeImage > 0) {
        PRUint32 colorSize = maskInfo.bmiHeader.biWidth * maskInfo.bmiHeader.biHeight * 4;
        PRUint32 iconSize = sizeof(ICONFILEHEADER) + sizeof(ICONENTRY) + sizeof(BITMAPINFOHEADER) + colorSize + maskInfo.bmiHeader.biSizeImage;
        char *buffer = new char[iconSize];
        if (!buffer)
          rv = NS_ERROR_OUT_OF_MEMORY;
        else {
          
          ICONFILEHEADER *iconHeader = (ICONFILEHEADER *)buffer;
          iconHeader->ifhReserved = 0;
          iconHeader->ifhType = 1;
          iconHeader->ifhCount = 1;
          
          ICONENTRY *iconEntry = (ICONENTRY *)(buffer + sizeof(ICONFILEHEADER));
          iconEntry->ieWidth = maskInfo.bmiHeader.biWidth;
          iconEntry->ieHeight = maskInfo.bmiHeader.biHeight;
          iconEntry->ieColors = 0;
          iconEntry->ieReserved = 0;
          iconEntry->iePlanes = 1;
          iconEntry->ieBitCount = 32;
          iconEntry->ieSizeImage = sizeof(BITMAPINFOHEADER) + colorSize + maskInfo.bmiHeader.biSizeImage;
          iconEntry->ieFileOffset = sizeof(ICONFILEHEADER) + sizeof(ICONENTRY);
          
          LPBITMAPINFO lpBitmapInfo = (LPBITMAPINFO)(buffer + sizeof(ICONFILEHEADER) + sizeof(ICONENTRY));
          memcpy(lpBitmapInfo, &maskInfo.bmiHeader, sizeof(BITMAPINFOHEADER));
          if (GetDIBits(hDC, iconInfo.hbmMask, 0, maskInfo.bmiHeader.biHeight, buffer + sizeof(ICONFILEHEADER) + sizeof(ICONENTRY) + sizeof(BITMAPINFOHEADER) + colorSize, lpBitmapInfo, DIB_RGB_COLORS)) {
            PRUint32 maskSize = lpBitmapInfo->bmiHeader.biSizeImage;
            lpBitmapInfo->bmiHeader.biBitCount = 32;
            lpBitmapInfo->bmiHeader.biSizeImage = colorSize;
            lpBitmapInfo->bmiHeader.biClrUsed = 0;
            lpBitmapInfo->bmiHeader.biClrImportant = 0;
            if (GetDIBits(hDC, iconInfo.hbmColor, 0, maskInfo.bmiHeader.biHeight, buffer + sizeof(ICONFILEHEADER) + sizeof(ICONENTRY) + sizeof(BITMAPINFOHEADER), lpBitmapInfo, DIB_RGB_COLORS)) {
              
              lpBitmapInfo->bmiHeader.biHeight *= 2;
              lpBitmapInfo->bmiHeader.biSizeImage += maskSize;

              
              nsCOMPtr<nsIInputStream> inStream;
              nsCOMPtr<nsIOutputStream> outStream;
              rv = NS_NewPipe(getter_AddRefs(inStream), getter_AddRefs(outStream),
                              iconSize, iconSize, nonBlocking);
              if (NS_SUCCEEDED(rv)) {
                PRUint32 written;
                rv = outStream->Write(buffer, iconSize, &written);
                if (NS_SUCCEEDED(rv)) {
                  NS_ADDREF(*_retval = inStream);
                }
              }

            } 
          } 
          delete [] buffer;
        } 
      } 

      DeleteDC(hDC);
      DeleteObject(iconInfo.hbmColor);
      DeleteObject(iconInfo.hbmMask);
    } 
    DestroyIcon(sfi.hIcon);
  } 

  return rv;
}

NS_IMETHODIMP nsIconChannel::GetContentType(nsACString &aContentType) 
{
  aContentType.AssignLiteral("image/x-icon");
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
