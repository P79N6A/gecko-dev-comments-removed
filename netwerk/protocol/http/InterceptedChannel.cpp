





#include "HttpLog.h"

#include "InterceptedChannel.h"
#include "nsInputStreamPump.h"
#include "nsIPipe.h"
#include "nsIStreamListener.h"
#include "nsHttpChannel.h"
#include "HttpChannelChild.h"
#include "nsHttpResponseHead.h"

namespace mozilla {
namespace net {

extern nsresult
DoAddCacheEntryHeaders(nsHttpChannel *self,
                       nsICacheEntry *entry,
                       nsHttpRequestHead *requestHead,
                       nsHttpResponseHead *responseHead,
                       nsISupports *securityInfo);

NS_IMPL_ISUPPORTS(InterceptedChannelBase, nsIInterceptedChannel)

InterceptedChannelBase::InterceptedChannelBase(nsINetworkInterceptController* aController,
                                               bool aIsNavigation)
: mController(aController)
, mIsNavigation(aIsNavigation)
{
}

InterceptedChannelBase::~InterceptedChannelBase()
{
}

NS_IMETHODIMP
InterceptedChannelBase::GetResponseBody(nsIOutputStream** aStream)
{
  NS_IF_ADDREF(*aStream = mResponseBody);
  return NS_OK;
}

void
InterceptedChannelBase::EnsureSynthesizedResponse()
{
  if (mSynthesizedResponseHead.isNothing()) {
    mSynthesizedResponseHead.emplace(new nsHttpResponseHead());
  }
}

void
InterceptedChannelBase::DoNotifyController()
{
    nsresult rv = mController->ChannelIntercepted(this);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      rv = ResetInterception();
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to resume intercepted network request");
    }
    mController = nullptr;
}

NS_IMETHODIMP
InterceptedChannelBase::GetIsNavigation(bool* aIsNavigation)
{
  *aIsNavigation = mIsNavigation;
  return NS_OK;
}

nsresult
InterceptedChannelBase::DoSynthesizeStatus(uint16_t aStatus, const nsACString& aReason)
{
    EnsureSynthesizedResponse();

    
    nsAutoCString statusLine;
    statusLine.AppendLiteral("HTTP/1.1 ");
    statusLine.AppendInt(aStatus);
    statusLine.AppendLiteral(" ");
    statusLine.Append(aReason);

    (*mSynthesizedResponseHead)->ParseStatusLine(statusLine.get());
    return NS_OK;
}

nsresult
InterceptedChannelBase::DoSynthesizeHeader(const nsACString& aName, const nsACString& aValue)
{
    EnsureSynthesizedResponse();

    nsAutoCString header = aName + NS_LITERAL_CSTRING(": ") + aValue;
    
    nsresult rv = (*mSynthesizedResponseHead)->ParseHeaderLine(header.get());
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
}

InterceptedChannelChrome::InterceptedChannelChrome(nsHttpChannel* aChannel,
                                                   nsINetworkInterceptController* aController,
                                                   nsICacheEntry* aEntry)
: InterceptedChannelBase(aController, aChannel->IsNavigation())
, mChannel(aChannel)
, mSynthesizedCacheEntry(aEntry)
{
  nsresult rv = mChannel->GetApplyConversion(&mOldApplyConversion);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    mOldApplyConversion = false;
  }
}

void
InterceptedChannelChrome::NotifyController()
{
  nsCOMPtr<nsIOutputStream> out;

  
  mChannel->SetApplyConversion(false);

  nsresult rv = mSynthesizedCacheEntry->OpenOutputStream(0, getter_AddRefs(mResponseBody));
  NS_ENSURE_SUCCESS_VOID(rv);

  DoNotifyController();
}

NS_IMETHODIMP
InterceptedChannelChrome::GetChannel(nsIChannel** aChannel)
{
  NS_IF_ADDREF(*aChannel = mChannel);
  return NS_OK;
}

