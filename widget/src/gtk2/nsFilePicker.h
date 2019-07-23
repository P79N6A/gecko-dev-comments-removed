




































#ifndef nsFilePicker_h__
#define nsFilePicker_h__

#include <gtk/gtkwidget.h>

#include "nsBaseFilePicker.h"
#include "nsString.h"
#include "nsVoidArray.h"
#include "nsCOMArray.h"

class nsIWidget;
class nsILocalFile;
class PRLibrary;

class nsFilePicker : public nsBaseFilePicker
{
public:
  nsFilePicker();
  virtual ~nsFilePicker();

  NS_DECL_ISUPPORTS

  
  NS_IMETHODIMP Init(nsIDOMWindow *aParent, const nsAString &aTitle, PRInt16 aMode);
  NS_IMETHODIMP AppendFilters(PRInt32 aFilterMask);
  NS_IMETHODIMP AppendFilter(const nsAString& aTitle, const nsAString& aFilter);
  NS_IMETHODIMP SetDefaultString(const nsAString& aString);
  NS_IMETHODIMP GetDefaultString(nsAString& aString);
  NS_IMETHODIMP SetDefaultExtension(const nsAString& aExtension);
  NS_IMETHODIMP GetDefaultExtension(nsAString& aExtension);
  NS_IMETHODIMP GetFilterIndex(PRInt32 *aFilterIndex);
  NS_IMETHODIMP SetFilterIndex(PRInt32 aFilterIndex);
  NS_IMETHODIMP GetFile(nsILocalFile **aFile);
  NS_IMETHODIMP GetFileURL(nsIFileURL **aFileURL);
  NS_IMETHODIMP GetFiles(nsISimpleEnumerator **aFiles);
  NS_IMETHODIMP Show(PRInt16 *aReturn);

  virtual void InitNative(nsIWidget *aParent, const nsAString& aTitle, PRInt16 aMode);

  static void Shutdown();

protected:
  static nsresult LoadSymbolsGTK24();

  void ReadValuesFromFileChooser(GtkWidget *file_chooser);

  nsCOMPtr<nsIWidget>    mParentWidget;
  nsCOMArray<nsILocalFile> mFiles;

  PRInt16   mMode;
  PRInt16   mSelectedType;
  nsCString mFile;
  nsString  mTitle;
  nsString  mDefault;
  nsString  mDefaultExtension;

  nsCStringArray mFilters;
  nsCStringArray mFilterNames;

private:
  static nsILocalFile *mPrevDisplayDirectory;
  static PRLibrary *mGTK24;
};

#endif
