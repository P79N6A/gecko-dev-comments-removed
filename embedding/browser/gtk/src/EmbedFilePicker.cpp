








































#include "EmbedFilePicker.h"
#include "EmbedGtkTools.h"
#include "nsIFileURL.h"
#include "nsILocalFile.h"
#include "nsIDOMWindow.h"
#include "nsStringGlue.h"
#include "nsNetUtil.h"
#include "prenv.h"

#ifdef MOZ_LOGGING
#include <stdlib.h>
#endif

NS_IMPL_ISUPPORTS1(EmbedFilePicker, nsIFilePicker)

EmbedFilePicker::EmbedFilePicker(): mParent (nsnull),
                                    mMode(nsIFilePicker::modeOpen)
{
}

EmbedFilePicker::~EmbedFilePicker()
{
}


NS_IMETHODIMP EmbedFilePicker::Init(nsIDOMWindow *parent, const nsAString &title, PRInt16 mode)
{
  mParent = parent;
  mMode = mode;
  return NS_OK;
}


NS_IMETHODIMP EmbedFilePicker::AppendFilters(PRInt32 filterMask)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP EmbedFilePicker::AppendFilter(const nsAString &title, const nsAString &filter)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP EmbedFilePicker::GetDefaultString(nsAString &aDefaultString)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP EmbedFilePicker::SetDefaultString(const nsAString &aDefaultString)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP EmbedFilePicker::GetDefaultExtension(nsAString &aDefaultExtension)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP EmbedFilePicker::SetDefaultExtension(const nsAString &aDefaultExtension)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP EmbedFilePicker::GetFilterIndex(PRInt32 *aFilterIndex)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP EmbedFilePicker::SetFilterIndex(PRInt32 aFilterIndex)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP EmbedFilePicker::GetDisplayDirectory(nsILocalFile **aDisplayDirectory)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP EmbedFilePicker::SetDisplayDirectory(nsILocalFile *aDisplayDirectory)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP EmbedFilePicker::GetFile(nsILocalFile **aFile)
{

  NS_ENSURE_ARG_POINTER(aFile);

  if (mFilename.IsEmpty())
    return NS_OK;

  nsCOMPtr<nsIURI> baseURI;
  nsresult rv = NS_NewURI(getter_AddRefs(baseURI), mFilename);

  nsCOMPtr<nsIFileURL> fileURL(do_QueryInterface(baseURI, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> file;
  rv = fileURL->GetFile(getter_AddRefs(file));

  nsCOMPtr<nsILocalFile> localfile;
  localfile = do_QueryInterface(file, &rv);

  if (NS_SUCCEEDED(rv)) {
    NS_ADDREF(*aFile = localfile);
    return NS_OK;
  }

  NS_ENSURE_TRUE(mParent, NS_OK);
  GtkWidget* parentWidget = GetGtkWidgetForDOMWindow(mParent);
  NS_ENSURE_TRUE(parentWidget, NS_OK);
  
  g_signal_emit_by_name(GTK_OBJECT(parentWidget), "alert", "File protocol not supported.", NULL);
  return NS_OK;
}


NS_IMETHODIMP EmbedFilePicker::GetFileURL(nsIFileURL **aFileURL)
{
  NS_ENSURE_ARG_POINTER(aFileURL);
  *aFileURL = nsnull;

  nsCOMPtr<nsILocalFile> file;
  GetFile(getter_AddRefs(file));
  NS_ENSURE_TRUE(file, NS_ERROR_FAILURE);
  nsCOMPtr<nsIFileURL> fileURL = do_CreateInstance(NS_STANDARDURL_CONTRACTID);
  NS_ENSURE_TRUE(fileURL, NS_ERROR_OUT_OF_MEMORY);
  fileURL->SetFile(file);
  NS_ADDREF(*aFileURL = fileURL);
  return NS_OK;
}


NS_IMETHODIMP EmbedFilePicker::GetFiles(nsISimpleEnumerator * *aFiles)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP EmbedFilePicker::Show(PRInt16 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE(mParent, NS_OK);

  GtkWidget *parentWidget = GetGtkWidgetForDOMWindow(mParent);
  NS_ENSURE_TRUE(parentWidget, NS_OK);

  gboolean response = 0;
  char *retname = nsnull;
  g_signal_emit_by_name(GTK_OBJECT(parentWidget),
                        "upload_dialog",
                        PR_GetEnv("HOME"),
                        "",
                        &retname,
                        &response,
                        NULL);

  *_retval = response ? nsIFilePicker::returnOK : nsIFilePicker::returnCancel;

  mFilename = retname;
  if (retname)
    NS_Free(retname);

  return NS_OK;
}
