





#ifndef mozilla_dom_FilePickerParent_h
#define mozilla_dom_FilePickerParent_h

#include "nsIFilePicker.h"
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

  nsRefPtr<FilePickerShownCallback> mCallback;
  nsCOMPtr<nsIFilePicker> mFilePicker;

  nsString mTitle;
  int16_t mMode;
};

} 
} 

#endif