NS_IMETHODIMP
InterceptedChannelChrome::ResetInterception()
{
  if (!mChannel) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  mSynthesizedCacheEntry->AsyncDoom(nullptr);
  mSynthesizedCacheEntry = nullptr;

  mChannel->SetApplyConversion(mOldApplyConversion);

  nsCOMPtr<nsIURI> uri;
  mChannel->GetURI(getter_AddRefs(uri));

  nsresult rv = mChannel->StartRedirectChannelToURI(uri, nsIChannelEventSink::REDIRECT_INTERNAL);
  NS_ENSURE_SUCCESS(rv, rv);

  mChannel = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
InterceptedChannelChrome::SynthesizeStatus(uint16_t aStatus, const nsACString& aReason)
{
  if (!mSynthesizedCacheEntry) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  return DoSynthesizeStatus(aStatus, aReason);
}

NS_IMETHODIMP
InterceptedChannelChrome::SynthesizeHeader(const nsACString& aName, const nsACString& aValue)
{
  if (!mSynthesizedCacheEntry) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  return DoSynthesizeHeader(aName, aValue);
}

NS_IMETHODIMP
InterceptedChannelChrome::FinishSynthesizedResponse()
{
  if (!mChannel) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  EnsureSynthesizedResponse();

  mChannel->MarkIntercepted();

  
  

  nsCOMPtr<nsISupports> securityInfo;
  nsresult rv = mChannel->GetSecurityInfo(getter_AddRefs(securityInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = DoAddCacheEntryHeaders(mChannel, mSynthesizedCacheEntry,
                              mChannel->GetRequestHead(),
                              mSynthesizedResponseHead.ref(), securityInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> uri;
  mChannel->GetURI(getter_AddRefs(uri));

  bool usingSSL = false;
  uri->SchemeIs("https", &usingSSL);

  
  rv = mChannel->OpenCacheEntry(usingSSL);
  NS_ENSURE_SUCCESS(rv, rv);

  mSynthesizedCacheEntry = nullptr;

  if (!mChannel->AwaitingCacheCallbacks()) {
    rv = mChannel->ContinueConnect();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mChannel = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
InterceptedChannelChrome::Cancel()
{
  if (!mChannel) {
    return NS_ERROR_FAILURE;
  }

  
  
  nsresult rv = mChannel->AsyncAbort(NS_BINDING_ABORTED);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

NS_IMETHODIMP
InterceptedChannelChrome::SetSecurityInfo(nsISupports* aSecurityInfo)
{
  if (!mChannel) {
    return NS_ERROR_FAILURE;
  }

  return mChannel->OverrideSecurityInfo(aSecurityInfo);
}

InterceptedChannelContent::InterceptedChannelContent(HttpChannelChild* aChannel,
                                                     nsINetworkInterceptController* aController,
                                                     nsIStreamListener* aListener)
: InterceptedChannelBase(aController, aChannel->IsNavigation())
, mChannel(aChannel)
, mStreamListener(aListener)
{
}

void
InterceptedChannelContent::NotifyController()
{
  nsresult rv = NS_NewPipe(getter_AddRefs(mSynthesizedInput),
                           getter_AddRefs(mResponseBody),
                           0, UINT32_MAX, true, true);
  NS_ENSURE_SUCCESS_VOID(rv);

  DoNotifyController();
}

NS_IMETHODIMP
InterceptedChannelContent::GetChannel(nsIChannel** aChannel)
{
  NS_IF_ADDREF(*aChannel = mChannel);
  return NS_OK;
}

NS_IMETHODIMP
InterceptedChannelContent::ResetInterception()
{
  if (!mChannel) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  mResponseBody = nullptr;
  mSynthesizedInput = nullptr;

  mChannel->ResetInterception();
  mChannel = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
InterceptedChannelContent::SynthesizeStatus(uint16_t aStatus, const nsACString& aReason)
{
  if (!mResponseBody) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  return DoSynthesizeStatus(aStatus, aReason);
}

NS_IMETHODIMP
InterceptedChannelContent::SynthesizeHeader(const nsACString& aName, const nsACString& aValue)
{
  if (!mResponseBody) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  return DoSynthesizeHeader(aName, aValue);
}

NS_IMETHODIMP
InterceptedChannelContent::FinishSynthesizedResponse()
{
  if (NS_WARN_IF(!mChannel)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  EnsureSynthesizedResponse();

  mChannel->OverrideWithSynthesizedResponse(mSynthesizedResponseHead.ref(),
                                            mSynthesizedInput,
                                            mStreamListener);

  mResponseBody = nullptr;
  mChannel = nullptr;
  mStreamListener = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
InterceptedChannelContent::Cancel()
{
  if (!mChannel) {
    return NS_ERROR_FAILURE;
  }

  
  
  nsresult rv = mChannel->AsyncAbort(NS_BINDING_ABORTED);
  NS_ENSURE_SUCCESS(rv, rv);
  mChannel = nullptr;
  mStreamListener = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
InterceptedChannelContent::SetSecurityInfo(nsISupports* aSecurityInfo)
{
  if (!mChannel) {
    return NS_ERROR_FAILURE;
  }

  return mChannel->OverrideSecurityInfo(aSecurityInfo);
}

} 
} 
