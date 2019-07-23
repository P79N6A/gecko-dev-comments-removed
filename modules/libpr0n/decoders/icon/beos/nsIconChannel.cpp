










































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
#include "nsDirectoryServiceDefs.h"

#include <Mime.h>
#include <Bitmap.h>
#include <Screen.h>
#include <Node.h>
#include <NodeInfo.h>


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

  *aLocalFile = file;
  NS_IF_ADDREF(*aLocalFile);
  return NS_OK;
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

nsresult nsIconChannel::MakeInputStream(nsIInputStream** _retval, PRBool nonBlocking)
{
  nsXPIDLCString contentType;
  nsCAutoString filePath;
  nsCAutoString fileExtension;
  nsCOMPtr<nsIFile> localFile; 
  PRUint32 desiredImageSize;
  nsresult rv = ExtractIconInfoFromUrl(getter_AddRefs(localFile), &desiredImageSize, contentType, fileExtension);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 iconSize = 16;
  if (desiredImageSize > 16)
    iconSize = 32;

  PRUint32 alphaBytesPerRow = (iconSize / 8);
  if (iconSize % 32 != 0)
    alphaBytesPerRow = ((iconSize / 32) + 1) * 4;
    
  PRBool fileExists = PR_FALSE;
  if (localFile)
  {
    localFile->GetNativePath(filePath);
    localFile->Exists(&fileExists);
  }
  
  
  
  
  
  BBitmap nativeIcon(BRect(0, 0, iconSize - 1, iconSize - 1), B_CMAP8);
  if (!nativeIcon.IsValid())
    return NS_ERROR_OUT_OF_MEMORY;

  PRBool gotBitmap = PR_FALSE;
  if (fileExists)
  {
    BNode localNode(filePath.get());
    
    
    if (localNode.ReadAttr("BEOS:TYPE", B_STRING_TYPE, 0, NULL, 0) != B_OK)
    	update_mime_info(filePath.get(), 0, 1, 1);

    BNodeInfo localNodeInfo(&localNode);
    if (iconSize == 16)
    {
      if (localNodeInfo.GetTrackerIcon(&nativeIcon, B_MINI_ICON) == B_OK)
        gotBitmap = PR_TRUE;
    }
    else
    {
      if (localNodeInfo.GetTrackerIcon(&nativeIcon, B_LARGE_ICON) == B_OK)
        gotBitmap = PR_TRUE;
    }
  }

  
  if (!gotBitmap)    
  {
    
    if (contentType.IsEmpty())
    {
      nsCOMPtr<nsIMIMEService> mimeService (do_GetService("@mozilla.org/mime;1", &rv));
      if (NS_SUCCEEDED(rv))
        mimeService->GetTypeFromExtension(fileExtension, contentType);
      
      if (contentType.IsEmpty())
        contentType = "application/octet-stream";
    }
    
    BMimeType mimeType(contentType.get());
    if (!mimeType.IsInstalled())
    	mimeType.SetTo("application/octet-stream");
    if (iconSize == 16)
    {
      if (mimeType.GetIcon(&nativeIcon, B_MINI_ICON) == B_OK)
        gotBitmap = PR_TRUE;
    }
    else
    {
      if (mimeType.GetIcon(&nativeIcon, B_LARGE_ICON) == B_OK)
        gotBitmap = PR_TRUE;
    }
  }
  
  if (!gotBitmap)
    return NS_ERROR_NOT_AVAILABLE;

  BScreen mainScreen(B_MAIN_SCREEN_ID);
  if (!mainScreen.IsValid())
    return NS_ERROR_NOT_AVAILABLE;

  
  PRUint32 iconLength = 2 + iconSize * iconSize * 4;
  uint8 *buffer = new uint8[iconLength];
  if (!buffer)
    return NS_ERROR_OUT_OF_MEMORY;

  uint8* destByte = buffer;
  *(destByte++) = iconSize;
  *(destByte++) = iconSize;

  
  uint8* sourceByte = (uint8*)nativeIcon.Bits();
  for(PRUint32 iconRow = 0; iconRow < iconSize; iconRow++)
  {
    sourceByte = (uint8*)nativeIcon.Bits() + nativeIcon.BytesPerRow() * iconRow;
    for(PRUint32 iconCol = 0; iconCol < iconSize; iconCol++)
    {
      if (*sourceByte != B_TRANSPARENT_MAGIC_CMAP8)
      {
        rgb_color colorVal = mainScreen.ColorForIndex(*sourceByte);
#ifdef IS_LITTLE_ENDIAN
        *(destByte++) = colorVal.blue;
        *(destByte++) = colorVal.green;
        *(destByte++) = colorVal.red;
        *(destByte++) = uint8(255);
#else
        *(destByte++) = uint8(255);
        *(destByte++) = colorVal.red;
        *(destByte++) = colorVal.green;
        *(destByte++) = colorVal.blue;
#endif
      }
      else
      {
        *destByte++ = 0;
        *destByte++ = 0;
        *destByte++ = 0;
        *destByte++ = 0;
      }
      
      
      
      sourceByte++;
    }
  }

  NS_ASSERTION(buffer + iconLength == destByte, "size miscalculation");
  
  
  nsCOMPtr<nsIInputStream> inStream;
  nsCOMPtr<nsIOutputStream> outStream;
  rv = NS_NewPipe(getter_AddRefs(inStream), getter_AddRefs(outStream),
                  iconLength, iconLength, nonBlocking);
  if (NS_SUCCEEDED(rv))
  {
    PRUint32 written;
    rv = outStream->Write((char*)buffer, iconLength, &written);
    if (NS_SUCCEEDED(rv))
      NS_ADDREF(*_retval = inStream);
  }
  delete [] buffer;
  
  return rv;
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
