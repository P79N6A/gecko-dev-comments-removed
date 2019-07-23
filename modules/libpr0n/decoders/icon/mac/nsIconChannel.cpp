









































#include "nsIconChannel.h"
#include "nsIIconURI.h"
#include "nsIServiceManager.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsXPIDLString.h"
#include "nsMimeTypes.h"
#include "nsMemory.h"
#include "nsIStringStream.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsIMIMEService.h"
#include "nsCExternalHandlerService.h"
#include "plstr.h"
#include "nsILocalFileMac.h"
#include "nsIFileURL.h"
#include "nsInt64.h"

#include <Icons.h>
#include <Files.h>
#include <Folders.h>
#include <Icons.h>
#include <Quickdraw.h>
#include <MacMemory.h>


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

  nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(fileURI, &rv);
  if (NS_FAILED(rv) || !fileURL) return NS_OK;

  nsCOMPtr<nsIFile> file;
  rv = fileURL->GetFile(getter_AddRefs(file));
  if (NS_FAILED(rv) || !file) return NS_OK;
  
  nsCOMPtr<nsILocalFileMac> localFileMac (do_QueryInterface(file, &rv));
  if (NS_FAILED(rv) || !localFileMac) return NS_OK;
  
  *aLocalFile = file;
  NS_IF_ADDREF(*aLocalFile);

  return NS_OK;
}

nsresult
GetLockedIconData(IconFamilyHandle iconFamilyH, PRUint32 iconType,
                           Handle iconDataH, PRUint32 *iconDataSize)
{
  *iconDataSize = 0;

  if (::GetIconFamilyData(iconFamilyH, iconType, iconDataH) != noErr)
    return(NS_ERROR_FAILURE);

  *iconDataSize = (PRUint32)::GetHandleSize(iconDataH);
  if (*iconDataSize > 0)
  {
    ::HLock(iconDataH);
  }
  return(NS_OK);
}


nsresult
GetLockedIcons(IconFamilyHandle icnFamily, PRUint32 desiredImageSize,
            Handle iconH, PRUint32 *dataCount, PRBool *isIndexedData,
            Handle iconMaskH, PRUint32 *maskCount, PRUint8 *alphaBitCount)
{
  
  
  
  
  
  
  
  

  *dataCount = *maskCount = 0L;

  
  
  HUnlock(iconH);
  HUnlock(iconMaskH);

  
  
  nsresult rv = GetLockedIconData(icnFamily, (desiredImageSize > 16) ?
                                  kLarge32BitData : kSmall32BitData,
                                  iconH, dataCount);
  if (NS_SUCCEEDED(rv) && (*dataCount > 0))
  {
    *isIndexedData = PR_FALSE;
  }
  else
  {
    
    
    rv = GetLockedIconData(icnFamily, (desiredImageSize > 16) ?
                           kLarge8BitData : kSmall8BitData,
                           iconH, dataCount);
    if (NS_SUCCEEDED(rv) && (*dataCount > 0))
    {
      *isIndexedData = PR_TRUE;
    }
  }

  
  if (NS_SUCCEEDED(rv) && (*dataCount > 0))
  {
    
    *alphaBitCount = 8;
    rv = GetLockedIconData(icnFamily, (desiredImageSize > 16) ?
                           kLarge8BitMask : kSmall8BitMask,
                           iconMaskH, maskCount);
    if (NS_FAILED(rv) || (*maskCount == 0)) {
      
      *alphaBitCount = 1;
      rv = GetLockedIconData(icnFamily, (desiredImageSize > 16) ?
                             kLarge1BitMask : kSmall1BitMask,
                             iconMaskH, maskCount);
      if (NS_FAILED(rv) || (*maskCount == 0))
      {
        
        
        
        
        *maskCount = (desiredImageSize > 16) ? 256 : 64;
        rv = NS_OK;
      }
    }
  }
  return rv;
}


