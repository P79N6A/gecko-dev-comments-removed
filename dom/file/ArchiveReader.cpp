





#include "ArchiveReader.h"
#include "ArchiveRequest.h"
#include "ArchiveEvent.h"
#include "ArchiveZipEvent.h"

#include "nsContentUtils.h"
#include "nsLayoutStatics.h"

#include "nsIURI.h"
#include "nsNetUtil.h"

#include "mozilla/dom/ArchiveReaderBinding.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/Preferences.h"

using namespace mozilla;
using namespace mozilla::dom;
USING_FILE_NAMESPACE

 already_AddRefed<ArchiveReader>
ArchiveReader::Constructor(const GlobalObject& aGlobal, nsIDOMBlob* aBlob,
                           const ArchiveReaderOptions& aOptions,
                           ErrorResult& aError)
{
  MOZ_ASSERT(aBlob);
  MOZ_ASSERT(PrefEnabled());

  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aGlobal.Get());
  if (!window) {
    aError.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  nsRefPtr<ArchiveReader> reader =
    new ArchiveReader(aBlob, window, aOptions.mEncoding);
  return reader.forget();
}

ArchiveReader::ArchiveReader(nsIDOMBlob* aBlob, nsPIDOMWindow* aWindow,
                             const nsString& aEncoding)
  : mBlob(aBlob)
  , mWindow(aWindow)
  , mStatus(NOT_STARTED)
  , mEncoding(aEncoding)
{
  MOZ_ASSERT(aBlob);
  MOZ_ASSERT(aWindow);

  nsLayoutStatics::AddRef();
  SetIsDOMBinding();
}

ArchiveReader::~ArchiveReader()
{
  nsLayoutStatics::Release();
}

 JSObject*
ArchiveReader::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return ArchiveReaderBinding::Wrap(aCx, aScope, this);
}

 bool
ArchiveReader::PrefEnabled()
{
  return Preferences::GetBool("dom.archivereader.enabled", true);
}

nsresult
ArchiveReader::RegisterRequest(ArchiveRequest* aRequest)
{
  switch (mStatus) {
    
    case NOT_STARTED:
      mRequests.AppendElement(aRequest);
      return OpenArchive();

    
    case WORKING:
      mRequests.AppendElement(aRequest);
      return NS_OK;

    
    case READY:
      RequestReady(aRequest);
      return NS_OK;
  }

  NS_ASSERTION(false, "unexpected mStatus value");
  return NS_OK;
}


nsresult
ArchiveReader::GetInputStream(nsIInputStream** aInputStream)
{
  
  mBlob->GetInternalStream(aInputStream);
  NS_ENSURE_TRUE(*aInputStream, NS_ERROR_UNEXPECTED);
  return NS_OK;
}

nsresult
ArchiveReader::GetSize(uint64_t* aSize)
{
  nsresult rv = mBlob->GetSize(aSize);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}


nsresult
ArchiveReader::OpenArchive()
{
  mStatus = WORKING;
  nsresult rv;

  
  nsCOMPtr<nsIEventTarget> target = do_GetService(NS_STREAMTRANSPORTSERVICE_CONTRACTID);
  NS_ASSERTION(target, "Must have stream transport service");

  
  nsRefPtr<ArchiveReaderEvent> event;

  
  event = new ArchiveReaderZipEvent(this, mEncoding);
  rv = target->Dispatch(event, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  AddRef();

  return NS_OK;
}


void
ArchiveReader::Ready(nsTArray<nsCOMPtr<nsIDOMFile> >& aFileList,
                     nsresult aStatus)
{
  mStatus = READY;
 
  
  mData.fileList = aFileList;
  mData.status = aStatus;

  
  for (uint32_t index = 0; index < mRequests.Length(); ++index) {
    nsRefPtr<ArchiveRequest> request = mRequests[index];
    RequestReady(request);
  }

  mRequests.Clear();

  
  Release();
}

void
ArchiveReader::RequestReady(ArchiveRequest* aRequest)
{
  
  aRequest->ReaderReady(mData.fileList, mData.status);
}

already_AddRefed<nsIDOMArchiveRequest>
ArchiveReader::GetFilenames()
{
  nsRefPtr<ArchiveRequest> request = GenerateArchiveRequest();
  request->OpGetFilenames();

  return request.forget();
}

already_AddRefed<nsIDOMArchiveRequest>
ArchiveReader::GetFile(const nsAString& filename)
{
  nsRefPtr<ArchiveRequest> request = GenerateArchiveRequest();
  request->OpGetFile(filename);

  return request.forget();
}

already_AddRefed<nsIDOMArchiveRequest>
ArchiveReader::GetFiles()
{
  nsRefPtr<ArchiveRequest> request = GenerateArchiveRequest();
  request->OpGetFiles();

  return request.forget();
}

already_AddRefed<ArchiveRequest>
ArchiveReader::GenerateArchiveRequest()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  return ArchiveRequest::Create(mWindow, this);
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_4(ArchiveReader,
                                        mBlob,
                                        mWindow,
                                        mData.fileList,
                                        mRequests)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(ArchiveReader)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(ArchiveReader)
NS_IMPL_CYCLE_COLLECTING_RELEASE(ArchiveReader)
