







































#ifndef NSFILEPICKERPROXY_H
#define NSFILEPICKERPROXY_H

#include "nsBaseFilePicker.h"
#include "nsString.h"
#include "nsIURI.h"
#include "nsTArray.h"
#include "nsCOMArray.h"

class nsIWidget;
class nsILocalFile;







class nsFilePickerProxy : public nsBaseFilePicker
{
public:
    nsFilePickerProxy();

    NS_DECL_ISUPPORTS

    
    NS_IMETHODIMP Init(nsIDOMWindow* parent, const nsAString& title, PRInt16 mode);
    NS_IMETHODIMP AppendFilter(const nsAString& aTitle, const nsAString& aFilter);
    NS_IMETHODIMP GetDefaultString(nsAString& aDefaultString);
    NS_IMETHODIMP SetDefaultString(const nsAString& aDefaultString);
    NS_IMETHODIMP GetDefaultExtension(nsAString& aDefaultExtension);
    NS_IMETHODIMP SetDefaultExtension(const nsAString& aDefaultExtension);
    NS_IMETHODIMP GetFilterIndex(PRInt32* aFilterIndex);
    NS_IMETHODIMP SetFilterIndex(PRInt32 aFilterIndex);
    NS_IMETHODIMP GetFile(nsILocalFile** aFile);
    NS_IMETHODIMP GetFileURL(nsIURI** aFileURL);
    NS_IMETHODIMP GetFiles(nsISimpleEnumerator** aFiles);
    NS_IMETHODIMP Show(PRInt16* aReturn);

private:
    ~nsFilePickerProxy();
    void InitNative(nsIWidget*, const nsAString&, short int);

    nsCOMArray<nsILocalFile> mFiles;

    PRInt16   mMode;
    PRInt16   mSelectedType;
    nsString  mFile;
    nsString  mTitle;
    nsString  mDefault;
    nsString  mDefaultExtension;

    InfallibleTArray<nsString> mFilters;
    InfallibleTArray<nsString> mFilterNames;
};

#endif 
