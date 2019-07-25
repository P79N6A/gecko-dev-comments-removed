




































#include "nsFilePicker.h"
#include "AndroidBridge.h"
#include "nsNetUtil.h"
#include "nsIURI.h"

NS_IMPL_ISUPPORTS1(nsFilePicker, nsIFilePicker)

NS_IMETHODIMP nsFilePicker::Init(nsIDOMWindow *parent, const nsAString& title, 
                                 PRInt16 mode)
{
    return nsIFilePicker::modeOpen == mode ? NS_OK : NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsFilePicker::AppendFilter(const nsAString& ,
                                         const nsAString& filter)
{
    if (!mFilters.IsEmpty())
        mFilters.AppendLiteral(", ");
    mFilters.Append(filter);
    return NS_OK;
}

NS_IMETHODIMP nsFilePicker::GetDefaultString(nsAString & aDefaultString)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsFilePicker::SetDefaultString(const nsAString & aDefaultString)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsFilePicker::GetDefaultExtension(nsAString & aDefaultExtension)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsFilePicker::SetDefaultExtension(const nsAString & aDefaultExtension)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsFilePicker::GetDisplayDirectory(nsILocalFile **aDisplayDirectory)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsFilePicker::SetDisplayDirectory(nsILocalFile *aDisplayDirectory)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsFilePicker::GetFile(nsILocalFile **aFile)
{
    NS_ENSURE_ARG_POINTER(aFile);

    *aFile = nsnull;
    if (mFilePath.IsEmpty()) {
        return NS_OK;
    }

    nsCOMPtr<nsILocalFile> file(do_CreateInstance("@mozilla.org/file/local;1"));
    NS_ENSURE_TRUE(file, NS_ERROR_FAILURE);

    file->InitWithPath(mFilePath);

    NS_ADDREF(*aFile = file);

    nsCString path;
    (*aFile)->GetNativePath(path);
    return NS_OK;
}

NS_IMETHODIMP nsFilePicker::GetFileURL(nsIURI **aFileURL)
{
    nsCOMPtr<nsILocalFile> file;
    GetFile(getter_AddRefs(file));

    nsCOMPtr<nsIURI> uri;
    NS_NewFileURI(getter_AddRefs(uri), file);
    NS_ENSURE_TRUE(uri, NS_ERROR_FAILURE);

    return CallQueryInterface(uri, aFileURL);
}

NS_IMETHODIMP nsFilePicker::Show(PRInt16 *_retval NS_OUTPARAM)
{
    if (!mozilla::AndroidBridge::Bridge())
        return NS_ERROR_NOT_IMPLEMENTED;
    nsAutoString filePath;

    mozilla::AndroidBridge::Bridge()->ShowFilePicker(filePath, mFilters);
    *_retval = EmptyString().Equals(filePath) ? 
        nsIFilePicker::returnCancel : nsIFilePicker::returnOK;
    if (*_retval == nsIFilePicker::returnOK)
        mFilePath.Assign(filePath);
    return NS_OK;
}
