








































#ifndef nsFilePicker_h__
#define nsFilePicker_h__

#ifndef DEBUG



#define FILEPICKER_SAVE_LAST_DIR 1
#endif

#include "nsICharsetConverterManager.h"
#include "nsBaseFilePicker.h"
#include "nsString.h"
#include "nsdefs.h"
#include "nsIFileChannel.h"
#include "nsILocalFile.h"
#include "nsISimpleEnumerator.h"
#include "nsISupportsArray.h"





#include <StorageKit.h>
#include <Message.h>
#include <Window.h>
#include <String.h>
class BButton;

class nsFilePicker : public nsBaseFilePicker
{
public:
  NS_DECL_ISUPPORTS

  nsFilePicker();
  virtual ~nsFilePicker();

  
  NS_IMETHOD GetDefaultString(nsAString& aDefaultString);
  NS_IMETHOD SetDefaultString(const nsAString& aDefaultString);
  NS_IMETHOD GetDefaultExtension(nsAString& aDefaultExtension);
  NS_IMETHOD SetDefaultExtension(const nsAString& aDefaultExtension);
  NS_IMETHOD GetFile(nsILocalFile * *aFile);
  NS_IMETHOD GetFileURL(nsIFileURL * *aFileURL);
  NS_IMETHOD GetFiles(nsISimpleEnumerator **aFiles);
  NS_IMETHOD Show(PRInt16 *_retval);
  NS_IMETHOD AppendFilter(const nsAString& aTitle, const nsAString& aFilter);

protected:
  
  virtual void InitNative(nsIWidget *aParent, const nsAString& aTitle,
                          PRInt16 aMode);


  void GetFilterListArray(nsString& aFilterList);

  BWindow*                      mParentWindow;
  nsString                      mTitle;
  PRInt16                       mMode;
  nsCString                     mFile;
  nsString                      mDefault;
  nsString                      mFilterList;
  nsIUnicodeEncoder*            mUnicodeEncoder;
  nsIUnicodeDecoder*            mUnicodeDecoder;
  PRInt16                       mSelectedType;
  nsCOMPtr <nsISupportsArray>   mFiles;

#ifdef FILEPICKER_SAVE_LAST_DIR
  static char                      mLastUsedDirectory[];
#endif
};

class nsFilePanelBeOS : public BLooper, public BFilePanel
{
public:
  nsFilePanelBeOS(file_panel_mode mode,
                  uint32 node_flavors,
                  bool allow_multiple_selection,
                  bool modal,
                  bool hide_when_done);
  virtual ~nsFilePanelBeOS();

  virtual void MessageReceived(BMessage *message);
  virtual void WaitForSelection();

  virtual void SelectionChanged();

  virtual bool IsOpenSelected() {
    return (SelectedActivity() == OPEN_SELECTED);
  }
  virtual bool IsSaveSelected() {
    return (SelectedActivity() == SAVE_SELECTED);
  }
  virtual bool IsCancelSelected() {
    return (SelectedActivity() == CANCEL_SELECTED);
  }
  virtual uint32 SelectedActivity();

  virtual BList *OpenRefs() { return &mOpenRefs; }
  virtual BString SaveFileName() { return mSaveFileName; }
  virtual entry_ref SaveDirRef() { return mSaveDirRef; }

  enum {
      NOT_SELECTED    = 0,
      OPEN_SELECTED   = 1,
      SAVE_SELECTED   = 2,
      CANCEL_SELECTED = 3
  };

protected:
  BButton *mDirectoryButton;
  sem_id wait_sem ;
  uint32 mSelectedActivity;
  bool mIsSelected;
  BString mSaveFileName;
  entry_ref mSaveDirRef;
  BList mOpenRefs;
};

#endif 
