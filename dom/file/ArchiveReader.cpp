





#include "ArchiveReader.h"
#include "ArchiveRequest.h"
#include "ArchiveEvent.h"
#include "ArchiveZipEvent.h"

#include "nsContentUtils.h"
#include "nsLayoutStatics.h"
#include "nsDOMClassInfoID.h"

#include "nsIURI.h"
#include "nsNetUtil.h"

#include "mozilla/Preferences.h"

USING_FILE_NAMESPACE

ArchiveReader::ArchiveReader()
: mBlob(nullptr),
  mWindow(nullptr),
  mStatus(NOT_STARTED)
{
  MOZ_COUNT_CTOR(ArchiveReader);
  nsLayoutStatics::AddRef();
}

ArchiveReader::~ArchiveReader()
{
  MOZ_COUNT_DTOR(ArchiveReader);
  nsLayoutStatics::Release();
}

bool
ArchiveReader::PrefEnabled()
{
  return Preferences::GetBool("dom.archivereader.enabled", true);
}

NS_IMETHODIMP
ArchiveReader::Initialize(nsISupports* aOwner,
                          JSContext* aCx,
                          JSObject* aObj,
                          uint32_t aArgc,
                          JS::Value* aArgv)
{
  NS_ENSURE_TRUE(aArgc == 1 || aArgc == 2, NS_ERROR_INVALID_ARG);

  if (!PrefEnabled()) {
    return NS_ERROR_UNEXPECTED;
  }

  
  if (!aArgv[0].isObject()) {
    return NS_ERROR_INVALID_ARG; 
  }

  JSObject* obj = &aArgv[0].toObject();

  nsCOMPtr<nsIDOMBlob> blob;
  blob = do_QueryInterface(nsContentUtils::XPConnect()->GetNativeOfWrapper(aCx, obj));
  if (!blob) {
    return NS_ERROR_INVALID_ARG;
  }

  
  if (aArgc > 1) {
    nsresult rv = mOptions.Init(aCx, &aArgv[1]);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mWindow = do_QueryInterface(aOwner);
  if (!mWindow) {
    return NS_ERROR_UNEXPECTED;
  }

  mBlob = blob;

  return NS_OK;
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

  
  event = new ArchiveReaderZipEvent(this, mOptions);
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


NS_IMETHODIMP
ArchiveReader::GetFilenames(nsIDOMArchiveRequest** _retval)
{
  nsRefPtr<ArchiveRequest> request = GenerateArchiveRequest();
  request->OpGetFilenames();

  request.forget(_retval);
  return NS_OK;
}


NS_IMETHODIMP
ArchiveReader::GetFile(const nsAString& filename,
                       nsIDOMArchiveRequest** _retval)
{
  nsRefPtr<ArchiveRequest> request = GenerateArchiveRequest();
  request->OpGetFile(filename);

  request.forget(_retval);
  return NS_OK;
}


NS_IMETHODIMP
ArchiveReader::GetFiles(nsIDOMArchiveRequest** _retval)
{
  nsRefPtr<ArchiveRequest> request = GenerateArchiveRequest();
  request->OpGetFiles();

  request.forget(_retval);
  return NS_OK;
}

already_AddRefed<ArchiveRequest>
ArchiveReader::GenerateArchiveRequest()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  return ArchiveRequest::Create(mWindow, this);
}

NS_IMPL_CYCLE_COLLECTION_CLASS(ArchiveReader)


NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(ArchiveReader)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mBlob)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mWindow)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mData.fileList)

  for (uint32_t i = 0; i < tmp->mRequests.Length(); i++) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mRequests[i]");
    cb.NoteXPCOMChild(static_cast<nsIDOMArchiveRequest*>(tmp->mRequests[i].get()));
  }

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END


NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(ArchiveReader)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mBlob)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mWindow)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mData.fileList)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mRequests)
  tmp->mRequests.Clear();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(ArchiveReader)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMArchiveReader)
  NS_INTERFACE_MAP_ENTRY(nsIJSNativeInitializer)
  NS_INTERFACE_MAP_ENTRY(nsIDOMArchiveReader)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(ArchiveReader)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(ArchiveReader)
NS_IMPL_CYCLE_COLLECTING_RELEASE(ArchiveReader)

DOMCI_DATA(ArchiveReader, ArchiveReader)
