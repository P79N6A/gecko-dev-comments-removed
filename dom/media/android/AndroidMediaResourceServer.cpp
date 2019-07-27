




#include "mozilla/Assertions.h"
#include "mozilla/Base64.h"
#include "mozilla/IntegerPrintfMacros.h"
#include "nsThreadUtils.h"
#include "nsIServiceManager.h"
#include "nsISocketTransport.h"
#include "nsIOutputStream.h"
#include "nsIInputStream.h"
#include "nsIRandomGenerator.h"
#include "nsReadLine.h"
#include "nsNetCID.h"
#include "VideoUtils.h"
#include "MediaResource.h"
#include "AndroidMediaResourceServer.h"

#if defined(_MSC_VER)
#define strtoll _strtoi64
#if _MSC_VER < 1900
#define snprintf _snprintf_s
#endif
#endif

using namespace mozilla;





template<typename CharT, class StreamType, class StringType>
nsresult
ReadCRLF (StreamType* aStream, nsLineBuffer<CharT> * aBuffer,
          StringType & aLine, bool *aMore)
{
  
  
  bool eollast = false;

  aLine.Truncate();

  while (1) { 
    if (aBuffer->start == aBuffer->end) { 
      uint32_t bytesRead;
      nsresult rv = aStream->Read(aBuffer->buf, kLineBufferSize, &bytesRead);
      if (NS_FAILED(rv) || bytesRead == 0) {
        *aMore = false;
        return rv;
      }
      aBuffer->start = aBuffer->buf;
      aBuffer->end = aBuffer->buf + bytesRead;
      *(aBuffer->end) = '\0';
    }

    










    CharT* current = aBuffer->start;
    if (eollast) { 
      if (*current == '\n') {
        aBuffer->start = ++current;
        *aMore = true;
        return NS_OK;
      }
      else {
        eollast = false;
        aLine.Append('\r');
      }
    }
    
    for ( ; current < aBuffer->end-1; ++current) {
      if (*current == '\r' && *(current+1) == '\n') {
        *current++ = '\0';
        *current++ = '\0';
        aLine.Append(aBuffer->start);
        aBuffer->start = current;
        *aMore = true;
        return NS_OK;
      }
    }
    
    if (*current == '\r') {
      eollast = true;
      *current++ = '\0';
    }

    aLine.Append(aBuffer->start);
    aBuffer->start = aBuffer->end; 
  }
}






class ServeResourceEvent : public nsRunnable {
private:
  
  nsCOMPtr<nsIInputStream> mInput;

  
  nsCOMPtr<nsIOutputStream> mOutput;

  
  
  nsRefPtr<AndroidMediaResourceServer> mServer;

  
  
  
  
  nsresult WriteAll(char const* aBuffer, int32_t aBufferLength);

public:
  ServeResourceEvent(nsIInputStream* aInput, nsIOutputStream* aOutput,
                     AndroidMediaResourceServer* aServer)
    : mInput(aInput), mOutput(aOutput), mServer(aServer) {}

  
  
  NS_IMETHOD Run();

  
  
  already_AddRefed<MediaResource> GetMediaResource(nsCString const& aHTTPRequest);

  
  void Shutdown();
};

nsresult
ServeResourceEvent::WriteAll(char const* aBuffer, int32_t aBufferLength)
{
  while (aBufferLength > 0) {
    uint32_t written = 0;
    nsresult rv = mOutput->Write(aBuffer, aBufferLength, &written);
    if (NS_FAILED (rv)) return rv;

    aBufferLength -= written;
    aBuffer += written;
  }

  return NS_OK;
}

already_AddRefed<MediaResource>
ServeResourceEvent::GetMediaResource(nsCString const& aHTTPRequest)
{
  
  const char* HTTP_METHOD = "GET ";
  if (strncmp(aHTTPRequest.get(), HTTP_METHOD, strlen(HTTP_METHOD)) != 0) {
    return nullptr;
  }

  const char* url_start = strchr(aHTTPRequest.get(), ' ');
  if (!url_start) {
    return nullptr;
  }

  const char* url_end = strrchr(++url_start, ' ');
  if (!url_end) {
    return nullptr;
  }

  
  
  
  
  nsCString relative(url_start, url_end - url_start);
  nsRefPtr<MediaResource> resource =
    mServer->GetResource(mServer->GetURLPrefix() + relative);
  return resource.forget();
}

