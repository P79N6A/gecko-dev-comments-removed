














#include "nsAnnoProtocolHandler.h"
#include "nsFaviconService.h"
#include "nsIChannel.h"
#include "nsIInputStreamChannel.h"
#include "nsILoadGroup.h"
#include "nsIStandardURL.h"
#include "nsIStringStream.h"
#include "nsISupportsUtils.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsContentUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsStringStream.h"
#include "mozilla/storage.h"
#include "nsIPipe.h"
#include "Helpers.h"

using namespace mozilla;
using namespace mozilla::places;







static
nsresult
GetDefaultIcon(nsIChannel **aChannel)
{
  nsCOMPtr<nsIURI> defaultIconURI;
  nsresult rv = NS_NewURI(getter_AddRefs(defaultIconURI),
                          NS_LITERAL_CSTRING(FAVICON_DEFAULT_URL));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_NewChannel(aChannel,
                       defaultIconURI,
                       nsContentUtils::GetSystemPrincipal(),
                       nsILoadInfo::SEC_NORMAL,
                       nsIContentPolicy::TYPE_IMAGE);
}




namespace {












class faviconAsyncLoader : public AsyncStatementCallback
                         , public nsIRequestObserver
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  faviconAsyncLoader(nsIChannel *aChannel, nsIOutputStream *aOutputStream) :
      mChannel(aChannel)
    , mOutputStream(aOutputStream)
    , mReturnDefaultIcon(true)
  {
    NS_ASSERTION(aChannel,
                 "Not providing a channel will result in crashes!");
    NS_ASSERTION(aOutputStream,
                 "Not providing an output stream will result in crashes!");
  }

  
  

  NS_IMETHOD HandleResult(mozIStorageResultSet *aResultSet) override
  {
    
    nsCOMPtr<mozIStorageRow> row;
    nsresult rv = aResultSet->GetNextRow(getter_AddRefs(row));
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    nsAutoCString mimeType;
    (void)row->GetUTF8String(1, mimeType);
    NS_ENSURE_FALSE(mimeType.IsEmpty(), NS_OK);

    
    rv = mChannel->SetContentType(mimeType);
    NS_ENSURE_SUCCESS(rv, rv);

    
    uint8_t *favicon;
    uint32_t size = 0;
    rv = row->GetBlob(0, &size, &favicon);
    NS_ENSURE_SUCCESS(rv, rv);

    uint32_t totalWritten = 0;
    do {
      uint32_t bytesWritten;
      rv = mOutputStream->Write(
        &(reinterpret_cast<const char *>(favicon)[totalWritten]),
        size - totalWritten,
        &bytesWritten
      );
      if (NS_FAILED(rv) || !bytesWritten)
        break;
      totalWritten += bytesWritten;
    } while (size != totalWritten);
    NS_ASSERTION(NS_FAILED(rv) || size == totalWritten,
                 "Failed to write all of our data out to the stream!");

    
    free(favicon);

    
    
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    mReturnDefaultIcon = false;
    return NS_OK;
  }

  NS_IMETHOD HandleCompletion(uint16_t aReason) override
  {
    if (!mReturnDefaultIcon)
      return mOutputStream->Close();

    
    
    
    nsCOMPtr<nsIStreamListener> listener;
    nsresult rv = NS_NewSimpleStreamListener(getter_AddRefs(listener),
                                             mOutputStream, this);
    NS_ENSURE_SUCCESS(rv, mOutputStream->Close());

    nsCOMPtr<nsIChannel> newChannel;
    rv = GetDefaultIcon(getter_AddRefs(newChannel));
    NS_ENSURE_SUCCESS(rv, mOutputStream->Close());

    rv = newChannel->AsyncOpen(listener, nullptr);
    NS_ENSURE_SUCCESS(rv, mOutputStream->Close());

    return NS_OK;
  }

  
  

  NS_IMETHOD OnStartRequest(nsIRequest *, nsISupports *) override
  {
    return NS_OK;
  }

  NS_IMETHOD OnStopRequest(nsIRequest *, nsISupports *, nsresult aStatusCode) override
  {
    
    (void)mOutputStream->Close();

    
    NS_WARN_IF_FALSE(NS_SUCCEEDED(aStatusCode),
                     "Got an error when trying to load our default favicon!");

    return NS_OK;
  }

protected:
  virtual ~faviconAsyncLoader() {}

private:
  nsCOMPtr<nsIChannel> mChannel;
  nsCOMPtr<nsIOutputStream> mOutputStream;
  bool mReturnDefaultIcon;
};

NS_IMPL_ISUPPORTS_INHERITED(
  faviconAsyncLoader,
  AsyncStatementCallback,
  nsIRequestObserver
)

} 




NS_IMPL_ISUPPORTS(nsAnnoProtocolHandler, nsIProtocolHandler)



