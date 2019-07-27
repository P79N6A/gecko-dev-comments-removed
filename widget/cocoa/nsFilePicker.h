





#ifndef nsFilePicker_h_
#define nsFilePicker_h_

#include "nsBaseFilePicker.h"
#include "nsString.h"
#include "nsIFileChannel.h"
#include "nsIFile.h"
#include "nsCOMArray.h"
#include "nsTArray.h"

class nsILocalFileMac;
@class NSArray;

class nsFilePicker : public nsBaseFilePicker
{
public:
  nsFilePicker(); 

  NS_DECL_ISUPPORTS

  
  NS_IMETHOD GetDefaultString(nsAString& aDefaultString) override;
  NS_IMETHOD SetDefaultString(const nsAString& aDefaultString) override;
  NS_IMETHOD GetDefaultExtension(nsAString& aDefaultExtension) override;
  NS_IMETHOD GetFilterIndex(int32_t *aFilterIndex) override;
  NS_IMETHOD SetFilterIndex(int32_t aFilterIndex) override;
  NS_IMETHOD SetDefaultExtension(const nsAString& aDefaultExtension) override;
  NS_IMETHOD GetFile(nsIFile * *aFile) override;
  NS_IMETHOD GetFileURL(nsIURI * *aFileURL) override;
  NS_IMETHOD GetFiles(nsISimpleEnumerator **aFiles) override;
  NS_IMETHOD Show(int16_t *_retval) override; 
  NS_IMETHOD AppendFilter(const nsAString& aTitle, const nsAString& aFilter) override;

  




  NSArray* GetFilterList();

protected:
  virtual ~nsFilePicker();

  virtual void InitNative(nsIWidget *aParent, const nsAString& aTitle) override;

  
  
  
  
  int16_t GetLocalFiles(const nsString& inTitle, bool inAllowMultiple, nsCOMArray<nsIFile>& outFiles);
  int16_t GetLocalFolder(const nsString& inTitle, nsIFile** outFile);
  int16_t PutLocalFile(const nsString& inTitle, const nsString& inDefaultName, nsIFile** outFile);

  void     SetDialogTitle(const nsString& inTitle, id aDialog);
  NSString *PanelDefaultDirectory();
  NSView* GetAccessoryView();
                                                
  nsString               mTitle;
  nsCOMArray<nsIFile>    mFiles;
  nsString               mDefault;

  nsTArray<nsString>     mFilters; 
  nsTArray<nsString>     mTitles;

  int32_t                mSelectedTypeIndex;
};

#endif 
