




































#ifndef NSFILEPICKER_H
#define NSFILEPICKER_H

#include "nsBaseFilePicker.h"
#include "nsString.h"

class nsFilePicker : public nsBaseFilePicker
{
public:
    NS_DECL_ISUPPORTS

    NS_IMETHODIMP Init(nsIDOMWindow *parent, const nsAString& title,
                       PRInt16 mode);
    NS_IMETHOD AppendFilter(const nsAString & aTitle,
                            const nsAString & aFilter);
    NS_IMETHOD GetDefaultString(nsAString & aDefaultString);
    NS_IMETHOD SetDefaultString(const nsAString & aDefaultString);
    NS_IMETHOD GetDefaultExtension(nsAString & aDefaultExtension);
    NS_IMETHOD SetDefaultExtension(const nsAString & aDefaultExtension);
    NS_IMETHOD GetFile(nsILocalFile * *aFile);
    NS_IMETHOD GetFileURL(nsIURI * *aFileURL);
    NS_IMETHOD SetDisplayDirectory(nsILocalFile *aDisplayDirectory);
    NS_IMETHOD GetDisplayDirectory(nsILocalFile **aDisplayDirectory);
    NS_IMETHOD Show(PRInt16 *aReturn);
private:
    void InitNative(nsIWidget*, const nsAString&, PRInt16) {};
    nsString mFilePath;
    nsString mFilters;
};
#endif