NS_IMETHODIMP
ServeResourceEvent::Run() {
  bool more = false; 
  nsCString line;    
  nsLineBuffer<char>* buffer = new nsLineBuffer<char>();
  nsresult rv = ReadCRLF(mInput.get(), buffer, line, &more);
  if (NS_FAILED(rv)) { Shutdown(); return rv; }

  
  
  nsRefPtr<MediaResource> resource = GetMediaResource(line);
  if (!resource) {
    const char* response_404 = "HTTP/1.1 404 Not Found\r\n"
                               "Content-Length: 0\r\n\r\n";
    rv = WriteAll(response_404, strlen(response_404));
    Shutdown();
    return rv;
  }

  
  
  
  int64_t start = 0;

  
  
  
  while (more && line.Length() > 0) {
    rv = ReadCRLF(mInput.get(), buffer, line, &more);
    if (NS_FAILED(rv)) { Shutdown(); return rv; }

    
    
    
    
    
    
    
    
    
    
    NS_NAMED_LITERAL_CSTRING(byteRange, "Range: bytes=");
    const char* s = strstr(line.get(), byteRange.get());
    if (s) {
      start = strtoll(s+byteRange.Length(), nullptr, 10);

      
      start = std::max(int64_t(0), std::min(resource->GetLength(), start));
    }
  }

  
  const char* response_normal = "HTTP/1.1 200 OK\r\n";

  
  const char* response_range = "HTTP/1.1 206 Partial Content\r\n";

  
  const char* response_end = "\r\n";

  
  
  
  
  if (start > 0 && !resource->IsTransportSeekable()) {
    start = 0;
  }

  const char* response_line = start > 0 ?
                                response_range :
                                response_normal;
  rv = WriteAll(response_line, strlen(response_line));
  if (NS_FAILED(rv)) { Shutdown(); return NS_OK; }

  
  
  
  
  const int buffer_size = 32768;
  nsAutoArrayPtr<char> b(new char[buffer_size]);

  
  int64_t contentlength = resource->GetLength() - start;
  if (contentlength > 0) {
    static_assert (buffer_size > 1024,
                   "buffer_size must be large enough "
                   "to hold response headers");
    snprintf(b, buffer_size, "Content-Length: %" PRId64 "\r\n", contentlength);
    rv = WriteAll(b, strlen(b));
    if (NS_FAILED(rv)) { Shutdown(); return NS_OK; }
  }

  
  
  if (start > 0) {
    static_assert (buffer_size > 1024,
                   "buffer_size must be large enough "
                   "to hold response headers");
    snprintf(b, buffer_size, "Content-Range: "
             "bytes %" PRId64 "-%" PRId64 "/%" PRId64 "\r\n",
             start, resource->GetLength() - 1, resource->GetLength());
    rv = WriteAll(b, strlen(b));
    if (NS_FAILED(rv)) { Shutdown(); return NS_OK; }
  }

  rv = WriteAll(response_end, strlen(response_end));
  if (NS_FAILED(rv)) { Shutdown(); return NS_OK; }

  rv = mOutput->Flush();
  if (NS_FAILED(rv)) { Shutdown(); return NS_OK; }

  
  uint32_t bytesRead = 0; 
  rv = resource->ReadAt(start, b, buffer_size, &bytesRead);
  while (NS_SUCCEEDED(rv) && bytesRead != 0) {
    
    
    
    
    start += bytesRead;

    
    rv = WriteAll(b, bytesRead);
    if (NS_FAILED (rv)) break;

    rv = resource->ReadAt(start, b, 32768, &bytesRead);
  }

  Shutdown();
  return NS_OK;
}

void
ServeResourceEvent::Shutdown()
{
  
  mInput->Close();
  mOutput->Close();

  
  
  nsCOMPtr<nsIRunnable> event = new ShutdownThreadEvent(NS_GetCurrentThread());
  NS_DispatchToMainThread(event);
}













class ResourceSocketListener : public nsIServerSocketListener
{
public:
  
  
  nsRefPtr<AndroidMediaResourceServer> mServer;

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSISERVERSOCKETLISTENER

  ResourceSocketListener(AndroidMediaResourceServer* aServer) :
    mServer(aServer)
  {
  }

private:
  virtual ~ResourceSocketListener() { }
};

