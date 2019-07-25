





#ifndef nsFilePicker_h__
#define nsFilePicker_h__

#include "nsISimpleEnumerator.h"
#include "nsCOMArray.h"
#include "nsTArray.h"
#include "nsICharsetConverterManager.h"
#include "nsBaseFilePicker.h"
#include "nsString.h"

#define INCL_DOS
#define INCL_NLS
#define INCL_WIN
#define INCL_WINSTDFILE
#include <os2.h>





class nsFilePicker : public nsBaseFilePicker
{
public:
  nsFilePicker(); 
  virtual ~nsFilePicker();

  static void ReleaseGlobals();

  NS_DECL_ISUPPORTS

    
  NS_IMETHOD GetDefaultString(nsAString& aDefaultString);
  NS_IMETHOD SetDefaultString(const nsAString& aDefaultString);
  NS_IMETHOD GetDefaultExtension(nsAString& aDefaultExtension);
  NS_IMETHOD SetDefaultExtension(const nsAString& aDefaultExtension);
  NS_IMETHOD GetFilterIndex(int32_t *aFilterIndex);
  NS_IMETHOD SetFilterIndex(int32_t aFilterIndex);
  NS_IMETHOD GetFile(nsIFile * *aFile);
  NS_IMETHOD GetFileURL(nsIURI * *aFileURL);
  NS_IMETHOD GetFiles(nsISimpleEnumerator **aFiles);
  NS_IMETHOD Show(int16_t *_retval); 
  NS_IMETHOD AppendFilter(const nsAString& aTitle, const nsAString& aFilter);

protected:
  
  virtual void InitNative(nsIWidget *aParent, const nsAString& aTitle,
                          int16_t aMode);


  void GetFilterListArray(nsString& aFilterList);
  static void GetFileSystemCharset(nsCString & fileSystemCharset);
  char * ConvertToFileSystemCharset(const nsAString& inString);
  PRUnichar * ConvertFromFileSystemCharset(const char *inString);

  HWND                   mWnd;
  nsString               mTitle;
  int16_t                mMode;
  nsCString              mFile;
  nsString               mDefault;
  nsString               mDefaultExtension;
  nsTArray<nsString>     mFilters;
  nsTArray<nsString>     mTitles;
  nsIUnicodeEncoder*     mUnicodeEncoder;
  nsIUnicodeDecoder*     mUnicodeDecoder;
  int16_t                mSelectedType;
  nsCOMArray<nsIFile> mFiles;
  static char            mLastUsedDirectory[];
};

#endif 
