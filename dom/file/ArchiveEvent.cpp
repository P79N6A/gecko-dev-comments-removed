





#include "ArchiveEvent.h"

#include "nsContentUtils.h"
#include "nsCExternalHandlerService.h"
#include "nsProxyRelease.h"

USING_FILE_NAMESPACE

NS_IMPL_THREADSAFE_ISUPPORTS0(ArchiveItem)

ArchiveItem::ArchiveItem()
{
  MOZ_COUNT_CTOR(ArchiveItem);
}

ArchiveItem::~ArchiveItem()
{
  MOZ_COUNT_DTOR(ArchiveItem);
}


nsCString
ArchiveItem::GetType()
{
  return mType.IsEmpty() ? nsCString("binary/octet-stream") : mType;
}

void
ArchiveItem::SetType(const nsCString& aType)
{
  mType = aType;
}

ArchiveReaderEvent::ArchiveReaderEvent(ArchiveReader* aArchiveReader)
: mArchiveReader(aArchiveReader)
{
  MOZ_COUNT_CTOR(ArchiveReaderEvent);
}

ArchiveReaderEvent::~ArchiveReaderEvent()
{
  if (!NS_IsMainThread()) {
    nsIMIMEService* mimeService;
    mMimeService.forget(&mimeService);

    if (mimeService) {
      nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
      NS_WARN_IF_FALSE(mainThread, "Couldn't get the main thread! Leaking!");

      if (mainThread) {
        NS_ProxyRelease(mainThread, mimeService);
      }
    }
  }

  MOZ_COUNT_DTOR(ArchiveReaderEvent);
}


nsresult
ArchiveReaderEvent::GetType(nsCString& aExt,
                            nsCString& aMimeType)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsresult rv;
  
  if (mMimeService.get() == nullptr) {
    mMimeService = do_GetService(NS_MIMESERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = mMimeService->GetTypeFromExtension(aExt, aMimeType);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
ArchiveReaderEvent::Run()
{
  return Exec();
}

nsresult
ArchiveReaderEvent::RunShare(nsresult aStatus)
{
  mStatus = aStatus;

  nsCOMPtr<nsIRunnable> event = NS_NewRunnableMethod(this, &ArchiveReaderEvent::ShareMainThread);
  NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);

  return NS_OK;
}

void
ArchiveReaderEvent::ShareMainThread()
{
  nsTArray<nsCOMPtr<nsIDOMFile> > fileList;

  if (!NS_FAILED(mStatus)) {
    
    for (uint32_t index = 0; index < mFileList.Length(); ++index) {
      nsRefPtr<ArchiveItem> item = mFileList[index];

      int32_t offset = item->GetFilename().RFindChar('.');
      if (offset != kNotFound) {
        nsCString ext(item->GetFilename());
        ext.Cut(0, offset + 1);

        
        nsCString type;
        if (NS_SUCCEEDED(GetType(ext, type)))
          item->SetType(type);
      }

      
      nsRefPtr<nsIDOMFile> file = item->File(mArchiveReader);
      fileList.AppendElement(file);
    }
  }

  mArchiveReader->Ready(fileList, mStatus);
}