NS_IMETHODIMP
nsAnnoProtocolHandler::GetScheme(nsACString& aScheme)
{
  aScheme.AssignLiteral("moz-anno");
  return NS_OK;
}






NS_IMETHODIMP
nsAnnoProtocolHandler::GetDefaultPort(int32_t *aDefaultPort)
{
  *aDefaultPort = -1;
  return NS_OK;
}




NS_IMETHODIMP
nsAnnoProtocolHandler::GetProtocolFlags(uint32_t *aProtocolFlags)
{
  *aProtocolFlags = (URI_NORELATIVE | URI_NOAUTH | URI_DANGEROUS_TO_LOAD |
                     URI_IS_LOCAL_RESOURCE);
  return NS_OK;
}




NS_IMETHODIMP
nsAnnoProtocolHandler::NewURI(const nsACString& aSpec,
                              const char *aOriginCharset,
                              nsIURI *aBaseURI, nsIURI **_retval)
{
  nsCOMPtr <nsIURI> uri = do_CreateInstance(NS_SIMPLEURI_CONTRACTID);
  if (!uri)
    return NS_ERROR_OUT_OF_MEMORY;
  nsresult rv = uri->SetSpec(aSpec);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = nullptr;
  uri.swap(*_retval);
  return NS_OK;
}





NS_IMETHODIMP
nsAnnoProtocolHandler::NewChannel2(nsIURI* aURI,
                                   nsILoadInfo* aLoadInfo,
                                   nsIChannel** _retval)
{
  NS_ENSURE_ARG_POINTER(aURI);

  
  nsCOMPtr<nsIURI> annoURI;
  nsAutoCString annoName;
  nsresult rv = ParseAnnoURI(aURI, getter_AddRefs(annoURI), annoName);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (!annoName.EqualsLiteral(FAVICON_ANNOTATION_NAME))
    return NS_ERROR_INVALID_ARG;

  return NewFaviconChannel(aURI, annoURI, aLoadInfo, _retval);
}

NS_IMETHODIMP
nsAnnoProtocolHandler::NewChannel(nsIURI *aURI, nsIChannel **_retval)
{
  return NewChannel2(aURI, nullptr, _retval);
}






NS_IMETHODIMP
nsAnnoProtocolHandler::AllowPort(int32_t port, const char *scheme,
                                 bool *_retval)
{
  *_retval = false;
  return NS_OK;
}






nsresult
nsAnnoProtocolHandler::ParseAnnoURI(nsIURI* aURI,
                                    nsIURI** aResultURI, nsCString& aName)
{
  nsresult rv;
  nsAutoCString path;
  rv = aURI->GetPath(path);
  NS_ENSURE_SUCCESS(rv, rv);

  int32_t firstColon = path.FindChar(':');
  if (firstColon <= 0)
    return NS_ERROR_MALFORMED_URI;

  rv = NS_NewURI(aResultURI, Substring(path, firstColon + 1));
  NS_ENSURE_SUCCESS(rv, rv);

  aName = Substring(path, 0, firstColon);
  return NS_OK;
}

nsresult
nsAnnoProtocolHandler::NewFaviconChannel(nsIURI *aURI, nsIURI *aAnnotationURI,
                                         nsILoadInfo* aLoadInfo, nsIChannel **_channel)
{
  
  
  nsCOMPtr<nsIInputStream> inputStream;
  nsCOMPtr<nsIOutputStream> outputStream;
  nsresult rv = NS_NewPipe(getter_AddRefs(inputStream),
                           getter_AddRefs(outputStream),
                           MAX_FAVICON_SIZE, MAX_FAVICON_SIZE, true,
                           true);
  NS_ENSURE_SUCCESS(rv, GetDefaultIcon(_channel));

  
  
  nsCOMPtr<nsIChannel> channel;
  rv = NS_NewInputStreamChannelInternal(getter_AddRefs(channel),
                                        aURI,
                                        inputStream,
                                        EmptyCString(), 
                                        EmptyCString(), 
                                        aLoadInfo);
  NS_ENSURE_SUCCESS(rv, GetDefaultIcon(_channel));

  
  nsCOMPtr<mozIStorageStatementCallback> callback =
    new faviconAsyncLoader(channel, outputStream);
  NS_ENSURE_TRUE(callback, GetDefaultIcon(_channel));
  nsFaviconService* faviconService = nsFaviconService::GetFaviconService();
  NS_ENSURE_TRUE(faviconService, GetDefaultIcon(_channel));

  rv = faviconService->GetFaviconDataAsync(aAnnotationURI, callback);
  NS_ENSURE_SUCCESS(rv, GetDefaultIcon(_channel));

  channel.forget(_channel);
  return NS_OK;
}
