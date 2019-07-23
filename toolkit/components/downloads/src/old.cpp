  








  void openProgressDialogFor(in wstring aPersistentDescriptor, in nsIDOMWindow aParent);

NS_IMETHODIMP
nsDownloadManager::OpenProgressDialogFor(const PRUnichar* aPath, nsIDOMWindow* aParent)
{
  nsresult rv;
  nsStringKey key(aPath);
  if (!mCurrDownloads.Exists(&key))
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDownload> download;
  nsDownload* internalDownload = static_cast<nsDownload*>(mCurrDownloads.Get(&key));
  internalDownload->QueryInterface(NS_GET_IID(nsIDownload), (void**) getter_AddRefs(download));
  if (!download)
    return NS_ERROR_FAILURE;
 

  nsCOMPtr<nsIProgressDialog> oldDialog;
  internalDownload->GetDialog(getter_AddRefs(oldDialog));
  
  if (oldDialog) {
    nsCOMPtr<nsIDOMWindow> window;
    oldDialog->GetDialog(getter_AddRefs(window));
    if (window) {
      nsCOMPtr<nsIDOMWindowInternal> internalWin = do_QueryInterface(window);
      internalWin->Focus();
      return NS_OK;
    }
  }

  nsCOMPtr<nsIProgressDialog> dialog(do_CreateInstance("@mozilla.org/progressdialog;1", &rv));
  if (NS_FAILED(rv)) return rv;
  
  nsCOMPtr<nsIDownload> dl = do_QueryInterface(dialog);

  
  
  
  PRInt64 startTime = 0;
  download->GetStartTime(&startTime);
  
  
  nsCOMPtr<nsIURI> source;
  download->GetSource(getter_AddRefs(source));

  
  nsCOMPtr<nsILocalFile> target;
  download->GetTarget(getter_AddRefs(target));
  
  
  nsCOMPtr<nsIMIMEInfo> mimeInfo;
  download->GetMIMEInfo(getter_AddRefs(mimeInfo));

  dl->Init(source, target, nsnull, mimeInfo, startTime, nsnull); 
  dl->SetObserver(this);

  
  nsCOMPtr<nsIWebProgressListener> listener = do_QueryInterface(dialog);
  internalDownload->SetDialogListener(listener);
  
  internalDownload->SetDialog(dialog);
  
  return dialog->Open(aParent);
}

