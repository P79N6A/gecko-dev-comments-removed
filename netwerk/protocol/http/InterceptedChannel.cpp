





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
    mController = nullptr;
    NS_ENSURE_SUCCESS_VOID(rv);
}

NS_IMETHODIMP
InterceptedChannelBase::GetIsNavigation(bool* aIsNavigation)
{
  *aIsNavigation = mIsNavigation;
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
}

void
InterceptedChannelChrome::NotifyController()
{
  nsCOMPtr<nsIOutputStream> out;

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

  nsCOMPtr<nsIURI> uri;
  mChannel->GetURI(getter_AddRefs(uri));

  nsresult rv = mChannel->StartRedirectChannelToURI(uri, nsIChannelEventSink::REDIRECT_INTERNAL);
  NS_ENSURE_SUCCESS(rv, rv);

  mChannel = nullptr;
  return NS_OK;
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

  nsresult rv = nsInputStreamPump::Create(getter_AddRefs(mStoragePump), mSynthesizedInput,
                                          int64_t(-1), int64_t(-1), 0, 0, true);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    mSynthesizedInput->Close();
    return rv;
  }

  mResponseBody = nullptr;

  rv = mStoragePump->AsyncRead(mStreamListener, nullptr);
  NS_ENSURE_SUCCESS(rv, rv);

  mChannel->OverrideWithSynthesizedResponse(mSynthesizedResponseHead.ref(), mStoragePump);

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

} 
} 
