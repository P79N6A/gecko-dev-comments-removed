






































#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "EmbedFilePicker.h"
#include "EmbedGtkTools.h"
#include "gtkmozembed.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsIURI.h"
#include "nsIFileURL.h"
#include "nsILocalFile.h"
#include "nsIDOMWindow.h"
#include "nsNetCID.h"
#include "nsIDOMWindowInternal.h"
#ifndef MOZILLA_INTERNAL_API
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#endif
#ifdef MOZ_LOGGING
#include <stdlib.h>
#endif

NS_IMPL_ISUPPORTS1(EmbedFilePicker, nsIFilePicker)
EmbedFilePicker::EmbedFilePicker()
: mParent (nsnull),
  mMode(nsIFilePicker::modeOpen),
  mFilename (nsnull)
{
}

EmbedFilePicker::~EmbedFilePicker()
{
  if (mFilename)
    g_free (mFilename);
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
  if (!mFilename) return NS_OK;
  


  gchar *strippedFileName = nsnull;
  if (!strncmp (mFilename, GTK_MOZ_EMBED_COMMON_FILE_SCHEME, 7))
    strippedFileName = (g_strsplit(mFilename, GTK_MOZ_EMBED_COMMON_FILE_SCHEME, -1))[1];
  else if (!strncmp (mFilename, GTK_MOZ_EMBED_BLUETOOTH_FILE_SCHEME, 7))
    strippedFileName = (g_strsplit(mFilename, GTK_MOZ_EMBED_BLUETOOTH_FILE_SCHEME, -1))[1];
  else {
    if (!mParent) return NS_OK;
    GtkWidget* parentWidget = GetGtkWidgetForDOMWindow (mParent);
    if (!parentWidget) return NS_OK;
    g_signal_emit_by_name(GTK_OBJECT (parentWidget), "alert", "File protocol not supported." ,NULL);
    
    return NS_OK;
  }
  if (strippedFileName)
  {
    nsCAutoString localSavePath (strippedFileName);
    nsCOMPtr<nsILocalFile> file = do_CreateInstance (NS_LOCAL_FILE_CONTRACTID);
    if (!file) return NS_OK;
    file->InitWithNativePath (localSavePath);
    NS_ADDREF (*aFile = file);
    g_free (strippedFileName);
    strippedFileName = nsnull;
  }
  return NS_OK;
}


NS_IMETHODIMP EmbedFilePicker::GetFileURL(nsIFileURL **aFileURL)
{
  nsCOMPtr<nsILocalFile> file;
  GetFile (getter_AddRefs(file));
  if (!file) return NS_OK;
  NS_ENSURE_TRUE (file, NS_ERROR_FAILURE);
  nsCOMPtr<nsIFileURL> fileURL = do_CreateInstance (NS_STANDARDURL_CONTRACTID);
  if (!fileURL) return NS_OK;
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
  if (!mParent)
    return NS_OK;
  GtkWidget* parentWidget = GetGtkWidgetForDOMWindow (mParent);
  if (!parentWidget)
    return NS_OK;
  if (mFilename) {
    g_free (mFilename);
    mFilename = nsnull;
  }
  int response;
  g_signal_emit_by_name (
    GTK_OBJECT (parentWidget),
    "upload_dialog",
    "/home/user", 
    "",
    &mFilename,
    &response,
    NULL);
  if (response == 1 && mFilename != NULL)
    *_retval = nsIFilePicker::returnOK;
  else
    *_retval = nsIFilePicker::returnCancel;
  return NS_OK;
}
