









































#ifndef nsFilePicker_h_
#define nsFilePicker_h_

#include "nsBaseFilePicker.h"
#include "nsString.h"
#include "nsIFileChannel.h"
#include "nsILocalFile.h"
#include "nsCOMArray.h"
#include "nsTArray.h"

class nsILocalFileMac;
@class NSArray;

class nsFilePicker : public nsBaseFilePicker
{
public:
  nsFilePicker(); 
  virtual ~nsFilePicker();

  NS_DECL_ISUPPORTS

  
  NS_IMETHOD GetDefaultString(nsAString& aDefaultString);
  NS_IMETHOD SetDefaultString(const nsAString& aDefaultString);
  NS_IMETHOD GetDefaultExtension(nsAString& aDefaultExtension);
  NS_IMETHOD GetFilterIndex(PRInt32 *aFilterIndex);
  NS_IMETHOD SetFilterIndex(PRInt32 aFilterIndex);
  NS_IMETHOD SetDefaultExtension(const nsAString& aDefaultExtension);
  NS_IMETHOD GetFile(nsILocalFile * *aFile);
  NS_IMETHOD GetFileURL(nsIURI * *aFileURL);
  NS_IMETHOD GetFiles(nsISimpleEnumerator **aFiles);
  NS_IMETHOD Show(PRInt16 *_retval); 
  NS_IMETHOD AppendFilter(const nsAString& aTitle, const nsAString& aFilter);

  




  NSArray* GetFilterList();

protected:

  virtual void InitNative(nsIWidget *aParent, const nsAString& aTitle, PRInt16 aMode);

  
  
  
  
  PRInt16 GetLocalFiles(const nsString& inTitle, PRBool inAllowMultiple, nsCOMArray<nsILocalFile>& outFiles);
  PRInt16 GetLocalFolder(const nsString& inTitle, nsILocalFile** outFile);
  PRInt16 PutLocalFile(const nsString& inTitle, const nsString& inDefaultName, nsILocalFile** outFile);

  void     SetDialogTitle(const nsString& inTitle, id aDialog);
  NSString *PanelDefaultDirectory();
  NSView* GetAccessoryView();
                                                
  nsString               mTitle;
  PRInt16                mMode;
  nsCOMArray<nsILocalFile> mFiles;
  nsString               mDefault;

  nsTArray<nsString>     mFilters; 
  nsTArray<nsString>     mTitles;

  PRInt32                mSelectedTypeIndex;
};

#endif 