NS_IMETHODIMP nsIconChannel::AsyncOpen(nsIStreamListener *aListener, nsISupports *ctxt)
{
  nsCOMPtr<nsIInputStream> inStream;
  nsresult rv = MakeInputStream(getter_AddRefs(inStream), PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mPump->Init(inStream, nsInt64(-1), nsInt64(-1), 0, 0, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = mPump->AsyncRead(this, ctxt);
  if (NS_SUCCEEDED(rv)) 
  {
    
    mListener = aListener;
    
    if (mLoadGroup)
      mLoadGroup->AddRequest(this, nsnull);
  }

  return rv;
}

nsresult nsIconChannel::MakeInputStream(nsIInputStream** _retval, PRBool nonBlocking)
  {
  nsXPIDLCString contentType;
  nsCAutoString fileExt;
  nsCOMPtr<nsIFile> fileloc; 
  PRUint32 desiredImageSize;
  nsresult rv = ExtractIconInfoFromUrl(getter_AddRefs(fileloc), &desiredImageSize, contentType, fileExt);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsILocalFile>  localFile = do_QueryInterface(fileloc);
  if (localFile)
    localFile->SetFollowLinks(PR_FALSE);

  PRBool fileExists = PR_FALSE;
  if (fileloc)
    localFile->Exists(&fileExists);

  IconRef icnRef = nsnull;
  if (fileExists)
  {
    
    nsCOMPtr<nsILocalFileMac> localFileMac (do_QueryInterface(fileloc, &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    FSSpec spec;
    if (NS_FAILED(localFileMac->GetFSSpec(&spec)))
      return NS_ERROR_FAILURE;

    SInt16  label;
    if (::GetIconRefFromFile (&spec, &icnRef, &label) != noErr)
      return NS_ERROR_FAILURE;
  }

  
  

  
  PRUint32 dataCount = 0L;
  PRUint32 maskCount = 0L;
  PRUint8 alphaBitsCount = 1;
  Handle iconH = ::NewHandle(dataCount);
  Handle iconMaskH = ::NewHandle(maskCount);

  PRUint8 *iconBitmapData = nsnull, *maskBitmapData = nsnull;

  PRBool  isIndexedData = PR_FALSE;
  IconFamilyHandle icnFamily;
  if (fileExists && (::IconRefToIconFamily(icnRef,
      kSelectorAllAvailableData, &icnFamily) == noErr))
  {
    GetLockedIcons(icnFamily, desiredImageSize, iconH,  &dataCount,
                   &isIndexedData, iconMaskH, &maskCount, &alphaBitsCount);
    if (dataCount > 0)
    {
      iconBitmapData = (PRUint8 *)*iconH;
      if (maskCount > 0)  maskBitmapData = (PRUint8 *)*iconMaskH;
    }
    ::DisposeHandle((Handle)icnFamily);
  }
  ::ReleaseIconRef(icnRef);
  icnRef = nsnull;

  if (!dataCount)
  {
    
    

    nsCOMPtr<nsIMIMEService> mimeService (do_GetService(NS_MIMESERVICE_CONTRACTID, &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIMIMEInfo> mimeInfo;
    if (mimeService && (!contentType.IsEmpty() || !fileExt.IsEmpty()))
    {
      mimeService->GetFromTypeAndExtension(contentType,
                                           fileExt,
                                           getter_AddRefs(mimeInfo));
    }

    
    if (!mimeInfo)
    {
      if (iconH)      ::DisposeHandle(iconH);
      if (iconMaskH)  ::DisposeHandle(iconMaskH);
      return NS_ERROR_FAILURE;
    }

    
    PRUint32 macType;
    PRUint32 macCreator;
    mimeInfo->GetMacType(&macType);
    mimeInfo->GetMacCreator(&macCreator);

    if (::GetIconRef(kOnSystemDisk, macCreator, macType, &icnRef) != noErr)
    {
      if (iconH)      ::DisposeHandle(iconH);
      if (iconMaskH)  ::DisposeHandle(iconMaskH);
      return(NS_ERROR_FAILURE);
    }

    if (::IconRefToIconFamily(icnRef, kSelectorAllAvailableData, &icnFamily) == noErr)
    {
      GetLockedIcons(icnFamily, desiredImageSize, iconH, &dataCount,
                     &isIndexedData, iconMaskH, &maskCount, &alphaBitsCount);
      if (dataCount > 0)
      {
        iconBitmapData = (PRUint8 *)*iconH;
        if (maskCount > 0)  maskBitmapData = (PRUint8 *)*iconMaskH;
      }
      ::DisposeHandle((Handle)icnFamily);
    }
    ::ReleaseIconRef(icnRef);
    icnRef = nsnull;
  }

  
  
  if (!iconBitmapData)
  {
    if (iconH)      DisposeHandle(iconH);
    if (iconMaskH)  DisposeHandle(iconMaskH);
    return(NS_ERROR_FAILURE);
  }

  PRUint32 numPixelsInRow = (desiredImageSize > 16) ? 32 : 16;
  PRUint8  *bitmapData = (PRUint8 *)iconBitmapData;

  nsCString iconBuffer;
  iconBuffer.Assign((char) numPixelsInRow);
  iconBuffer.Append((char) numPixelsInRow);
  iconBuffer.Append((char) alphaBitsCount); 

  CTabHandle cTabHandle = nsnull;
  CTabPtr colTable = nsnull;
  if (isIndexedData)
  {
    
    cTabHandle = ::GetCTable(72);
    if (!cTabHandle)
    {
      if (iconH)      ::DisposeHandle(iconH);
      if (iconMaskH)  ::DisposeHandle(iconMaskH);
      return NS_ERROR_FAILURE;
    }
    ::HLock((Handle) cTabHandle);
    colTable = *cTabHandle;
  }

  RGBColor rgbCol;
  PRUint8 redValue, greenValue, blueValue;
  PRUint32 index = 0L;
  while (index < dataCount)
  {
    if (!isIndexedData)
    {
      
      iconBuffer.Append((char) bitmapData[index++]);
      iconBuffer.Append((char) bitmapData[index++]);
      iconBuffer.Append((char) bitmapData[index++]);
      iconBuffer.Append((char) bitmapData[index++]);
    }
    else
    {
      
      
      ColorSpec colSpec =  colTable->ctTable[ bitmapData[index]];
      rgbCol = colSpec.rgb;

      redValue = rgbCol.red & 0xff;
      greenValue = rgbCol.green & 0xff;
      blueValue = rgbCol.blue & 0xff;

      iconBuffer.Append((char) 0);        
      iconBuffer.Append((char) redValue);
      iconBuffer.Append((char) greenValue);
      iconBuffer.Append((char) blueValue);
      index++;
    }
  }
  
  if (cTabHandle)
  {
    ::HUnlock((Handle) cTabHandle);
    ::DisposeCTable(cTabHandle);
  }

  ::DisposeHandle(iconH);
  iconH = nsnull;

  bitmapData = (PRUint8 *)maskBitmapData;
  if (maskBitmapData && alphaBitsCount == 1)
  {
    
    
    index = maskCount / 2;
    while (index < maskCount)
    {
      iconBuffer.Append((char) bitmapData[index]);
      iconBuffer.Append((char) bitmapData[index + 1]);

      if (numPixelsInRow == 32)
      {
        iconBuffer.Append((char) bitmapData[index + 2]);
        iconBuffer.Append((char) bitmapData[index + 3]);
        index += 4;
      }
      else
      {
        iconBuffer.Append((char) 255); 
        iconBuffer.Append((char) 255);
        index += 2;
      }
    }
  }
  else if (maskBitmapData && alphaBitsCount == 8) {
    index = 0;
    while (index < maskCount) {
      iconBuffer.Append((char)bitmapData[index]);
      index++;
    }
  }
  else
  {
    index = 0L;
    while (index++ < maskCount)
    {
      
      iconBuffer.Append((char) 255);
    }
  }

  if (iconMaskH)
  {
    ::DisposeHandle(iconMaskH);
    iconMaskH = nsnull;
  }

  
  nsCOMPtr<nsIInputStream> inStream;
  nsCOMPtr<nsIOutputStream> outStream;
  PRUint32 iconSize = iconBuffer.Length();
  rv = NS_NewPipe(getter_AddRefs(inStream), getter_AddRefs(outStream), iconSize, iconSize, nonBlocking);  

  if (NS_SUCCEEDED(rv))
  {
    PRUint32 written;
    rv = outStream->Write(iconBuffer.get(), iconSize, &written);
    if (NS_SUCCEEDED(rv))
      NS_IF_ADDREF(*_retval = inStream);
  }

  
  mCallbacks = nsnull;

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
  aContentCharset.AssignLiteral("image/icon");
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

