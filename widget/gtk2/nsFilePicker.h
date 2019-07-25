




































#ifndef nsFilePicker_h__
#define nsFilePicker_h__

#include <gtk/gtk.h>

#include "nsBaseFilePicker.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsCOMArray.h"

class nsIWidget;
class nsILocalFile;

class nsFilePicker : public nsBaseFilePicker
{
public:
  nsFilePicker();
  virtual ~nsFilePicker();

  NS_DECL_ISUPPORTS

  
  NS_IMETHODIMP AppendFilters(PRInt32 aFilterMask);
  NS_IMETHODIMP AppendFilter(const nsAString& aTitle, const nsAString& aFilter);
  NS_IMETHODIMP SetDefaultString(const nsAString& aString);
  NS_IMETHODIMP GetDefaultString(nsAString& aString);
  NS_IMETHODIMP SetDefaultExtension(const nsAString& aExtension);
  NS_IMETHODIMP GetDefaultExtension(nsAString& aExtension);
  NS_IMETHODIMP GetFilterIndex(PRInt32 *aFilterIndex);
  NS_IMETHODIMP SetFilterIndex(PRInt32 aFilterIndex);
  NS_IMETHODIMP GetFile(nsILocalFile **aFile);
  NS_IMETHODIMP GetFileURL(nsIURI **aFileURL);
  NS_IMETHODIMP GetFiles(nsISimpleEnumerator **aFiles);
  NS_IMETHODIMP Show(PRInt16 *aReturn);

  virtual void InitNative(nsIWidget *aParent, const nsAString& aTitle, PRInt16 aMode);

  static void Shutdown();

protected:

  void ReadValuesFromFileChooser(GtkWidget *file_chooser);

  nsCOMPtr<nsIWidget>    mParentWidget;
  nsCOMArray<nsILocalFile> mFiles;

  PRInt16   mMode;
  PRInt16   mSelectedType;
  bool      mAllowURLs;
  nsCString mFileURL;
  nsString  mTitle;
  nsString  mDefault;
  nsString  mDefaultExtension;

  nsTArray<nsCString> mFilters;
  nsTArray<nsCString> mFilterNames;

private:
  static nsILocalFile *mPrevDisplayDirectory;
};

#endif
