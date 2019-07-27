




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

    
    NS_IMETHODIMP Init(nsIDOMWindow* aParent, const nsAString& aTitle, int16_t aMode) MOZ_OVERRIDE;
    NS_IMETHODIMP AppendFilter(const nsAString& aTitle, const nsAString& aFilter) MOZ_OVERRIDE;
    NS_IMETHODIMP GetDefaultString(nsAString& aDefaultString) MOZ_OVERRIDE;
    NS_IMETHODIMP SetDefaultString(const nsAString& aDefaultString) MOZ_OVERRIDE;
    NS_IMETHODIMP GetDefaultExtension(nsAString& aDefaultExtension) MOZ_OVERRIDE;
    NS_IMETHODIMP SetDefaultExtension(const nsAString& aDefaultExtension) MOZ_OVERRIDE;
    NS_IMETHODIMP GetFilterIndex(int32_t* aFilterIndex) MOZ_OVERRIDE;
    NS_IMETHODIMP SetFilterIndex(int32_t aFilterIndex) MOZ_OVERRIDE;
    NS_IMETHODIMP GetFile(nsIFile** aFile) MOZ_OVERRIDE;
    NS_IMETHODIMP GetFileURL(nsIURI** aFileURL) MOZ_OVERRIDE;
    NS_IMETHODIMP GetFiles(nsISimpleEnumerator** aFiles) MOZ_OVERRIDE;

    NS_IMETHODIMP GetDomfile(nsIDOMFile** aFile) MOZ_OVERRIDE;
    NS_IMETHODIMP GetDomfiles(nsISimpleEnumerator** aFiles) MOZ_OVERRIDE;

    NS_IMETHODIMP Show(int16_t* aReturn) MOZ_OVERRIDE;
    NS_IMETHODIMP Open(nsIFilePickerShownCallback* aCallback) MOZ_OVERRIDE;

    
    virtual bool
    Recv__delete__(const MaybeInputFiles& aFiles, const int16_t& aResult) MOZ_OVERRIDE;

private:
    ~nsFilePickerProxy();
    void InitNative(nsIWidget*, const nsAString&) MOZ_OVERRIDE;

    
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
