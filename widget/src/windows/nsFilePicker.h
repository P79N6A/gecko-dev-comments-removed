







































#ifndef nsFilePicker_h__
#define nsFilePicker_h__

#include "nsILocalFile.h"
#include "nsISimpleEnumerator.h"
#include "nsCOMArray.h"
#include "nsAutoPtr.h"

#include "nsICharsetConverterManager.h"
#include "nsBaseFilePicker.h"
#include "nsString.h"
#include "nsdefs.h"
#include <windows.h>
#include <commdlg.h>




class nsFilePicker : public nsBaseFilePicker
{
public:
  nsFilePicker(); 
  virtual ~nsFilePicker();

  NS_DECL_ISUPPORTS

    
  NS_IMETHOD GetDefaultString(nsAString& aDefaultString);
  NS_IMETHOD SetDefaultString(const nsAString& aDefaultString);
  NS_IMETHOD GetDefaultExtension(nsAString& aDefaultExtension);
  NS_IMETHOD SetDefaultExtension(const nsAString& aDefaultExtension);
  NS_IMETHOD GetFilterIndex(PRInt32 *aFilterIndex);
  NS_IMETHOD SetFilterIndex(PRInt32 aFilterIndex);
  NS_IMETHOD GetFile(nsILocalFile * *aFile);
  NS_IMETHOD GetFileURL(nsIURI * *aFileURL);
  NS_IMETHOD GetFiles(nsISimpleEnumerator **aFiles);
  NS_IMETHOD Show(PRInt16 *aReturnVal); 
  NS_IMETHOD ShowW(PRInt16 *aReturnVal); 
  NS_IMETHOD AppendFilter(const nsAString& aTitle, const nsAString& aFilter);

protected:
  enum PickerType {
    PICKER_TYPE_OPEN,
    PICKER_TYPE_SAVE,
  };

  
  virtual void InitNative(nsIWidget *aParent,
                          const nsAString& aTitle,
                          PRInt16 aMode);
  static void GetQualifiedPath(const PRUnichar *aInPath, nsString &aOutPath);
  void GetFilterListArray(nsString& aFilterList);
  bool GetFileName(OPENFILENAMEW* ofn, PickerType aType);

  nsCOMPtr<nsIWidget>    mParentWidget;
  nsString               mTitle;
  PRInt16                mMode;
  nsCString              mFile;
  nsString               mDefault;
  nsString               mDefaultExtension;
  nsString               mFilterList;
  PRInt16                mSelectedType;
  nsCOMArray<nsILocalFile> mFiles;
  static char            mLastUsedDirectory[];
  nsString               mUnicodeFile;
  static PRUnichar      *mLastUsedUnicodeDirectory;
};



class AutoRestoreWorkingPath
{
public:
  AutoRestoreWorkingPath();
  ~AutoRestoreWorkingPath();
  inline bool HasWorkingPath() const
  {
    return mWorkingPath != NULL;
  }

private:
  nsAutoArrayPtr<PRUnichar> mWorkingPath;
};

#endif
