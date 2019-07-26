





#ifndef mozilla_dom_FilePickerParent_h
#define mozilla_dom_FilePickerParent_h

#include "nsIDOMFile.h"
#include "nsIEventTarget.h"
#include "nsIFilePicker.h"
#include "nsCOMArray.h"
#include "nsThreadUtils.h"
#include "mozilla/dom/PFilePickerParent.h"

namespace mozilla {
namespace dom {

class FilePickerParent : public PFilePickerParent
{
 public:
  FilePickerParent(const nsString& aTitle,
                   const int16_t& aMode)
  : mTitle(aTitle)
  , mMode(aMode)
  {}

  virtual ~FilePickerParent();

  void Done(int16_t aResult);
  void SendFiles(const nsCOMArray<nsIDOMFile>& aDomfiles);

  virtual bool RecvOpen(const int16_t& aSelectedType,
                        const bool& aAddToRecentDocs,
                        const nsString& aDefaultFile,
                        const nsString& aDefaultExtension,
                        const InfallibleTArray<nsString>& aFilters,
                        const InfallibleTArray<nsString>& aFilterNames) MOZ_OVERRIDE;

  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  class FilePickerShownCallback : public nsIFilePickerShownCallback
  {
  public:
    FilePickerShownCallback(FilePickerParent* aFilePickerParent)
      : mFilePickerParent(aFilePickerParent)
    { }
    virtual ~FilePickerShownCallback() {}

    NS_DECL_ISUPPORTS
    NS_DECL_NSIFILEPICKERSHOWNCALLBACK

    void Destroy();

  private:
    FilePickerParent* mFilePickerParent;
  };

 private:
  bool CreateFilePicker();

  class FileSizeAndDateRunnable : public nsRunnable
  {
    FilePickerParent* mFilePickerParent;
    nsCOMArray<nsIDOMFile> mDomfiles;
    nsCOMPtr<nsIEventTarget> mEventTarget;

  public:
    FileSizeAndDateRunnable(FilePickerParent *aFPParent, nsCOMArray<nsIDOMFile>& aDomfiles);
    bool Dispatch();
    NS_IMETHOD Run();
    void Destroy();
  };

  nsRefPtr<FileSizeAndDateRunnable> mRunnable;
  nsRefPtr<FilePickerShownCallback> mCallback;
  nsCOMPtr<nsIFilePicker> mFilePicker;

  nsString mTitle;
  int16_t mMode;
  int16_t mResult;
};

} 
} 

#endif
