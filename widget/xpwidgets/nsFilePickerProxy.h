




#ifndef NSFILEPICKERPROXY_H
#define NSFILEPICKERPROXY_H

#include "nsBaseFilePicker.h"
#include "nsString.h"
#include "nsIURI.h"
#include "nsTArray.h"
#include "nsCOMArray.h"

#include "mozilla/dom/PFilePickerChild.h"

class nsIWidget;
class nsIFile;
class nsPIDOMWindow;







class nsFilePickerProxy : public nsBaseFilePicker,
                          public mozilla::dom::PFilePickerChild
{
public:
    nsFilePickerProxy();

    NS_DECL_ISUPPORTS

    
    NS_IMETHODIMP Init(nsIDOMWindow* aParent, const nsAString& aTitle, int16_t aMode);
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

    NS_IMETHODIMP GetDomfile(nsIDOMFile** aFile);
    NS_IMETHODIMP GetDomfiles(nsISimpleEnumerator** aFiles);

    NS_IMETHODIMP Show(int16_t* aReturn);
    NS_IMETHODIMP Open(nsIFilePickerShownCallback* aCallback);

    
    virtual bool
    Recv__delete__(const MaybeInputFiles& aFiles, const int16_t& aResult);

private:
    ~nsFilePickerProxy();
    void InitNative(nsIWidget*, const nsAString&);

    
    nsCOMPtr<nsPIDOMWindow> mParent;
    nsCOMArray<nsIDOMFile> mDomfiles;
    nsCOMPtr<nsIFilePickerShownCallback> mCallback;

    int16_t   mSelectedType;
    nsString  mFile;
    nsString  mDefault;
    nsString  mDefaultExtension;

    InfallibleTArray<nsString> mFilters;
    InfallibleTArray<nsString> mFilterNames;
};

#endif 
