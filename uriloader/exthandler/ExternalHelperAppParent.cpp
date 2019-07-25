





































#include "ExternalHelperAppParent.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsCExternalHandlerService.h"
#include "nsIExternalHelperAppService.h"
#include "mozilla/dom/ContentParent.h"
#include "nsIBrowserDOMWindow.h"
#include "nsStringStream.h"

#include "mozilla/unused.h"
#include "mozilla/Util.h" 

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS_INHERITED4(ExternalHelperAppParent,
                             nsHashPropertyBag,
                             nsIRequest,
                             nsIChannel,
                             nsIMultiPartChannel,
                             nsIResumableChannel)

ExternalHelperAppParent::ExternalHelperAppParent(
    const IPC::URI& uri,
    const PRInt64& aContentLength)
  : mURI(uri)
  , mPending(PR_FALSE)
  , mLoadFlags(0)
  , mStatus(NS_OK)
  , mContentLength(aContentLength)
{
}

void
ExternalHelperAppParent::Init(ContentParent *parent,
                              const nsCString& aMimeContentType,
                              const nsCString& aContentDisposition,
                              const PRBool& aForceSave,
                              const IPC::URI& aReferrer)
{
  nsHashPropertyBag::Init();

  nsCOMPtr<nsIExternalHelperAppService> helperAppService =
    do_GetService(NS_EXTERNALHELPERAPPSERVICE_CONTRACTID);
  NS_ASSERTION(helperAppService, "No Helper App Service!");

  SetPropertyAsInt64(NS_CHANNEL_PROP_CONTENT_LENGTH, mContentLength);
  if (aReferrer)
    SetPropertyAsInterface(NS_LITERAL_STRING("docshell.internalReferrer"), aReferrer);
  SetContentDisposition(aContentDisposition);
  helperAppService->DoContent(aMimeContentType, this, nsnull,
                              aForceSave, getter_AddRefs(mListener));
}

bool
ExternalHelperAppParent::RecvOnStartRequest(const nsCString& entityID)
{
  mEntityID = entityID;
  mPending = PR_TRUE;
  mStatus = mListener->OnStartRequest(this, nsnull);
  return true;
}

bool
ExternalHelperAppParent::RecvOnDataAvailable(const nsCString& data,
                                             const PRUint32& offset,
                                             const PRUint32& count)
{
  if (NS_FAILED(mStatus))
    return true;

  NS_ASSERTION(mPending, "must be pending!");
  nsCOMPtr<nsIInputStream> stringStream;
  DebugOnly<nsresult> rv = NS_NewByteInputStream(getter_AddRefs(stringStream), data.get(), count, NS_ASSIGNMENT_DEPEND);
  NS_ASSERTION(NS_SUCCEEDED(rv), "failed to create dependent string!");
  mStatus = mListener->OnDataAvailable(this, nsnull, stringStream, offset, count);

  return true;
}

bool
ExternalHelperAppParent::RecvOnStopRequest(const nsresult& code)
{
  mPending = PR_FALSE;
  mListener->OnStopRequest(this, nsnull,
                           (NS_SUCCEEDED(code) && NS_FAILED(mStatus)) ? mStatus : code);
  unused << Send__delete__(this);
  return true;
}

ExternalHelperAppParent::~ExternalHelperAppParent()
{
}





NS_IMETHODIMP
ExternalHelperAppParent::GetName(nsACString& aResult)
{
  mURI->GetAsciiSpec(aResult);
  return NS_OK;
}

NS_IMETHODIMP
ExternalHelperAppParent::IsPending(PRBool *aResult)
{
  *aResult = mPending;
  return NS_OK;
}

NS_IMETHODIMP
ExternalHelperAppParent::GetStatus(nsresult *aResult)
{
  *aResult = mStatus;
  return NS_OK;
}

NS_IMETHODIMP
ExternalHelperAppParent::Cancel(nsresult aStatus)
{
  mStatus = aStatus;
  unused << SendCancel(aStatus);
  return NS_OK;
}

