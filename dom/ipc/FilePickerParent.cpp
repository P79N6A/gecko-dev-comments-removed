





#include "FilePickerParent.h"
#include "nsComponentManagerUtils.h"
#include "nsNetCID.h"
#include "nsIDocument.h"
#include "nsIDOMWindow.h"
#include "nsIFile.h"
#include "nsISimpleEnumerator.h"
#include "mozilla/unused.h"
#include "mozilla/dom/File.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/TabParent.h"
#include "mozilla/dom/ipc/BlobParent.h"

using mozilla::unused;
using namespace mozilla::dom;

NS_IMPL_ISUPPORTS(FilePickerParent::FilePickerShownCallback,
                  nsIFilePickerShownCallback);

NS_IMETHODIMP
FilePickerParent::FilePickerShownCallback::Done(int16_t aResult)
{
  if (mFilePickerParent) {
    mFilePickerParent->Done(aResult);
  }
  return NS_OK;
}

void
FilePickerParent::FilePickerShownCallback::Destroy()
{
  mFilePickerParent = nullptr;
}

FilePickerParent::~FilePickerParent()
{
}











FilePickerParent::FileSizeAndDateRunnable::FileSizeAndDateRunnable(FilePickerParent *aFPParent,
                                                                   nsTArray<nsRefPtr<BlobImpl>>& aBlobs)
 : mFilePickerParent(aFPParent)
{
  mBlobs.SwapElements(aBlobs);
}

bool
FilePickerParent::FileSizeAndDateRunnable::Dispatch()
{
  MOZ_ASSERT(NS_IsMainThread());

  mEventTarget = do_GetService(NS_STREAMTRANSPORTSERVICE_CONTRACTID);
  if (!mEventTarget) {
    return false;
  }

  nsresult rv = mEventTarget->Dispatch(this, NS_DISPATCH_NORMAL);
  return NS_SUCCEEDED(rv);
}

NS_IMETHODIMP
FilePickerParent::FileSizeAndDateRunnable::Run()
{
  
  
  if (NS_IsMainThread()) {
    if (mFilePickerParent) {
      mFilePickerParent->SendFiles(mBlobs);
    }
    return NS_OK;
  }

  
  for (unsigned i = 0; i < mBlobs.Length(); i++) {
    ErrorResult rv;
    mBlobs[i]->GetSize(rv);
    mBlobs[i]->GetLastModified(rv);
    mBlobs[i]->LookupAndCacheIsDirectory();
  }

  
  if (NS_FAILED(NS_DispatchToMainThread(this))) {
    
    
    
    MOZ_CRASH();
  }
  return NS_OK;
}

void
FilePickerParent::FileSizeAndDateRunnable::Destroy()
{
  mFilePickerParent = nullptr;
}

void
FilePickerParent::SendFiles(const nsTArray<nsRefPtr<BlobImpl>>& aBlobs)
{
  nsIContentParent* parent = TabParent::GetFrom(Manager())->Manager();
  InfallibleTArray<PBlobParent*> blobs;

  for (unsigned i = 0; i < aBlobs.Length(); i++) {
    BlobParent* blobParent = parent->GetOrCreateActorForBlobImpl(aBlobs[i]);
    if (blobParent) {
      blobs.AppendElement(blobParent);
    }
  }

  InputFiles inblobs;
  inblobs.blobsParent().SwapElements(blobs);
  unused << Send__delete__(this, inblobs, mResult);
}

void
FilePickerParent::Done(int16_t aResult)
{
  mResult = aResult;

  if (mResult != nsIFilePicker::returnOK) {
    unused << Send__delete__(this, void_t(), mResult);
    return;
  }

  nsTArray<nsRefPtr<BlobImpl>> blobs;
  if (mMode == nsIFilePicker::modeOpenMultiple ||
      mMode == nsIFilePicker::modeGetFolder) {
    nsCOMPtr<nsISimpleEnumerator> iter;
    NS_ENSURE_SUCCESS_VOID(mFilePicker->GetFiles(getter_AddRefs(iter)));

    nsCOMPtr<nsISupports> supports;
    bool loop = true;
    while (NS_SUCCEEDED(iter->HasMoreElements(&loop)) && loop) {
      iter->GetNext(getter_AddRefs(supports));
      if (supports) {
        nsCOMPtr<nsIFile> file = do_QueryInterface(supports);

        nsRefPtr<BlobImpl> blobimpl = new BlobImplFile(file);
        blobs.AppendElement(blobimpl);
      }
    }
  } else {
    nsCOMPtr<nsIFile> file;
    mFilePicker->GetFile(getter_AddRefs(file));
    if (file) {
      nsRefPtr<BlobImpl> blobimpl = new BlobImplFile(file);
      blobs.AppendElement(blobimpl);
    }
  }

  MOZ_ASSERT(!mRunnable);
  mRunnable = new FileSizeAndDateRunnable(this, blobs);
  
  if (!mRunnable->Dispatch()) {
    unused << Send__delete__(this, void_t(), nsIFilePicker::returnCancel);
  }
}

bool
FilePickerParent::CreateFilePicker()
{
  mFilePicker = do_CreateInstance("@mozilla.org/filepicker;1");
  if (!mFilePicker) {
    return false;
  }

  Element* element = TabParent::GetFrom(Manager())->GetOwnerElement();
  if (!element) {
    return false;
  }

  nsCOMPtr<nsIDOMWindow> window = do_QueryInterface(element->OwnerDoc()->GetWindow());
  if (!window) {
    return false;
  }

  return NS_SUCCEEDED(mFilePicker->Init(window, mTitle, mMode));
}

bool
FilePickerParent::RecvOpen(const int16_t& aSelectedType,
                           const bool& aAddToRecentDocs,
                           const nsString& aDefaultFile,
                           const nsString& aDefaultExtension,
                           InfallibleTArray<nsString>&& aFilters,
                           InfallibleTArray<nsString>&& aFilterNames,
                           const nsString& aDisplayDirectory)
{
  if (!CreateFilePicker()) {
    unused << Send__delete__(this, void_t(), nsIFilePicker::returnCancel);
    return true;
  }

  mFilePicker->SetAddToRecentDocs(aAddToRecentDocs);

  for (uint32_t i = 0; i < aFilters.Length(); ++i) {
    mFilePicker->AppendFilter(aFilterNames[i], aFilters[i]);
  }

  mFilePicker->SetDefaultString(aDefaultFile);
  mFilePicker->SetDefaultExtension(aDefaultExtension);
  mFilePicker->SetFilterIndex(aSelectedType);

  if (!aDisplayDirectory.IsEmpty()) {
    nsCOMPtr<nsIFile> localFile = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID);
    if (localFile) {
      localFile->InitWithPath(aDisplayDirectory);
      mFilePicker->SetDisplayDirectory(localFile);
    }
  }

  mCallback = new FilePickerShownCallback(this);

  mFilePicker->Open(mCallback);
  return true;
}

void
FilePickerParent::ActorDestroy(ActorDestroyReason aWhy)
{
  if (mCallback) {
    mCallback->Destroy();
    mCallback = nullptr;
  }
  if (mRunnable) {
    mRunnable->Destroy();
    mRunnable = nullptr;
  }
}
