





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

  
  NS_IMETHOD GetDefaultString(nsAString& aDefaultString) MOZ_OVERRIDE;
  NS_IMETHOD SetDefaultString(const nsAString& aDefaultString) MOZ_OVERRIDE;
  NS_IMETHOD GetDefaultExtension(nsAString& aDefaultExtension) MOZ_OVERRIDE;
  NS_IMETHOD GetFilterIndex(int32_t *aFilterIndex) MOZ_OVERRIDE;
  NS_IMETHOD SetFilterIndex(int32_t aFilterIndex) MOZ_OVERRIDE;
  NS_IMETHOD SetDefaultExtension(const nsAString& aDefaultExtension) MOZ_OVERRIDE;
  NS_IMETHOD GetFile(nsIFile * *aFile) MOZ_OVERRIDE;
  NS_IMETHOD GetFileURL(nsIURI * *aFileURL) MOZ_OVERRIDE;
  NS_IMETHOD GetFiles(nsISimpleEnumerator **aFiles) MOZ_OVERRIDE;
  NS_IMETHOD Show(int16_t *_retval) MOZ_OVERRIDE; 
  NS_IMETHOD AppendFilter(const nsAString& aTitle, const nsAString& aFilter) MOZ_OVERRIDE;

  




  NSArray* GetFilterList();

protected:
  virtual ~nsFilePicker();

  virtual void InitNative(nsIWidget *aParent, const nsAString& aTitle) MOZ_OVERRIDE;

  
  
  
  
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