NS_IMETHODIMP
ExternalHelperAppParent::Suspend()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
ExternalHelperAppParent::Resume()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}





NS_IMETHODIMP
ExternalHelperAppParent::GetOriginalURI(nsIURI * *aURI)
{
  NS_IF_ADDREF(*aURI = mURI);
  return NS_OK;
}

NS_IMETHODIMP
ExternalHelperAppParent::SetOriginalURI(nsIURI *aURI)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
ExternalHelperAppParent::GetURI(nsIURI **aURI)
{
  NS_IF_ADDREF(*aURI = mURI);
  return NS_OK;
}

NS_IMETHODIMP
ExternalHelperAppParent::Open(nsIInputStream **aResult)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
ExternalHelperAppParent::AsyncOpen(nsIStreamListener *aListener,
                                   nsISupports *aContext)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
ExternalHelperAppParent::GetLoadFlags(nsLoadFlags *aLoadFlags)
{
  *aLoadFlags = mLoadFlags;
  return NS_OK;
}

NS_IMETHODIMP
ExternalHelperAppParent::SetLoadFlags(nsLoadFlags aLoadFlags)
{
  mLoadFlags = aLoadFlags;
  return NS_OK;
}

NS_IMETHODIMP
ExternalHelperAppParent::GetLoadGroup(nsILoadGroup* *aLoadGroup)
{
  *aLoadGroup = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
ExternalHelperAppParent::SetLoadGroup(nsILoadGroup* aLoadGroup)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
ExternalHelperAppParent::GetOwner(nsISupports* *aOwner)
{
  *aOwner = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
ExternalHelperAppParent::SetOwner(nsISupports* aOwner)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
ExternalHelperAppParent::GetNotificationCallbacks(nsIInterfaceRequestor* *aCallbacks)
{
  *aCallbacks = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
ExternalHelperAppParent::SetNotificationCallbacks(nsIInterfaceRequestor* aCallbacks)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
ExternalHelperAppParent::GetSecurityInfo(nsISupports * *aSecurityInfo)
{
  *aSecurityInfo = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
ExternalHelperAppParent::GetContentType(nsACString& aContentType)
{
  aContentType.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
ExternalHelperAppParent::SetContentType(const nsACString& aContentType)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
ExternalHelperAppParent::GetContentCharset(nsACString& aContentCharset)
{
  aContentCharset.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
ExternalHelperAppParent::SetContentCharset(const nsACString& aContentCharset)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
ExternalHelperAppParent::GetContentLength(PRInt32 *aContentLength)
{
  if (mContentLength > PR_INT32_MAX || mContentLength < 0)
    *aContentLength = -1;
  else
    *aContentLength = (PRInt32)mContentLength;
  return NS_OK;
}

NS_IMETHODIMP
ExternalHelperAppParent::SetContentLength(PRInt32 aContentLength)
{
  mContentLength = aContentLength;
  return NS_OK;
}





NS_IMETHODIMP
ExternalHelperAppParent::ResumeAt(PRUint64 startPos, const nsACString& entityID)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
ExternalHelperAppParent::GetEntityID(nsACString& aEntityID)
{
  aEntityID = mEntityID;
  return NS_OK;
}





NS_IMETHODIMP
ExternalHelperAppParent::GetBaseChannel(nsIChannel* *aChannel)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
ExternalHelperAppParent::GetContentDisposition(nsACString& aContentDisposition)
{
  aContentDisposition = mContentDisposition;
  return NS_OK;
}

NS_IMETHODIMP
ExternalHelperAppParent::SetContentDisposition(const nsACString& aDisposition)
{
  mContentDisposition = aDisposition;
  return NS_OK;
}

NS_IMETHODIMP
ExternalHelperAppParent::GetPartID(PRUint32* aPartID)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
ExternalHelperAppParent::GetIsLastPart(PRBool* aIsLastPart)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

} 
} 
