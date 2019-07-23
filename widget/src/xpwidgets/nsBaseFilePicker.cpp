







































#include "nsCOMPtr.h"
#include "nsPIDOMWindow.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIBaseWindow.h"
#include "nsIContentViewer.h"
#include "nsIDocumentViewer.h"
#include "nsIWidget.h"

#include "nsIStringBundle.h"
#include "nsXPIDLString.h"
#include "nsIServiceManager.h"
#include "nsCOMArray.h"
#include "nsILocalFile.h"
#include "nsEnumeratorUtils.h"

#include "nsBaseFilePicker.h"

#define FILEPICKER_TITLES "chrome://global/locale/filepicker.properties"
#define FILEPICKER_FILTERS "chrome://global/content/filepicker.properties"

nsBaseFilePicker::nsBaseFilePicker()
{

}

nsBaseFilePicker::~nsBaseFilePicker()
{

}



nsIWidget *nsBaseFilePicker::DOMWindowToWidget(nsIDOMWindow *dw)
{
  nsCOMPtr<nsIWidget> widget;

  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(dw);
  if (window) {
    nsCOMPtr<nsIBaseWindow> baseWin(do_QueryInterface(window->GetDocShell()));

    while (!widget && baseWin) {
      baseWin->GetParentWidget(getter_AddRefs(widget));
      if (!widget) {
        nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(baseWin));
        if (!docShellAsItem)
          return nsnull;

        nsCOMPtr<nsIDocShellTreeItem> parent;
        docShellAsItem->GetSameTypeParent(getter_AddRefs(parent));

        window = do_GetInterface(parent);
        if (!window)
          return nsnull;

        baseWin = do_QueryInterface(window->GetDocShell());
      }
    }
  }

  
  
  
  return widget.get();
}


NS_IMETHODIMP nsBaseFilePicker::Init(nsIDOMWindow *aParent,
                                     const nsAString& aTitle,
                                     PRInt16 aMode)
{
  NS_PRECONDITION(aParent, "Null parent passed to filepicker, no file "
                  "picker for you!");
  nsIWidget *widget = DOMWindowToWidget(aParent);
  NS_ENSURE_TRUE(widget, NS_ERROR_FAILURE);

  InitNative(widget, aTitle, aMode);

  return NS_OK;
}


NS_IMETHODIMP
nsBaseFilePicker::AppendFilters(PRInt32 aFilterMask)
{
  nsresult rv;
  nsCOMPtr<nsIStringBundleService> stringService = do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIStringBundle> titleBundle, filterBundle;

  rv = stringService->CreateBundle(FILEPICKER_TITLES, getter_AddRefs(titleBundle));
  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;

  rv = stringService->CreateBundle(FILEPICKER_FILTERS, getter_AddRefs(filterBundle));
  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;

  nsXPIDLString title;
  nsXPIDLString filter;

  if (aFilterMask & filterAll) {
    titleBundle->GetStringFromName(NS_LITERAL_STRING("allTitle").get(), getter_Copies(title));
    filterBundle->GetStringFromName(NS_LITERAL_STRING("allFilter").get(), getter_Copies(filter));
    AppendFilter(title,filter);
  }
  if (aFilterMask & filterHTML) {
    titleBundle->GetStringFromName(NS_LITERAL_STRING("htmlTitle").get(), getter_Copies(title));
    filterBundle->GetStringFromName(NS_LITERAL_STRING("htmlFilter").get(), getter_Copies(filter));
    AppendFilter(title,filter);
  }
  if (aFilterMask & filterText) {
    titleBundle->GetStringFromName(NS_LITERAL_STRING("textTitle").get(), getter_Copies(title));
    filterBundle->GetStringFromName(NS_LITERAL_STRING("textFilter").get(), getter_Copies(filter));
    AppendFilter(title,filter);
  }
  if (aFilterMask & filterImages) {
    titleBundle->GetStringFromName(NS_LITERAL_STRING("imageTitle").get(), getter_Copies(title));
    filterBundle->GetStringFromName(NS_LITERAL_STRING("imageFilter").get(), getter_Copies(filter));
    AppendFilter(title,filter);
  }
  if (aFilterMask & filterXML) {
    titleBundle->GetStringFromName(NS_LITERAL_STRING("xmlTitle").get(), getter_Copies(title));
    filterBundle->GetStringFromName(NS_LITERAL_STRING("xmlFilter").get(), getter_Copies(filter));
    AppendFilter(title,filter);
  }
  if (aFilterMask & filterXUL) {
    titleBundle->GetStringFromName(NS_LITERAL_STRING("xulTitle").get(), getter_Copies(title));
    filterBundle->GetStringFromName(NS_LITERAL_STRING("xulFilter").get(), getter_Copies(filter));
    AppendFilter(title, filter);
  }
  if (aFilterMask & filterApps) {
    titleBundle->GetStringFromName(NS_LITERAL_STRING("appsTitle").get(), getter_Copies(title));
    
    
    AppendFilter(title, NS_LITERAL_STRING("..apps"));
  }
  return NS_OK;
}






NS_IMETHODIMP nsBaseFilePicker::GetFilterIndex(PRInt32 *aFilterIndex)
{
  *aFilterIndex = 0;
  return NS_OK;
}

NS_IMETHODIMP nsBaseFilePicker::SetFilterIndex(PRInt32 aFilterIndex)
{
  return NS_OK;
}

NS_IMETHODIMP nsBaseFilePicker::GetFiles(nsISimpleEnumerator **aFiles)
{
  NS_ENSURE_ARG_POINTER(aFiles);
  nsCOMArray <nsILocalFile> files;
  nsresult rv;

  
  
  
  nsCOMPtr <nsILocalFile> file;
  rv = GetFile(getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv,rv);

  rv = files.AppendObject(file);
  NS_ENSURE_SUCCESS(rv,rv);

  return NS_NewArrayEnumerator(aFiles, files);
}

#ifdef BASEFILEPICKER_HAS_DISPLAYDIRECTORY





NS_IMETHODIMP nsBaseFilePicker::SetDisplayDirectory(nsILocalFile *aDirectory)
{
  if (!aDirectory) {
    mDisplayDirectory = nsnull;
    return NS_OK;
  }
  nsCOMPtr<nsIFile> directory;
  nsresult rv = aDirectory->Clone(getter_AddRefs(directory));
  if (NS_FAILED(rv))
    return rv;
  mDisplayDirectory = do_QueryInterface(directory, &rv);
  return rv;
}






NS_IMETHODIMP nsBaseFilePicker::GetDisplayDirectory(nsILocalFile **aDirectory)
{
  *aDirectory = nsnull;
  if (!mDisplayDirectory)
    return NS_OK;
  nsCOMPtr<nsIFile> directory;
  nsresult rv = mDisplayDirectory->Clone(getter_AddRefs(directory));
  if (NS_FAILED(rv))
    return rv;
  return CallQueryInterface(directory, aDirectory);
}
#endif
