




#ifndef NSFILEPICKERPROXY_H
#define NSFILEPICKERPROXY_H

#include "nsBaseFilePicker.h"
#include "nsString.h"
#include "nsIURI.h"
#include "nsTArray.h"
#include "nsCOMArray.h"

class nsIWidget;
class nsIFile;







class nsFilePickerProxy : public nsBaseFilePicker
{
public:
    nsFilePickerProxy();

    NS_DECL_ISUPPORTS

    
    NS_IMETHODIMP Init(nsIDOMWindow* parent, const nsAString& title, int16_t mode);
    NS_IMETHODIMP AppendFilter(const nsAString& aTitle, const nsAString& aFilter);
    NS_IMETHODIMP GetDefaultString(nsAString& aDefaultString);
    NS_IMETHODIMP SetDefaultString(const nsAString& aDefaultString);
    NS_IMETHODIMP GetDefaultExtension(nsAString& aDefaultExtension);
    NS_IMETHODIMP SetDefaultExtension(const nsAString& aDefaultExtension);
    NS_IMETHODIMP GetFilterIndex(int32_t* aFilterIndex);
    NS_IMETHODIMP SetFilterIndex(int32_t aFilterIndex);
    NS_IMETHODIMP GetFile(nsIFile** aFile);
    NS_IMETHODIMP GetFileURL(nsIURI** aFileURL);
    NS_IMETHODIMP GetFiles(nsISimpleEnumerator** aFiles);
    NS_IMETHODIMP Show(int16_t* aReturn);

private:
    ~nsFilePickerProxy();
    void InitNative(nsIWidget*, const nsAString&, short int);

    nsCOMArray<nsIFile> mFiles;

    int16_t   mMode;
    int16_t   mSelectedType;
    nsString  mFile;
    nsString  mTitle;
    nsString  mDefault;
    nsString  mDefaultExtension;

    InfallibleTArray<nsString> mFilters;
    InfallibleTArray<nsString> mFilterNames;
};

#endif 
