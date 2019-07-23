








































#include "EmbedDownloadMgr.h"
#include "EmbedGtkTools.h"
#ifdef MOZILLA_INTERNAL_API
#include "nsXPIDLString.h"
#else
#include "nsComponentManagerUtils.h"
#endif
#include "nsIChannel.h"
#include "nsIWebProgress.h"
#include "nsIDOMWindow.h"
#include "nsIURI.h"
#include "nsCRT.h"
#include "nsIPromptService.h"
#include "nsIWebProgressListener2.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIURI.h"
#include "nsIURL.h"
#include "nsIFile.h"
#include "nsIDOMWindow.h"
#include "nsIExternalHelperAppService.h"
#include "nsCExternalHandlerService.h"
#include "nsMemory.h"
#include "nsNetError.h"
#include "nsIStreamListener.h"
#include "nsIFile.h"
#include "nsILocalFile.h"
#include "nsNetCID.h"
#include <unistd.h>
#include <gtkmozembed_download.h>
class EmbedDownloadMgr;
class ProgressListener : public nsIWebProgressListener2
{
public:
    ProgressListener(EmbedDownload *aDownload, nsCAutoString aFilename, nsISupports *aContext) : mFilename (aFilename)
    {
        mDownload = aDownload;
        mContext = aContext;
    };
    ~ProgressListener()
    {
    };
    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBPROGRESSLISTENER
    NS_DECL_NSIWEBPROGRESSLISTENER2
    EmbedDownload *mDownload;
    nsISupports *mContext;            
    nsCOMPtr<nsILocalFile> mDestFile;
    nsCAutoString mFilename;
    nsCAutoString mLocalSaveFileName;
};

NS_IMPL_ISUPPORTS2(ProgressListener, nsIWebProgressListener2, nsIWebProgressListener)
NS_IMPL_ISUPPORTS1(EmbedDownloadMgr, nsIHelperAppLauncherDialog)

EmbedDownloadMgr::EmbedDownloadMgr(void)
{
}

EmbedDownloadMgr::~EmbedDownloadMgr(void)
{
}

nsresult
EmbedDownloadMgr::Init()
{
  return NS_OK;
}

NS_IMETHODIMP
EmbedDownloadMgr::Show(nsIHelperAppLauncher *aLauncher, nsISupports *aContext, PRUint32 aForced)
{
  nsresult rv;
  mContext = aContext;
  mLauncher = aLauncher;
  rv = GetDownloadInfo();
  return NS_OK;
}

NS_METHOD EmbedDownloadMgr::GetDownloadInfo (void)
{
  nsresult rv;
  
  GtkObject* instance = gtk_moz_embed_download_new ();
  EmbedDownload *download = (EmbedDownload *) GTK_MOZ_EMBED_DOWNLOAD(instance)->data;
  
  rv = mLauncher->GetMIMEInfo (getter_AddRefs(mMIMEInfo));
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
  nsCAutoString aMimeType;
  rv = mMIMEInfo->GetMIMEType (aMimeType);
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
  
  nsCAutoString aTempFileName;
  nsAutoString aSuggestedFileName;
  rv = mLauncher->GetSuggestedFileName (aSuggestedFileName);
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
  aTempFileName = NS_ConvertUTF16toUTF8 (aSuggestedFileName);
  
  rv = mLauncher->GetSource (getter_AddRefs(mUri));
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
  rv = mUri->Resolve(NS_LITERAL_CSTRING("."), mSpec);
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
  rv = mLauncher->GetTargetFile(getter_AddRefs(mDestFileTemp));
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
  download->file_target = mDestFileTemp;
  
  nsCOMPtr<nsIWebProgressListener2> listener = new ProgressListener(download, aTempFileName, mContext);
  rv = mLauncher->SetWebProgressListener(listener);
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
  
  download->parent = instance;
  download->started = 0;
  download->downloaded_size = -1;
  download->launcher = mLauncher;
  download->file_name = g_strdup ((gchar *) aTempFileName.get());
  download->server = g_strconcat(mSpec.get(), (gchar *) download->file_name, NULL);
  download->file_type = g_strdup (aMimeType.get());
  return NS_OK;
}


NS_IMETHODIMP EmbedDownloadMgr::PromptForSaveToFile (nsIHelperAppLauncher *aLauncher,
                                                        nsISupports *aWindowContext,
                                                        const PRUnichar *aDefaultFile,
                                                        const PRUnichar *aSuggestedFileExtension,
                                                        nsILocalFile **_retval)
{
  return NS_OK;
}