NS_IMPL_ISUPPORTS(ResourceSocketListener, nsIServerSocketListener)

NS_IMETHODIMP
ResourceSocketListener::OnSocketAccepted(nsIServerSocket* aServ,
                                         nsISocketTransport* aTrans)
{
  nsCOMPtr<nsIInputStream> input;
  nsCOMPtr<nsIOutputStream> output;
  nsresult rv;

  rv = aTrans->OpenInputStream(nsITransport::OPEN_BLOCKING, 0, 0, getter_AddRefs(input));
  if (NS_FAILED(rv)) return rv;

  rv = aTrans->OpenOutputStream(nsITransport::OPEN_BLOCKING, 0, 0, getter_AddRefs(output));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIThread> thread;
  rv = NS_NewThread(getter_AddRefs(thread));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIRunnable> event = new ServeResourceEvent(input.get(), output.get(), mServer);
  return thread->Dispatch(event, NS_DISPATCH_NORMAL);
}

NS_IMETHODIMP
ResourceSocketListener::OnStopListening(nsIServerSocket* aServ, nsresult aStatus)
{
  return NS_OK;
}

AndroidMediaResourceServer::AndroidMediaResourceServer() :
  mMutex("AndroidMediaResourceServer")
{
}

NS_IMETHODIMP
AndroidMediaResourceServer::Run()
{
  MOZ_DIAGNOSTIC_ASSERT(NS_IsMainThread());
  MutexAutoLock lock(mMutex);

  nsresult rv;
  mSocket = do_CreateInstance(NS_SERVERSOCKET_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;

  rv = mSocket->InitSpecialConnection(-1,
                                      nsIServerSocket::LoopbackOnly
                                      | nsIServerSocket::KeepWhenOffline,
                                      -1);
  if (NS_FAILED(rv)) return rv;

  rv = mSocket->AsyncListen(new ResourceSocketListener(this));
  if (NS_FAILED(rv)) return rv;

  return NS_OK;
}


already_AddRefed<AndroidMediaResourceServer>
AndroidMediaResourceServer::Start()
{
  MOZ_ASSERT(NS_IsMainThread());
  nsRefPtr<AndroidMediaResourceServer> server = new AndroidMediaResourceServer();
  server->Run();
  return server.forget();
}

void
AndroidMediaResourceServer::Stop()
{
  MutexAutoLock lock(mMutex);
  mSocket->Close();
  mSocket = nullptr;
}

nsresult
AndroidMediaResourceServer::AppendRandomPath(nsCString& aUrl)
{
  
  
  
  nsresult rv;
  nsAutoCString salt;
  rv = GenerateRandomPathName(salt, 16);
  if (NS_FAILED(rv)) return rv;
  aUrl += "/";
  aUrl += salt;
  return NS_OK;
}

nsresult
AndroidMediaResourceServer::AddResource(mozilla::MediaResource* aResource, nsCString& aUrl)
{
  nsCString url = GetURLPrefix();
  nsresult rv = AppendRandomPath(url);
  if (NS_FAILED (rv)) return rv;

  {
    MutexAutoLock lock(mMutex);

    
    if (mResources.find(aUrl) != mResources.end()) return NS_ERROR_FAILURE;
    mResources[url] = aResource;
  }

  aUrl = url;

  return NS_OK;
}

void
AndroidMediaResourceServer::RemoveResource(nsCString const& aUrl)
{
  MutexAutoLock lock(mMutex);
  mResources.erase(aUrl);
}

nsCString
AndroidMediaResourceServer::GetURLPrefix()
{
  MutexAutoLock lock(mMutex);

  int32_t port = 0;
  nsresult rv = mSocket->GetPort(&port);
  if (NS_FAILED (rv) || port < 0) {
    return nsCString("");
  }

  char buffer[256];
  snprintf(buffer, sizeof(buffer), "http://127.0.0.1:%d", port >= 0 ? port : 0);
  return nsCString(buffer);
}

already_AddRefed<MediaResource>
AndroidMediaResourceServer::GetResource(nsCString const& aUrl)
{
  MutexAutoLock lock(mMutex);
  ResourceMap::const_iterator it = mResources.find(aUrl);
  if (it == mResources.end()) return nullptr;

  nsRefPtr<MediaResource> resource = it->second;
  return resource.forget();
}
