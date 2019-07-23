






































#ifndef NSFILEPICKER_H
#define NSFILEPICKER_H

#include "nsBaseFilePicker.h"
#include "nsString.h"
#include "nsVoidArray.h"

class nsIWidget;
class nsILocalFile;
class nsISupportsArray;
class QFileDialog;

class nsFilePicker : public nsBaseFilePicker
{
public:
    nsFilePicker();

    NS_DECL_ISUPPORTS

    
    NS_IMETHODIMP Init(nsIDOMWindow *parent, const nsAString & title, PRInt16 mode);
    NS_IMETHODIMP AppendFilters(PRInt32 filterMask);
    NS_IMETHODIMP AppendFilter(const nsAString & aTitle, const nsAString & aFilter);
    NS_IMETHODIMP GetDefaultString(nsAString & aDefaultString);
    NS_IMETHODIMP SetDefaultString(const nsAString & aDefaultString);
    NS_IMETHODIMP GetDefaultExtension(nsAString & aDefaultExtension);
    NS_IMETHODIMP SetDefaultExtension(const nsAString & aDefaultExtension);
    NS_IMETHODIMP GetFilterIndex(PRInt32 *aFilterIndex);
    NS_IMETHODIMP SetFilterIndex(PRInt32 aFilterIndex);
    NS_IMETHODIMP GetFile(nsILocalFile * *aFile);
    NS_IMETHODIMP GetFileURL(nsIFileURL * *aFileURL);
    NS_IMETHODIMP GetFiles(nsISimpleEnumerator * *aFiles);
    NS_IMETHODIMP Show(PRInt16 *aReturn);

private:
    ~nsFilePicker();
    void InitNative(nsIWidget*, const nsAString&, short int);

protected:
    QFileDialog *mDialog;
    nsCOMPtr<nsISupportsArray> mFiles;

    PRInt16   mMode;
    PRInt16   mSelectedType;
    nsCString mFile;
    nsString  mTitle;
    nsString  mDefault;
    nsString  mDefaultExtension;

    nsCStringArray mFilters;
    nsCStringArray mFilterNames;
};

#endif