NS_IMETHODIMP ProgressListener::OnStatusChange (nsIWebProgress *aWebProgress,
                                                    nsIRequest *aRequest, nsresult aStatus,
                                                    const PRUnichar *aMessage)
{
  if (NS_SUCCEEDED (aStatus))
    return NS_OK;
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP ProgressListener::OnStateChange (nsIWebProgress *aWebProgress,
                                                    nsIRequest *aRequest, PRUint32 aStateFlags,
                                                    nsresult aStatus)
{
  if (NS_SUCCEEDED (aStatus))
    return NS_OK;
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP ProgressListener::OnProgressChange (nsIWebProgress *aWebProgress,
                                                    nsIRequest *aRequest, PRInt32 aCurSelfProgress,
                                                    PRInt32 aMaxSelfProgress, PRInt32 aCurTotalProgress,
                                                    PRInt32 aMaxTotalProgress)
{
  return OnProgressChange64 (aWebProgress, aRequest, aCurSelfProgress, aMaxSelfProgress, aCurTotalProgress, aMaxTotalProgress);
}

NS_IMETHODIMP ProgressListener::OnLocationChange (nsIWebProgress *aWebProgress,
                                                    nsIRequest *aRequest, nsIURI *location)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP ProgressListener::OnSecurityChange (nsIWebProgress *aWebProgress,
                                                    nsIRequest *aRequest, PRUint32 state)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP ProgressListener::OnProgressChange64 (nsIWebProgress *aWebProgress,
                                                    nsIRequest *aRequest, PRInt64 aCurSelfProgress,
                                                    PRInt64 aMaxSelfProgress, PRInt64 aCurTotalProgress,
                                                    PRInt64 aMaxTotalProgress)
{
  nsresult rv;

  if (!mDownload) return NS_OK;
  if (mDownload->started == 0) {
    mDownload->request = aRequest;
    mDownload->started = 1;
    
    mDownload->file_size = aMaxSelfProgress;
    nsCOMPtr<nsIDOMWindow> parentDOMWindow = do_GetInterface (mContext);
    mDownload->gtkMozEmbedParentWidget = GetGtkWidgetForDOMWindow(parentDOMWindow);
    if (mDownload->gtkMozEmbedParentWidget) {
      gtk_signal_emit(GTK_OBJECT(mDownload->gtkMozEmbedParentWidget),
                                    moz_embed_signals[DOWNLOAD_REQUEST],
                                    mDownload->server,
                                    mDownload->file_name,
                                    mDownload->file_type,
                                    (gulong) mDownload->file_size,
                                    1);
    }
  }
  if (mDownload->started == 1) {
    
    gtk_signal_emit(GTK_OBJECT(mDownload->parent),
                    moz_embed_download_signals[DOWNLOAD_STARTED_SIGNAL], &mDownload->file_name_with_path);
    
    if (!mDownload->file_name_with_path) {
      gtk_moz_embed_download_do_command (GTK_MOZ_EMBED_DOWNLOAD (mDownload->parent), GTK_MOZ_EMBED_DOWNLOAD_CANCEL);
      return NS_OK;
    }
    
    gchar *localUrl = nsnull, *localFileName = nsnull;
    
    if (g_str_has_prefix (mDownload->file_name_with_path, FILE_SCHEME)) {
      
      gchar *localUrlWithFileName = (g_strsplit (mDownload->file_name_with_path, FILE_SCHEME, -1))[1];
      gint i;
      gchar **localUrlSplitted = (char **) (g_strsplit(localUrlWithFileName, SLASH, -1));
      for(i = 0; localUrlSplitted[i]; i++);
          localFileName = localUrlSplitted[i-1];
      localUrl = (gchar *) (g_strsplit(localUrlWithFileName, localFileName, -1))[0];
    } else {
      
      localUrl = (char *) (g_strsplit (mDownload->file_name_with_path, mFilename.get(), -1))[0];
      localFileName = g_strdup ((gchar *) mFilename.get());
    }
    nsCAutoString localSavePath;
    if (localUrl) {
      localSavePath.Assign(localUrl);
      g_free (localUrl);
      localUrl = nsnull;
    }
    if (localFileName) {
      mLocalSaveFileName.Assign(localFileName);
      g_free (localFileName);
      localFileName = nsnull;
    }
    
    mDestFile = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID);
    mDestFile->InitWithNativePath(localSavePath);
    mDestFile->Create(nsIFile::NORMAL_FILE_TYPE, 0777);
    mDownload->started = 2;
  }
  
  
  if (aCurSelfProgress == aMaxSelfProgress) {
    
    gtk_signal_emit(GTK_OBJECT(mDownload->parent),
                    moz_embed_download_signals[DOWNLOAD_COMPLETED_SIGNAL]);
  } else {
    
    gtk_signal_emit(GTK_OBJECT(mDownload->parent),
                    moz_embed_download_signals[DOWNLOAD_PROGRESS_SIGNAL],
                    (gulong) aCurSelfProgress, (gulong) aMaxSelfProgress, 1);
  }
  
  mDownload->downloaded_size = (gulong) aCurSelfProgress;
  
  rv = mDownload->file_target->MoveToNative (mDestFile, mLocalSaveFileName);
  return NS_OK;
}

NS_IMETHODIMP ProgressListener::OnRefreshAttempted(nsIWebProgress *aWebProgress,
                                                   nsIURI *aUri, PRInt32 aDelay,
                                                   PRBool aSameUri,
                                                   PRBool *allowRefresh)
{
  *allowRefresh = PR_TRUE;
  return NS_OK;
}
