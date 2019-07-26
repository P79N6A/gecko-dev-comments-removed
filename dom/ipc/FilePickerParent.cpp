





#include "FilePickerParent.h"
#include "nsComponentManagerUtils.h"
#include "nsIDocument.h"
#include "nsIDOMWindow.h"
#include "nsIFile.h"
#include "nsISimpleEnumerator.h"
#include "mozilla/unused.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/TabParent.h"

using mozilla::unused;
using namespace mozilla::dom;

NS_IMPL_ISUPPORTS1(FilePickerParent::FilePickerShownCallback,
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

void
FilePickerParent::Done(int16_t aResult)
{
  InfallibleTArray<nsString> files;

  if (mMode == nsIFilePicker::modeOpenMultiple) {
    nsCOMPtr<nsISimpleEnumerator> iter;
    NS_ENSURE_SUCCESS_VOID(mFilePicker->GetFiles(getter_AddRefs(iter)));

    nsCOMPtr<nsIFile> file;
    bool loop = true;
    while (NS_SUCCEEDED(iter->HasMoreElements(&loop)) && loop) {
      iter->GetNext(getter_AddRefs(file));
      if (file) {
        nsAutoString path;
        if (NS_SUCCEEDED(file->GetPath(path))) {
          files.AppendElement(path);
        }
      }
    }
  } else {
    nsCOMPtr<nsIFile> file;
    mFilePicker->GetFile(getter_AddRefs(file));

    if (file) {
      nsAutoString path;
      if (NS_SUCCEEDED(file->GetPath(path))) {
        files.AppendElement(path);
      }
    }
  }

  unused << Send__delete__(this, InputFiles(files), aResult);
}

bool
FilePickerParent::CreateFilePicker()
{
  mFilePicker = do_CreateInstance("@mozilla.org/filepicker;1");
  if (!mFilePicker) {
    return false;
  }

  Element* element = static_cast<TabParent*>(Manager())->GetOwnerElement();
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
                           const InfallibleTArray<nsString>& aFilters,
                           const InfallibleTArray<nsString>& aFilterNames)
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

  mCallback = new FilePickerShownCallback(this);

  mFilePicker->Open(mCallback);
  return true;
}

void
FilePickerParent::ActorDestroy(ActorDestroyReason aWhy)
{
  if (mCallback) {
    mCallback->Destroy();
  }
}
