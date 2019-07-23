





































 
#include "nsDownloadManager.h"
#include "nsIWebProgress.h"
#include "nsIRDFLiteral.h"
#include "rdf.h"
#include "nsNetUtil.h"
#include "nsIURI.h"
#include "nsIURL.h"
#include "nsIDOMChromeWindow.h"
#include "nsIDOMWindow.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDOMEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsRDFCID.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsIWebBrowserPersist.h"
#include "nsIObserver.h"
#include "nsIProgressDialog.h"
#include "nsIWebBrowserPersist.h"
#include "nsIWindowWatcher.h"
#include "nsIStringBundle.h"
#include "nsCRT.h"
#include "nsIWindowMediator.h"
#include "nsIPromptService.h"
#include "nsIObserverService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsVoidArray.h"
#include "nsEnumeratorUtils.h"
#include "nsIFileURL.h"
#include "nsEmbedCID.h"
#include "nsInt64.h"
#include "nsAutoPtr.h"

#ifdef XP_WIN
#include <shlobj.h>
#endif





static NS_DEFINE_CID(kRDFServiceCID, NS_RDFSERVICE_CID);
static PRBool gStoppingDownloads = PR_FALSE;

#define DOWNLOAD_MANAGER_FE_URL "chrome://mozapps/content/downloads/downloads.xul"
#define DOWNLOAD_MANAGER_BUNDLE "chrome://mozapps/locale/downloads/downloads.properties"
#define DOWNLOAD_MANAGER_ALERT_ICON "chrome://mozapps/skin/downloads/downloadIcon.png"
#define PREF_BDM_SHOWALERTONCOMPLETE "browser.download.manager.showAlertOnComplete"
#define PREF_BDM_SHOWALERTINTERVAL "browser.download.manager.showAlertInterval"
#define PREF_BDM_RETENTION "browser.download.manager.retention"
#define PREF_BDM_OPENDELAY "browser.download.manager.openDelay"
#define PREF_BDM_SHOWWHENSTARTING "browser.download.manager.showWhenStarting"
#define PREF_BDM_FOCUSWHENSTARTING "browser.download.manager.focusWhenStarting"
#define PREF_BDM_CLOSEWHENDONE "browser.download.manager.closeWhenDone"
#define PREF_BDM_FLASHCOUNT "browser.download.manager.flashCount"
#define PREF_BDM_ADDTORECENTDOCS "browser.download.manager.addToRecentDocs"

static const nsInt64 gInterval((PRUint32)(400 * PR_USEC_PER_MSEC));

static nsIRDFResource* gNC_DownloadsRoot = nsnull;
static nsIRDFResource* gNC_File = nsnull;
static nsIRDFResource* gNC_URL = nsnull;
static nsIRDFResource* gNC_IconURL = nsnull;
static nsIRDFResource* gNC_Name = nsnull;
static nsIRDFResource* gNC_ProgressPercent = nsnull;
static nsIRDFResource* gNC_Transferred = nsnull;
static nsIRDFResource* gNC_DownloadState = nsnull;
static nsIRDFResource* gNC_StatusText = nsnull;
static nsIRDFResource* gNC_DateStarted = nsnull;
static nsIRDFResource* gNC_DateEnded = nsnull;

static nsIRDFService* gRDFService = nsnull;
static nsIObserverService* gObserverService = nsnull;
static PRInt32 gRefCnt = 0;







static nsresult
GetFilePathFromURI(nsIURI *aURI, nsAString &aPath)
{
  nsresult rv;

  nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(aURI, &rv);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIFile> file;
  rv = fileURL->GetFile(getter_AddRefs(file));
  if (NS_SUCCEEDED(rv))
    rv = file->GetPath(aPath);

  return rv;
}




NS_IMPL_ISUPPORTS3(nsDownloadManager, nsIDownloadManager, nsIXPInstallManagerUI, nsIObserver)

nsDownloadManager::nsDownloadManager() : mBatches(0)
{
}

nsDownloadManager::~nsDownloadManager()
{
  if (--gRefCnt != 0 || !gRDFService  || !gObserverService)
    
    
    
    return;

  gRDFService->UnregisterDataSource(mDataSource);

#if 0
  
  
  gObserverService->RemoveObserver(this, "quit-application");
  gObserverService->RemoveObserver(this, "quit-application-requested");
  gObserverService->RemoveObserver(this, "offline-requested");
#endif

  NS_IF_RELEASE(gNC_DownloadsRoot);                                             
  NS_IF_RELEASE(gNC_File);                                                      
  NS_IF_RELEASE(gNC_URL);
  NS_IF_RELEASE(gNC_IconURL);
  NS_IF_RELEASE(gNC_Name);                                                      
  NS_IF_RELEASE(gNC_ProgressPercent);
  NS_IF_RELEASE(gNC_Transferred);
  NS_IF_RELEASE(gNC_DownloadState);
  NS_IF_RELEASE(gNC_StatusText);
  NS_IF_RELEASE(gNC_DateStarted);
  NS_IF_RELEASE(gNC_DateEnded);

  NS_RELEASE(gRDFService);
  NS_RELEASE(gObserverService);
}

PRInt32 PR_CALLBACK nsDownloadManager::CancelAllDownloads(nsHashKey* aKey, void* aData, void* aClosure)
{
  nsStringKey* key = (nsStringKey*)aKey;
  nsresult rv;

  nsCOMPtr<nsIDownloadManager> manager = do_QueryInterface((nsISupports*)aClosure, &rv);
  if (NS_FAILED(rv)) return kHashEnumerateRemove;  
  
  if (IsInProgress(NS_STATIC_CAST(nsDownload*, aData)->GetDownloadState()))  
    manager->CancelDownload(key->GetString());
  else
    NS_STATIC_CAST(nsDownloadManager*, aClosure)->DownloadEnded(key->GetString(), nsnull);
 
  return kHashEnumerateRemove;
}

nsresult
nsDownloadManager::Init()
{
  if (gRefCnt++ != 0) {
    NS_NOTREACHED("download manager should be used as a service");
    return NS_ERROR_UNEXPECTED; 
  }

  nsresult rv;
  mRDFContainerUtils = do_GetService("@mozilla.org/rdf/container-utils;1", &rv);
  if (NS_FAILED(rv)) return rv;

  rv = CallGetService("@mozilla.org/observer-service;1", &gObserverService);
  if (NS_FAILED(rv)) return rv;
  
  rv = CallGetService(kRDFServiceCID, &gRDFService);
  if (NS_FAILED(rv)) return rv;                                                 

  gRDFService->GetResource(NS_LITERAL_CSTRING("NC:DownloadsRoot"), &gNC_DownloadsRoot);
  gRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "File"), &gNC_File);
  gRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "URL"), &gNC_URL);
  gRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "IconURL"), &gNC_IconURL);
  gRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "Name"), &gNC_Name);
  gRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "ProgressPercent"), &gNC_ProgressPercent);
  gRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "Transferred"), &gNC_Transferred);
  gRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "DownloadState"), &gNC_DownloadState);
  gRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "StatusText"), &gNC_StatusText);
  gRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "DateStarted"), &gNC_DateStarted);
  gRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "DateEnded"), &gNC_DateEnded);

  mDataSource = new nsDownloadsDataSource();
  if (!mDataSource)
    return NS_ERROR_OUT_OF_MEMORY;

  rv = NS_STATIC_CAST(nsDownloadsDataSource*, (nsIRDFDataSource*)mDataSource.get())->LoadDataSource();
  if (NS_FAILED(rv)) {
    mDataSource = nsnull; 
    return rv;
  }

  nsCOMPtr<nsIStringBundleService> bundleService = do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;
  
  rv = bundleService->CreateBundle(DOWNLOAD_MANAGER_BUNDLE, getter_AddRefs(mBundle));
  if (NS_FAILED(rv))
    return rv;

  
  
  
  
  
  
  
  
  
  
  gObserverService->AddObserver(this, "quit-application", PR_FALSE);
  gObserverService->AddObserver(this, "quit-application-requested", PR_FALSE);
  gObserverService->AddObserver(this, "offline-requested", PR_FALSE);

  return NS_OK;
}

PRInt32 
nsDownloadManager::GetRetentionBehavior()
{
  PRInt32 val = 0;
  nsCOMPtr<nsIPrefBranch> pref(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (pref) {
    nsresult rv = pref->GetIntPref(PREF_BDM_RETENTION, &val);
    if (NS_FAILED(rv))
      val = 0; 
  }

  return val;
}

nsresult
nsDownloadManager::DownloadStarted(const PRUnichar* aPath)
{
  nsStringKey key(aPath);
  if (mCurrDownloads.Exists(&key)) {
  
    
    nsCOMPtr<nsIRDFDate> dateLiteral;
    if (NS_SUCCEEDED(gRDFService->GetDateLiteral(PR_Now(), getter_AddRefs(dateLiteral)))) {    
      nsCOMPtr<nsIRDFResource> res;
      nsCOMPtr<nsIRDFNode> node;
      
      gRDFService->GetUnicodeResource(nsDependentString(aPath), getter_AddRefs(res));
      
      mDataSource->GetTarget(res, gNC_DateStarted, PR_TRUE, getter_AddRefs(node));
      if (node)
        mDataSource->Change(res, gNC_DateStarted, node, dateLiteral);
      else
        mDataSource->Assert(res, gNC_DateStarted, dateLiteral, PR_TRUE);
    }
  
    AssertProgressInfoFor(aPath);
  }

  return NS_OK;
}

nsresult
nsDownloadManager::DownloadEnded(const PRUnichar* aPath, const PRUnichar* aMessage)
{
  nsStringKey key(aPath);
  if (mCurrDownloads.Exists(&key)) {

    
    nsCOMPtr<nsIRDFDate> dateLiteral;
    if (NS_SUCCEEDED(gRDFService->GetDateLiteral(PR_Now(), getter_AddRefs(dateLiteral)))) {    
      nsCOMPtr<nsIRDFResource> res;
      nsCOMPtr<nsIRDFNode> node;
      
      gRDFService->GetUnicodeResource(nsDependentString(aPath), getter_AddRefs(res));
      
      mDataSource->GetTarget(res, gNC_DateEnded, PR_TRUE, getter_AddRefs(node));
      if (node)
        mDataSource->Change(res, gNC_DateEnded, node, dateLiteral);
      else
        mDataSource->Assert(res, gNC_DateEnded, dateLiteral, PR_TRUE);
    }

    AssertProgressInfoFor(aPath);
    
    nsDownload* download = NS_STATIC_CAST(nsDownload*, mCurrDownloads.Get(&key));
    NS_RELEASE(download);

    if (!gStoppingDownloads)
      mCurrDownloads.Remove(&key);
  }

  return NS_OK;
}

nsresult
nsDownloadManager::GetDownloadsContainer(nsIRDFContainer** aResult)
{
  if (mDownloadsContainer) {
    *aResult = mDownloadsContainer;
    NS_ADDREF(*aResult);
    return NS_OK;
  }

  PRBool isContainer;
  nsresult rv = mRDFContainerUtils->IsContainer(mDataSource, gNC_DownloadsRoot, &isContainer);
  if (NS_FAILED(rv)) return rv;

  if (!isContainer) {
    rv = mRDFContainerUtils->MakeSeq(mDataSource, gNC_DownloadsRoot, getter_AddRefs(mDownloadsContainer));
    if (NS_FAILED(rv)) return rv;
  }
  else {
    mDownloadsContainer = do_CreateInstance(NS_RDF_CONTRACTID "/container;1", &rv);
    if (NS_FAILED(rv)) return rv;
    rv = mDownloadsContainer->Init(mDataSource, gNC_DownloadsRoot);
    if (NS_FAILED(rv)) return rv;
  }

  *aResult = mDownloadsContainer;
  NS_IF_ADDREF(*aResult);

  return rv;
}

nsresult
nsDownloadManager::GetInternalListener(nsIDownloadProgressListener** aListener)
{
  *aListener = mListener;
  NS_IF_ADDREF(*aListener);
  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::GetActiveDownloadCount(PRInt32* aResult)
{
  *aResult = mCurrDownloads.Count();
  
  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::GetActiveDownloads(nsISupportsArray** aResult)
{
  nsCOMPtr<nsISupportsArray> ary;
  NS_NewISupportsArray(getter_AddRefs(ary));
  mCurrDownloads.Enumerate(BuildActiveDownloadsList, (void*)ary);

  NS_ADDREF(*aResult = ary);

  return NS_OK;
}

PRInt32 PR_CALLBACK 
nsDownloadManager::BuildActiveDownloadsList(nsHashKey* aKey, void* aData, void* aClosure)
{
  nsCOMPtr<nsISupportsArray> ary(do_QueryInterface((nsISupports*)aClosure));
  nsCOMPtr<nsIDownload> dl(do_QueryInterface((nsISupports*)aData));  

  ary->AppendElement(dl);
 
  return kHashEnumerateNext;
}

NS_IMETHODIMP
nsDownloadManager::SaveState()
{
  nsCOMPtr<nsISupports> supports;
  nsCOMPtr<nsIRDFResource> res;
  nsCOMPtr<nsIRDFInt> intLiteral;

  DownloadState states[] = { nsIDownloadManager::DOWNLOAD_DOWNLOADING, 
                             nsIDownloadManager::DOWNLOAD_PAUSED,
                             nsIXPInstallManagerUI::INSTALL_DOWNLOADING,
                             nsIXPInstallManagerUI::INSTALL_INSTALLING };

  for (int i = 0; i < 4; ++i) {
    gRDFService->GetIntLiteral(states[i], getter_AddRefs(intLiteral));
    nsCOMPtr<nsISimpleEnumerator> downloads;
    nsresult rv = mDataSource->GetSources(gNC_DownloadState, intLiteral, PR_TRUE, getter_AddRefs(downloads));
    if (NS_FAILED(rv)) return rv;
  
    PRBool hasMoreElements;
    downloads->HasMoreElements(&hasMoreElements);

    while (hasMoreElements) {
      const char* uri;
      downloads->GetNext(getter_AddRefs(supports));
      res = do_QueryInterface(supports);
      res->GetValueConst(&uri);
      AssertProgressInfoFor(NS_ConvertASCIItoUTF16(uri).get());
      downloads->HasMoreElements(&hasMoreElements);
    }
  }

  return NS_OK;
}

nsresult
nsDownloadManager::AssertProgressInfoFor(const PRUnichar* aPath)
{
  nsStringKey key(aPath);
  if (!mCurrDownloads.Exists(&key))
    return NS_ERROR_FAILURE;
 
  nsDownload* internalDownload = NS_STATIC_CAST(nsDownload*, mCurrDownloads.Get(&key));
  if (!internalDownload)
    return NS_ERROR_FAILURE;
  
  nsresult rv;
  PRInt32 percentComplete;
  nsCOMPtr<nsIRDFNode> oldTarget;
  nsCOMPtr<nsIRDFInt> intLiteral;
  nsCOMPtr<nsIRDFResource> res;
  nsCOMPtr<nsIRDFLiteral> literal;

  gRDFService->GetUnicodeResource(nsDependentString(aPath), getter_AddRefs(res));

  DownloadState state = internalDownload->GetDownloadState();
 
  
  gRDFService->GetIntLiteral(state, getter_AddRefs(intLiteral));

  mDataSource->GetTarget(res, gNC_DownloadState, PR_TRUE, getter_AddRefs(oldTarget));
  
  if (oldTarget)
    rv = mDataSource->Change(res, gNC_DownloadState, oldTarget, intLiteral);
  else
    rv = mDataSource->Assert(res, gNC_DownloadState, intLiteral, PR_TRUE);
  if (NS_FAILED(rv)) return rv;

  
  internalDownload->GetPercentComplete(&percentComplete);

  mDataSource->GetTarget(res, gNC_ProgressPercent, PR_TRUE, getter_AddRefs(oldTarget));
  gRDFService->GetIntLiteral(percentComplete, getter_AddRefs(intLiteral));

  if (oldTarget)
    rv = mDataSource->Change(res, gNC_ProgressPercent, oldTarget, intLiteral);
  else
    rv = mDataSource->Assert(res, gNC_ProgressPercent, intLiteral, PR_TRUE);
  if (NS_FAILED(rv)) return rv;

  
  nsDownload::TransferInformation transferInfo =
                                 internalDownload->GetTransferInformation();

  
  PRInt64 current = (PRFloat64)transferInfo.mCurrBytes / 1024 + .5;
  PRInt64 max = (PRFloat64)transferInfo.mMaxBytes / 1024 + .5;
 
  nsAutoString currBytes; currBytes.AppendInt(current);
  nsAutoString maxBytes; maxBytes.AppendInt(max);
  const PRUnichar *strings[] = {
    currBytes.get(),
    maxBytes.get()
  };

  nsXPIDLString value; 
  rv = mBundle->FormatStringFromName(NS_LITERAL_STRING("transferred").get(),
                                     strings, 2, getter_Copies(value));
  if (NS_FAILED(rv)) return rv;

  gRDFService->GetLiteral(value, getter_AddRefs(literal));
 
  mDataSource->GetTarget(res, gNC_Transferred, PR_TRUE, getter_AddRefs(oldTarget));
 
  if (oldTarget)
    rv = mDataSource->Change(res, gNC_Transferred, oldTarget, literal);
  else
    rv = mDataSource->Assert(res, gNC_Transferred, literal, PR_TRUE);
  if (NS_FAILED(rv)) return rv;

  
  return Flush();
}  




NS_IMETHODIMP
nsDownloadManager::AddDownload(DownloadType aDownloadType, 
                               nsIURI* aSource,
                               nsIURI* aTarget,
                               const nsAString& aDisplayName,
                               const nsAString& aIconURL, 
                               nsIMIMEInfo *aMIMEInfo,
                               PRTime aStartTime,
                               nsILocalFile* aTempFile,
                               nsICancelable* aCancelable,
                               nsIDownload** aDownload)
{
  NS_ENSURE_ARG_POINTER(aSource);
  NS_ENSURE_ARG_POINTER(aTarget);
  NS_ENSURE_ARG_POINTER(aDownload);

  nsresult rv;

  
  nsCOMPtr<nsIFileURL> targetFileURL = do_QueryInterface(aTarget, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> targetFile;
  rv = targetFileURL->GetFile(getter_AddRefs(targetFile));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIRDFContainer> downloads;
  rv = GetDownloadsContainer(getter_AddRefs(downloads));
  if (NS_FAILED(rv)) return rv;

  nsDownload* internalDownload = new nsDownload();
  if (!internalDownload)
    return NS_ERROR_OUT_OF_MEMORY;

  internalDownload->QueryInterface(NS_GET_IID(nsIDownload), (void**) aDownload);
  if (!aDownload)
    return NS_ERROR_FAILURE;

  NS_ADDREF(*aDownload);

  
  internalDownload->SetDownloadManager(this);
  internalDownload->SetTarget(aTarget);
  internalDownload->SetSource(aSource);
  internalDownload->SetTempFile(aTempFile);

  
  
  
  
  
  

  nsAutoString path;
  rv = targetFile->GetPath(path);
  if (NS_FAILED(rv)) return rv;

  nsStringKey key(path);
  if (mCurrDownloads.Exists(&key))
    CancelDownload(path.get());

  nsCOMPtr<nsIRDFResource> downloadRes;
  gRDFService->GetUnicodeResource(path, getter_AddRefs(downloadRes));

  
  
  
  
  
  SaveState();

  
  
  PRInt32 itemIndex;
  nsCOMPtr<nsIRDFNode> node;
  downloads->IndexOf(downloadRes, &itemIndex);
  if (itemIndex > 0) {
    rv = downloads->RemoveElementAt(itemIndex, PR_TRUE, getter_AddRefs(node));
    if (NS_FAILED(rv)) return rv;
  }
  rv = downloads->InsertElementAt(downloadRes, 1, PR_TRUE);
  if (NS_FAILED(rv)) return rv;
  
  
  nsCAutoString spec;
  aSource->GetSpec(spec);

  nsCOMPtr<nsIRDFResource> urlResource;
  gRDFService->GetResource(spec, getter_AddRefs(urlResource));
  mDataSource->GetTarget(downloadRes, gNC_URL, PR_TRUE, getter_AddRefs(node));
  if (node)
    rv = mDataSource->Change(downloadRes, gNC_URL, node, urlResource);
  else
    rv = mDataSource->Assert(downloadRes, gNC_URL, urlResource, PR_TRUE);
  if (NS_FAILED(rv)) {
    downloads->IndexOf(downloadRes, &itemIndex);
    downloads->RemoveElementAt(itemIndex, PR_TRUE, getter_AddRefs(node));
    return rv;
  }

  
  nsAutoString displayName; displayName.Assign(aDisplayName);
  if (displayName.IsEmpty()) {
    targetFile->GetLeafName(displayName);
  }
  internalDownload->SetDisplayName(displayName.get());
 
  nsCOMPtr<nsIRDFLiteral> nameLiteral;
  gRDFService->GetLiteral(displayName.get(), getter_AddRefs(nameLiteral));
  mDataSource->GetTarget(downloadRes, gNC_Name, PR_TRUE, getter_AddRefs(node));
  if (node)
    rv = mDataSource->Change(downloadRes, gNC_Name, node, nameLiteral);
  else
    rv = mDataSource->Assert(downloadRes, gNC_Name, nameLiteral, PR_TRUE);
  if (NS_FAILED(rv)) {
    downloads->IndexOf(downloadRes, &itemIndex);
    downloads->RemoveElementAt(itemIndex, PR_TRUE, getter_AddRefs(node));
    return rv;
  }
  
  
  if (!aIconURL.IsEmpty()) {
    nsCOMPtr<nsIRDFResource> iconURIRes;
    gRDFService->GetUnicodeResource(aIconURL, getter_AddRefs(iconURIRes));
    mDataSource->GetTarget(downloadRes, gNC_IconURL, PR_TRUE, getter_AddRefs(node));
    if (node)
      rv = mDataSource->Change(downloadRes, gNC_IconURL, node, iconURIRes);
    else
      rv = mDataSource->Assert(downloadRes, gNC_IconURL, iconURIRes, PR_TRUE);
  }

  internalDownload->SetMIMEInfo(aMIMEInfo);
  internalDownload->SetStartTime(aStartTime);

  
  nsCOMPtr<nsIRDFResource> fileResource;
  gRDFService->GetUnicodeResource(path, getter_AddRefs(fileResource));
  rv = mDataSource->Assert(downloadRes, gNC_File, fileResource, PR_TRUE);
  if (NS_FAILED(rv)) {
    downloads->IndexOf(downloadRes, &itemIndex);
    downloads->RemoveElementAt(itemIndex, PR_TRUE, getter_AddRefs(node));
    return rv;
  }
  
  
  nsCOMPtr<nsIRDFInt> intLiteral;
  gRDFService->GetIntLiteral(nsIDownloadManager::DOWNLOAD_NOTSTARTED, getter_AddRefs(intLiteral));
  mDataSource->GetTarget(downloadRes, gNC_DownloadState, PR_TRUE, getter_AddRefs(node));
  if (node)
    rv = mDataSource->Change(downloadRes, gNC_DownloadState, node, intLiteral);
  else
    rv = mDataSource->Assert(downloadRes, gNC_DownloadState, intLiteral, PR_TRUE);
  if (NS_FAILED(rv)) {
    downloads->IndexOf(downloadRes, &itemIndex);
    downloads->RemoveElementAt(itemIndex, PR_TRUE, getter_AddRefs(node));
    return rv;
  }

  
  if (NS_FAILED(Flush())) {
    downloads->IndexOf(downloadRes, &itemIndex);
    downloads->RemoveElementAt(itemIndex, PR_TRUE, getter_AddRefs(node));
    return rv;
  }

  
  internalDownload->SetCancelable(aCancelable);

  
  
  if (aDownloadType == nsIXPInstallManagerUI::DOWNLOAD_TYPE_INSTALL) {
    if (!mXPIProgress)
      mXPIProgress = new nsXPIProgressListener(this);

    nsIXPIProgressDialog* dialog = mXPIProgress.get();
    nsXPIProgressListener* listener = NS_STATIC_CAST(nsXPIProgressListener*, dialog);
    listener->AddDownload(*aDownload);
  }

  mCurrDownloads.Put(&key, *aDownload);
  gObserverService->NotifyObservers(*aDownload, "dl-start", nsnull);

  return rv;
}

NS_IMETHODIMP
nsDownloadManager::GetDownload(const PRUnichar* aPath, nsIDownload** aDownloadItem)
{
  NS_ENSURE_ARG_POINTER(aDownloadItem);

  
  
  
  nsStringKey key(aPath);
  if (mCurrDownloads.Exists(&key)) {
    *aDownloadItem = NS_STATIC_CAST(nsIDownload*, mCurrDownloads.Get(&key));
    NS_ADDREF(*aDownloadItem);
    return NS_OK;
  }

  *aDownloadItem = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::CancelDownload(const PRUnichar* aPath)
{
  nsresult rv = NS_OK;
  nsStringKey key(aPath);
  if (!mCurrDownloads.Exists(&key))
    return RemoveDownload(aPath); 
  
  
  
  nsRefPtr<nsDownload> internalDownload =
      NS_STATIC_CAST(nsDownload*, mCurrDownloads.Get(&key));
  if (!internalDownload)
    return NS_ERROR_FAILURE;

  
  if (CompletedSuccessfully(internalDownload->mDownloadState))
    return NS_OK;

  internalDownload->SetDownloadState(nsIDownloadManager::DOWNLOAD_CANCELED);

  
  nsCOMPtr<nsICancelable> cancelable;
  internalDownload->GetCancelable(getter_AddRefs(cancelable));
  if (cancelable)
    cancelable->Cancel(NS_BINDING_ABORTED);

  DownloadEnded(aPath, nsnull);

  
  
  
  nsCOMPtr<nsILocalFile> tempFile;
  internalDownload->GetTempFile(getter_AddRefs(tempFile));
  if (tempFile) {
    PRBool exists;
    tempFile->Exists(&exists);
    if (exists)
      tempFile->Remove(PR_FALSE);
  }

  gObserverService->NotifyObservers(internalDownload, "dl-cancel", nsnull);

  
  
  nsCOMPtr<nsIProgressDialog> dialog;
  internalDownload->GetDialog(getter_AddRefs(dialog));
  if (dialog) {
    nsCOMPtr<nsIObserver> observer = do_QueryInterface(dialog);
    rv = observer->Observe(internalDownload, "oncancel", nsnull);
    if (NS_FAILED(rv)) return rv;
  }

  return rv;
}

NS_IMETHODIMP
nsDownloadManager::RemoveDownload(const PRUnichar* aPath)
{
  nsStringKey key(aPath);
  
  
  
  PRBool inProgress = mCurrDownloads.Exists(&key);
  NS_ASSERTION(!inProgress, "Can't call RemoveDownload on a download in progress!");
  if (inProgress)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIRDFResource> res;
  gRDFService->GetUnicodeResource(nsDependentString(aPath), getter_AddRefs(res));

  return RemoveDownload(res);
}





nsresult
nsDownloadManager::RemoveDownload(nsIRDFResource* aDownload)
{
  nsCOMPtr<nsIRDFContainer> downloads;
  nsresult rv = GetDownloadsContainer(getter_AddRefs(downloads));
  if (NS_FAILED(rv)) return rv;
  
  
  nsCOMPtr<nsISimpleEnumerator> arcs;
  rv = mDataSource->ArcLabelsOut(aDownload, getter_AddRefs(arcs));
  if (NS_FAILED(rv)) return rv;

  PRBool moreArcs;
  rv = arcs->HasMoreElements(&moreArcs);
  if (NS_FAILED(rv)) return rv;

  while (moreArcs) {
    nsCOMPtr<nsISupports> supports;
    rv = arcs->GetNext(getter_AddRefs(supports));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIRDFResource> arc(do_QueryInterface(supports, &rv));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsISimpleEnumerator> targets;
    rv = mDataSource->GetTargets(aDownload, arc, PR_TRUE, getter_AddRefs(targets));
    if (NS_FAILED(rv)) return rv;

    PRBool moreTargets;
    rv = targets->HasMoreElements(&moreTargets);
    if (NS_FAILED(rv)) return rv;

    while (moreTargets) {
      rv = targets->GetNext(getter_AddRefs(supports));
      if (NS_FAILED(rv)) return rv;

      nsCOMPtr<nsIRDFNode> target(do_QueryInterface(supports, &rv));
      if (NS_FAILED(rv)) return rv;

      
      rv = mDataSource->Unassert(aDownload, arc, target);
      if (NS_FAILED(rv)) return rv;

      rv = targets->HasMoreElements(&moreTargets);
      if (NS_FAILED(rv)) return rv;
    }
    rv = arcs->HasMoreElements(&moreArcs);
    if (NS_FAILED(rv)) return rv;
  }

  PRInt32 itemIndex;
  downloads->IndexOf(aDownload, &itemIndex);
  if (itemIndex <= 0)
    return NS_ERROR_FAILURE;
  
  nsCOMPtr<nsIRDFNode> node;
  rv = downloads->RemoveElementAt(itemIndex, PR_TRUE, getter_AddRefs(node));
  if (NS_FAILED(rv)) return rv;
  
  
  if (mBatches) return rv;

  return Flush();
}  






NS_IMETHODIMP
nsDownloadManager::CleanUp()
{
  nsCOMPtr<nsIRDFResource> downloadRes;
  nsCOMPtr<nsIRDFInt> intLiteral;
  nsCOMPtr<nsISimpleEnumerator> downloads;

  
  
  StartBatchUpdate();
  mDataSource->BeginUpdateBatch();

  
  
  DownloadState states[] = { nsIDownloadManager::DOWNLOAD_FINISHED,
                             nsIDownloadManager::DOWNLOAD_FAILED,
                             nsIDownloadManager::DOWNLOAD_CANCELED,
                             nsIXPInstallManagerUI::INSTALL_FINISHED };

  for (int i = 0; i < 4; ++i) {
    gRDFService->GetIntLiteral(states[i], getter_AddRefs(intLiteral));
    nsresult rv = mDataSource->GetSources(gNC_DownloadState, intLiteral, PR_TRUE, getter_AddRefs(downloads));
    if (NS_FAILED(rv)) return rv;
  
    PRBool hasMoreElements;
    downloads->HasMoreElements(&hasMoreElements);

    while (hasMoreElements) {
      downloads->GetNext(getter_AddRefs(downloadRes));
      
      
      RemoveDownload(downloadRes);
      
      downloads->HasMoreElements(&hasMoreElements);
    }
  }

  mDataSource->EndUpdateBatch();
  EndBatchUpdate();

  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::GetCanCleanUp(PRBool* aResult)
{
  nsCOMPtr<nsIRDFResource> downloadRes;
  nsCOMPtr<nsIRDFInt> intLiteral;

  *aResult = PR_FALSE;

  
  
  DownloadState states[] = { nsIDownloadManager::DOWNLOAD_FINISHED,
                             nsIDownloadManager::DOWNLOAD_FAILED,
                             nsIDownloadManager::DOWNLOAD_CANCELED,
                             nsIXPInstallManagerUI::INSTALL_FINISHED };

  for (int i = 0; i < 4; ++i) {
    gRDFService->GetIntLiteral(states[i], getter_AddRefs(intLiteral));
    
    mDataSource->GetSource(gNC_DownloadState, intLiteral, PR_TRUE, getter_AddRefs(downloadRes));
    if (downloadRes) {
      *aResult = PR_TRUE;
      break;
    }
  }
  return NS_OK;
}

nsresult
nsDownloadManager::ValidateDownloadsContainer()
{
  
  
  
  
  
  nsCOMPtr<nsIRDFContainer> downloads;
  GetDownloadsContainer(getter_AddRefs(downloads));

  nsCOMPtr<nsISimpleEnumerator> e;
  downloads->GetElements(getter_AddRefs(e));

  
  nsCOMPtr<nsISupportsArray> ary;
  NS_NewISupportsArray(getter_AddRefs(ary));

  PRBool hasMore;
  e->HasMoreElements(&hasMore);
  nsCOMPtr<nsIRDFResource> downloadRes;
  while (hasMore) {
    e->GetNext(getter_AddRefs(downloadRes));

    PRBool hasProperty;

    
    
    
    
    
    
    
    
    

    nsIRDFResource* properties[] = { gNC_DownloadState, gNC_File, gNC_Name };
    for (PRInt32 i = 0; i < 3; ++i) {
      mDataSource->HasArcOut(downloadRes, properties[i], &hasProperty);
      if (!hasProperty) {
        ary->AppendElement(downloadRes);
        break;
      }
    }

    e->HasMoreElements(&hasMore);
  }

  
  mDataSource->BeginUpdateBatch();

  
  PRUint32 cnt;
  ary->Count(&cnt);
  for (PRUint32 i = 0; i < cnt; ++i) {
    nsCOMPtr<nsIRDFResource> download(do_QueryElementAt(ary, i));

    
    RemoveDownload(download);
  }

  mDataSource->EndUpdateBatch();

  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::SetListener(nsIDownloadProgressListener* aListener)
{
  mListener = aListener;
  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::GetListener(nsIDownloadProgressListener** aListener)
{
  *aListener = mListener;
  NS_IF_ADDREF(*aListener);
  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::StartBatchUpdate()
{
  ++mBatches;
  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::EndBatchUpdate()
{
  return --mBatches == 0 ? Flush() : NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::PauseDownload(const PRUnichar* aPath)
{
  return PauseResumeDownload(aPath, PR_TRUE);
}

NS_IMETHODIMP
nsDownloadManager::ResumeDownload(const PRUnichar* aPath)
{
  return PauseResumeDownload(aPath, PR_FALSE);
}

nsresult
nsDownloadManager::PauseResumeDownload(const PRUnichar* aPath, PRBool aPause)
{
  nsresult rv;

  nsStringKey key(aPath);
  if (!mCurrDownloads.Exists(&key))
    return NS_ERROR_FAILURE;

  nsDownload* internalDownload = NS_STATIC_CAST(nsDownload*, mCurrDownloads.Get(&key));
  if (!internalDownload)
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIRDFInt> intLiteral;

  gRDFService->GetIntLiteral(
    aPause ? 
    (PRInt32)nsIDownloadManager::DOWNLOAD_PAUSED : 
    (PRInt32)nsIDownloadManager::DOWNLOAD_DOWNLOADING, getter_AddRefs(intLiteral));

  nsCOMPtr<nsIRDFResource> res;
  gRDFService->GetUnicodeResource(nsDependentString(aPath), getter_AddRefs(res));

  nsCOMPtr<nsIRDFNode> oldTarget;
  mDataSource->GetTarget(res, gNC_DownloadState, PR_TRUE, getter_AddRefs(oldTarget));

  if (oldTarget) {
    rv = mDataSource->Change(res, gNC_DownloadState, oldTarget, intLiteral);
    if (NS_FAILED(rv)) return rv;
  }
  else {
    rv = mDataSource->Assert(res, gNC_DownloadState, intLiteral, PR_TRUE);
    if (NS_FAILED(rv)) return rv;
  }

  
  internalDownload->Pause(aPause);

  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::Flush()
{
  
  ValidateDownloadsContainer();

  nsCOMPtr<nsIRDFRemoteDataSource> remote(do_QueryInterface(mDataSource));
  return remote->Flush();
}

NS_IMETHODIMP
nsDownloadManager::GetDatasource(nsIRDFDataSource** aDatasource)
{
  *aDatasource = mDataSource;
  NS_IF_ADDREF(*aDatasource);
  return NS_OK;
}
  
NS_IMETHODIMP
nsDownloadManager::Open(nsIDOMWindow* aParent, const PRUnichar* aPath)
{
  
  nsStringKey key(aPath);
  if (!mCurrDownloads.Exists(&key))
    return NS_ERROR_FAILURE;

  nsDownload* internalDownload = NS_STATIC_CAST(nsDownload*, mCurrDownloads.Get(&key));
  if (!internalDownload)
    return NS_ERROR_FAILURE;

  
  AssertProgressInfoFor(aPath);

  nsVoidArray* params = new nsVoidArray();
  if (!params)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_IF_ADDREF(aParent);
  NS_ADDREF(internalDownload);

  params->AppendElement((void*)aParent);
  params->AppendElement((void*)internalDownload);

  PRInt32 delay = 0;
  nsCOMPtr<nsIPrefBranch> pref(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (pref)
    pref->GetIntPref(PREF_BDM_OPENDELAY, &delay);

  
  
  
  
  
  mDMOpenTimer = do_CreateInstance("@mozilla.org/timer;1");
  return mDMOpenTimer->InitWithFuncCallback(OpenTimerCallback, 
                                       (void*)params, delay, 
                                       nsITimer::TYPE_ONE_SHOT);
}

void
nsDownloadManager::OpenTimerCallback(nsITimer* aTimer, void* aClosure)
{
  nsVoidArray* params = (nsVoidArray*)aClosure;
  nsIDOMWindow* parent = (nsIDOMWindow*)params->ElementAt(0);
  nsDownload* download = (nsDownload*)params->ElementAt(1);
  
  PRInt32 complete;
  download->GetPercentComplete(&complete);
  
  PRBool closeDM = PR_FALSE;
  nsCOMPtr<nsIPrefBranch> pref(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (pref)
    pref->GetBoolPref(PREF_BDM_CLOSEWHENDONE, &closeDM);

  
  if (!closeDM || complete < 100) {
    PRBool focusDM = PR_FALSE;
    PRBool showDM = PR_TRUE;
    PRInt32 flashCount = -1;

    if (pref) {
      pref->GetBoolPref(PREF_BDM_FOCUSWHENSTARTING, &focusDM);

      
      pref->GetBoolPref(PREF_BDM_SHOWWHENSTARTING, &showDM);
      if (showDM) 
        pref->GetIntPref(PREF_BDM_FLASHCOUNT, &flashCount);
      else
        flashCount = 0;
    }

    nsDownloadManager::OpenDownloadManager(focusDM, flashCount, download, parent);
  }

  NS_RELEASE(download);
  NS_IF_RELEASE(parent);

  delete params;
}

nsresult
nsDownloadManager::OpenDownloadManager(PRBool aShouldFocus, PRInt32 aFlashCount, nsIDownload* aDownload, nsIDOMWindow* aParent)
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsIWindowMediator> wm = do_GetService(NS_WINDOWMEDIATOR_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIDOMWindowInternal> recentWindow;
  wm->GetMostRecentWindow(NS_LITERAL_STRING("Download:Manager").get(), getter_AddRefs(recentWindow));
  if (recentWindow) {
    nsCOMPtr<nsIObserverService> obsService = do_GetService("@mozilla.org/observer-service;1", &rv);
    if (NS_FAILED(rv)) return rv;
    
    if (aShouldFocus)
      recentWindow->Focus();
    else {
      nsCOMPtr<nsIDOMChromeWindow> chromeWindow(do_QueryInterface(recentWindow));
      chromeWindow->GetAttentionWithCycleCount(aFlashCount);
    }
  }
  else {
    
    
    nsCOMPtr<nsIWindowWatcher> ww = do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    
    nsCOMPtr<nsISupportsArray> params;
    NS_NewISupportsArray(getter_AddRefs(params));

    
    nsCOMPtr<nsIDownloadManager> dlMgr(do_GetService("@mozilla.org/download-manager;1"));
    nsCOMPtr<nsIRDFDataSource> ds;
    dlMgr->GetDatasource(getter_AddRefs(ds));

    params->AppendElement(ds);
    params->AppendElement(aDownload);
    
    nsCOMPtr<nsIDOMWindow> newWindow;
    rv = ww->OpenWindow(aParent,
                        DOWNLOAD_MANAGER_FE_URL,
                        "_blank",
                        "chrome,dialog=no,resizable",
                        params,
                        getter_AddRefs(newWindow));
  }
  return rv;
}




NS_IMETHODIMP
nsDownloadManager::Observe(nsISupports* aSubject, const char* aTopic, const PRUnichar* aData)
{
  nsresult rv;
  PRInt32 currDownloadCount = 0;

  if (nsCRT::strcmp(aTopic, "oncancel") == 0) {
    nsCOMPtr<nsIDownload> dl = do_QueryInterface(aSubject);
    nsCOMPtr<nsIURI> target;
    dl->GetTarget(getter_AddRefs(target));

    nsAutoString path;
    rv = GetFilePathFromURI(target, path);
    if (NS_FAILED(rv)) return rv;
    
    nsStringKey key(path);
    if (mCurrDownloads.Exists(&key)) {
      
      nsDownload* download = NS_STATIC_CAST(nsDownload*, mCurrDownloads.Get(&key));
      download->SetDialog(nsnull);
      
      return CancelDownload(path.get());  
    }
  }
  else if (nsCRT::strcmp(aTopic, "quit-application") == 0) {
    gStoppingDownloads = PR_TRUE;
    if (mCurrDownloads.Count()) {
      mCurrDownloads.Enumerate(CancelAllDownloads, this);

      
      
      gObserverService->NotifyObservers(mXPIProgress, "xpinstall-progress", NS_LITERAL_STRING("cancel").get());

      
      SaveState();
    }

    
    
    if (GetRetentionBehavior() == 1) {
      nsCOMPtr<nsIRDFContainer> ctr;
      GetDownloadsContainer(getter_AddRefs(ctr));

      StartBatchUpdate();

      nsCOMPtr<nsISupportsArray> ary;
      NS_NewISupportsArray(getter_AddRefs(ary));

      if (ary) {
        nsCOMPtr<nsISimpleEnumerator> e;
        ctr->GetElements(getter_AddRefs(e));

        PRBool hasMore;
        e->HasMoreElements(&hasMore);
        while(hasMore) {
          nsCOMPtr<nsIRDFResource> curr;
          e->GetNext(getter_AddRefs(curr));
          
          ary->AppendElement(curr);
          
          e->HasMoreElements(&hasMore);
        }

        
        PRUint32 cnt;
        ary->Count(&cnt);
        for (PRUint32 i = 0; i < cnt; ++i) {
          nsCOMPtr<nsIRDFResource> download(do_QueryElementAt(ary, i));
          
          
          
          
          RemoveDownload(download);
        }
      }

      EndBatchUpdate();
    }
  }
  else if (nsCRT::strcmp(aTopic, "quit-application-requested") == 0 && 
           (currDownloadCount = mCurrDownloads.Count())) {
    nsCOMPtr<nsISupportsPRBool> cancelDownloads(do_QueryInterface(aSubject));
#ifndef XP_MACOSX
    ConfirmCancelDownloads(currDownloadCount, cancelDownloads,
                           NS_LITERAL_STRING("quitCancelDownloadsAlertTitle").get(),
                           NS_LITERAL_STRING("quitCancelDownloadsAlertMsgMultiple").get(),
                           NS_LITERAL_STRING("quitCancelDownloadsAlertMsg").get(),
                           NS_LITERAL_STRING("dontQuitButtonWin").get());
#else
    ConfirmCancelDownloads(currDownloadCount, cancelDownloads,
                           NS_LITERAL_STRING("quitCancelDownloadsAlertTitle").get(),
                           NS_LITERAL_STRING("quitCancelDownloadsAlertMsgMacMultiple").get(),
                           NS_LITERAL_STRING("quitCancelDownloadsAlertMsgMac").get(),
                           NS_LITERAL_STRING("dontQuitButtonMac").get());
#endif
  }
  else if (nsCRT::strcmp(aTopic, "offline-requested") == 0 && 
           (currDownloadCount = mCurrDownloads.Count())) {
    nsCOMPtr<nsISupportsPRBool> cancelDownloads(do_QueryInterface(aSubject));
    ConfirmCancelDownloads(currDownloadCount, cancelDownloads,
                           NS_LITERAL_STRING("offlineCancelDownloadsAlertTitle").get(),
                           NS_LITERAL_STRING("offlineCancelDownloadsAlertMsgMultiple").get(),
                           NS_LITERAL_STRING("offlineCancelDownloadsAlertMsg").get(),
                           NS_LITERAL_STRING("dontGoOfflineButton").get());
    PRBool data;
    cancelDownloads->GetData(&data);
    if (!data) {
      gStoppingDownloads = PR_TRUE;

      
      
      gObserverService->NotifyObservers(mXPIProgress, "xpinstall-progress", NS_LITERAL_STRING("cancel").get());
    
      mCurrDownloads.Enumerate(CancelAllDownloads, this);
      gStoppingDownloads = PR_FALSE;
    }
  }
  else if (nsCRT::strcmp(aTopic, "alertclickcallback") == 0)
  {
    
    nsCOMPtr<nsIWindowMediator> wm = do_GetService(NS_WINDOWMEDIATOR_CONTRACTID, &rv);
    nsCOMPtr<nsIDOMWindowInternal> browserWindow;
    if (wm)
      wm->GetMostRecentWindow(NS_LITERAL_STRING("navigator:browser").get(), getter_AddRefs(browserWindow));

    return OpenDownloadManager(PR_TRUE, -1, nsnull, browserWindow);
  }
  return NS_OK;
}

void
nsDownloadManager::ConfirmCancelDownloads(PRInt32 aCount, nsISupportsPRBool* aCancelDownloads,
                                          const PRUnichar* aTitle, 
                                          const PRUnichar* aCancelMessageMultiple, 
                                          const PRUnichar* aCancelMessageSingle,
                                          const PRUnichar* aDontCancelButton)
{
  nsXPIDLString title, message, quitButton, dontQuitButton;
  
  nsCOMPtr<nsIStringBundleService> bundleService = do_GetService(NS_STRINGBUNDLE_CONTRACTID);
  nsCOMPtr<nsIStringBundle> bundle;
  if (bundleService)
    bundleService->CreateBundle(DOWNLOAD_MANAGER_BUNDLE, getter_AddRefs(bundle));
  if (bundle) {
    bundle->GetStringFromName(aTitle, getter_Copies(title));    

    nsAutoString countString;
    countString.AppendInt(aCount);
    const PRUnichar* strings[1] = { countString.get() };
    if (aCount > 1) {
      bundle->FormatStringFromName(aCancelMessageMultiple, strings, 1, getter_Copies(message));
      bundle->FormatStringFromName(NS_LITERAL_STRING("cancelDownloadsOKTextMultiple").get(), strings, 1, getter_Copies(quitButton));
    }
    else {
      bundle->GetStringFromName(aCancelMessageSingle, getter_Copies(message));
      bundle->GetStringFromName(NS_LITERAL_STRING("cancelDownloadsOKText").get(), getter_Copies(quitButton));
    }

    bundle->GetStringFromName(aDontCancelButton, getter_Copies(dontQuitButton));
  }

  
  nsCOMPtr<nsIWindowMediator> wm = do_GetService(NS_WINDOWMEDIATOR_CONTRACTID);
  nsCOMPtr<nsIDOMWindowInternal> dmWindow;
  if (wm)
    wm->GetMostRecentWindow(NS_LITERAL_STRING("Download:Manager").get(), getter_AddRefs(dmWindow));

  
  nsCOMPtr<nsIPromptService> prompter(do_GetService(NS_PROMPTSERVICE_CONTRACTID));
  if (prompter) {
    PRInt32 flags = (nsIPromptService::BUTTON_TITLE_IS_STRING * nsIPromptService::BUTTON_POS_0) + (nsIPromptService::BUTTON_TITLE_IS_STRING * nsIPromptService::BUTTON_POS_1);
    PRBool nothing = PR_FALSE;
    PRInt32 button;
    prompter->ConfirmEx(dmWindow, title, message, flags, quitButton.get(), dontQuitButton.get(), nsnull, nsnull, &nothing, &button);

    aCancelDownloads->SetData(button == 1);
  }
}



NS_IMETHODIMP
nsDownloadManager::GetXpiProgress(nsIXPIProgressDialog** aProgress)
{
  *aProgress = mXPIProgress;
  NS_IF_ADDREF(*aProgress);
  return NS_OK;
}

NS_IMETHODIMP
nsDownloadManager::GetHasActiveXPIOperations(PRBool* aHasOps)
{
  nsIXPIProgressDialog* dialog = mXPIProgress.get();
  nsXPIProgressListener* listener = NS_STATIC_CAST(nsXPIProgressListener*, dialog);
  *aHasOps = !mXPIProgress ? PR_FALSE : listener->HasActiveXPIOperations();
  return NS_OK;
}




NS_IMPL_ISUPPORTS1(nsXPIProgressListener, nsIXPIProgressDialog)

nsXPIProgressListener::nsXPIProgressListener(nsDownloadManager* aDownloadManager)
{
  NS_NewISupportsArray(getter_AddRefs(mDownloads));

  mDownloadManager = aDownloadManager;
}

nsXPIProgressListener::~nsXPIProgressListener()
{
  
  mDownloads->Clear();

  mDownloadManager = nsnull;
}

void
nsXPIProgressListener::AddDownload(nsIDownload* aDownload)
{
  PRUint32 cnt;
  mDownloads->Count(&cnt);
  PRBool foundMatch = PR_FALSE;

  nsCOMPtr<nsIURI> uri1, uri2;
  for (PRUint32 i = 0; i < cnt; ++i) {
    nsCOMPtr<nsIDownload> download(do_QueryElementAt(mDownloads, i));
    download->GetSource(getter_AddRefs(uri1));
    aDownload->GetSource(getter_AddRefs(uri2));

    uri1->Equals(uri2, &foundMatch);
    if (foundMatch)
      break;
  }
  if (!foundMatch)
    mDownloads->AppendElement(aDownload);
}

void 
nsXPIProgressListener::RemoveDownloadAtIndex(PRUint32 aIndex)
{
  mDownloads->RemoveElementAt(aIndex);
}

PRBool
nsXPIProgressListener::HasActiveXPIOperations()
{
  PRUint32 count;
  mDownloads->Count(&count);
  return count != 0;
}



NS_IMETHODIMP
nsXPIProgressListener::OnStateChange(PRUint32 aIndex, PRInt16 aState, PRInt32 aValue)
{
  nsCOMPtr<nsIWebProgressListener> wpl(do_QueryElementAt(mDownloads, aIndex));
  nsIWebProgressListener* temp = wpl.get();
  nsDownload* dl = NS_STATIC_CAST(nsDownload*, temp);
  
  
  if (!dl) 
    return NS_ERROR_FAILURE;

  switch (aState) {
  case nsIXPIProgressDialog::DOWNLOAD_START:
    wpl->OnStateChange(nsnull, nsnull, nsIWebProgressListener::STATE_START, 0);

    dl->SetDownloadState(nsIXPInstallManagerUI::INSTALL_DOWNLOADING);
    AssertProgressInfoForDownload(dl);
    break;
  case nsIXPIProgressDialog::DOWNLOAD_DONE:
    break;
  case nsIXPIProgressDialog::INSTALL_START:
    dl->SetDownloadState(nsIXPInstallManagerUI::INSTALL_INSTALLING);
    AssertProgressInfoForDownload(dl);
    break;
  case nsIXPIProgressDialog::INSTALL_DONE:
    wpl->OnStateChange(nsnull, nsnull, nsIWebProgressListener::STATE_STOP, 0);
    
    dl->SetDownloadState(nsIXPInstallManagerUI::INSTALL_FINISHED);
    AssertProgressInfoForDownload(dl);
    
    
    RemoveDownloadAtIndex(aIndex);
    break;
  case nsIXPIProgressDialog::DIALOG_CLOSE:
    
    gObserverService->NotifyObservers(nsnull, "xpinstall-dialog-close", nsnull);

    if (!gStoppingDownloads) {
      nsCOMPtr<nsIStringBundleService> sbs(do_GetService("@mozilla.org/intl/stringbundle;1"));
      nsCOMPtr<nsIStringBundle> brandBundle, xpinstallBundle;
      sbs->CreateBundle("chrome://branding/locale/brand.properties", getter_AddRefs(brandBundle));
      sbs->CreateBundle("chrome://mozapps/locale/xpinstall/xpinstallConfirm.properties", getter_AddRefs(xpinstallBundle));

      nsXPIDLString brandShortName, message, title;
      brandBundle->GetStringFromName(NS_LITERAL_STRING("brandShortName").get(), getter_Copies(brandShortName));
      const PRUnichar* strings[1] = { brandShortName.get() };
      xpinstallBundle->FormatStringFromName(NS_LITERAL_STRING("installComplete").get(), strings, 1, getter_Copies(message));
      xpinstallBundle->GetStringFromName(NS_LITERAL_STRING("installCompleteTitle").get(), getter_Copies(title));

      nsCOMPtr<nsIPromptService> ps(do_GetService(NS_PROMPTSERVICE_CONTRACTID));
      ps->Alert(nsnull, title, message);
    }

    break;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXPIProgressListener::OnProgress(PRUint32 aIndex, PRUint64 aValue, PRUint64 aMaxValue)
{
  nsCOMPtr<nsIWebProgressListener> wpl(do_QueryElementAt(mDownloads, aIndex));
  
  if (wpl) 
    return wpl->OnProgressChange(nsnull, nsnull, 0, 0, nsUint64(aValue),
                                 nsUint64(aMaxValue));
  return NS_OK;
}

void 
nsXPIProgressListener::AssertProgressInfoForDownload(nsDownload* aDownload)
{
  nsCOMPtr<nsIURI> target;
  aDownload->GetTarget(getter_AddRefs(target));

  nsAutoString path;
  GetFilePathFromURI(target, path);

  mDownloadManager->AssertProgressInfoFor(path.get());
}











NS_IMPL_CYCLE_COLLECTION_CLASS(nsDownloadsDataSource)
NS_IMPL_CYCLE_COLLECTION_UNLINK_0(nsDownloadsDataSource)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsDownloadsDataSource)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mInner)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END


NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsDownloadsDataSource,
                                          nsIRDFDataSource)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(nsDownloadsDataSource,
                                           nsIRDFDataSource)

NS_INTERFACE_MAP_BEGIN(nsDownloadsDataSource)
    NS_INTERFACE_MAP_ENTRY(nsIRDFDataSource)
    NS_INTERFACE_MAP_ENTRY(nsIRDFRemoteDataSource)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIRDFDataSource)
    NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(nsDownloadsDataSource)
NS_INTERFACE_MAP_END

nsresult
nsDownloadsDataSource::LoadDataSource()
{
  nsCOMPtr<nsIFile> downloadsFile;
  nsresult rv = NS_GetSpecialDirectory(NS_APP_DOWNLOADS_50_FILE, getter_AddRefs(downloadsFile));
  if (NS_FAILED(rv)) return rv;

  nsCAutoString downloadsDB;
  NS_GetURLSpecFromFile(downloadsFile, downloadsDB);

  return gRDFService->GetDataSourceBlocking(downloadsDB.get(), getter_AddRefs(mInner));
}




NS_IMETHODIMP 
nsDownloadsDataSource::GetURI(char** aURI)
{
  return mInner->GetURI(aURI);
}

NS_IMETHODIMP 
nsDownloadsDataSource::GetSource(nsIRDFResource* aProperty, nsIRDFNode* aTarget, PRBool aTruthValue, nsIRDFResource** aResult)
{
  return mInner->GetSource(aProperty, aTarget, aTruthValue, aResult);
}

NS_IMETHODIMP 
nsDownloadsDataSource::GetSources(nsIRDFResource* aProperty, nsIRDFNode* aTarget, PRBool aTruthValue, nsISimpleEnumerator** aResult)
{
  return mInner->GetSources(aProperty, aTarget, aTruthValue, aResult);
}

NS_IMETHODIMP 
nsDownloadsDataSource::GetTarget(nsIRDFResource* aSource, nsIRDFResource* aProperty, PRBool aTruthValue, nsIRDFNode** aResult)
{
  if (aProperty == gNC_IconURL) {
    PRBool hasIconURLArc;
    nsresult rv = mInner->HasArcOut(aSource, aProperty, &hasIconURLArc);
    if (NS_FAILED(rv)) return rv;

    
    
    if (!hasIconURLArc) {
      nsCOMPtr<nsIRDFNode> target;
      rv = mInner->GetTarget(aSource, gNC_File, aTruthValue, getter_AddRefs(target));
      if (NS_SUCCEEDED(rv) && target) {
        nsXPIDLCString path;
        nsCOMPtr<nsIRDFResource> res(do_QueryInterface(target));
        res->GetValue(getter_Copies(path));

        
        
        PRBool alreadyURL = PR_FALSE;
        nsCOMPtr<nsIURI> fileURI;
        NS_NewURI(getter_AddRefs(fileURI), path);
        if (fileURI) {
          nsCOMPtr<nsIURL> url(do_QueryInterface(fileURI, &rv));
          if (NS_SUCCEEDED(rv))
            alreadyURL = PR_TRUE;
        }
        
        nsCAutoString fileURL;
        if (alreadyURL) {
          fileURL.Assign(path);
        }
        else {
          nsCOMPtr<nsILocalFile> lf(do_CreateInstance("@mozilla.org/file/local;1"));
          lf->InitWithNativePath(path);
          nsCOMPtr<nsIIOService> ios(do_GetService("@mozilla.org/network/io-service;1"));
          nsCOMPtr<nsIProtocolHandler> ph;
          ios->GetProtocolHandler("file", getter_AddRefs(ph));
          nsCOMPtr<nsIFileProtocolHandler> fph(do_QueryInterface(ph));

          fph->GetURLSpecFromFile(lf, fileURL);
        }

        nsAutoString iconURL(NS_LITERAL_STRING("moz-icon://"));
        AppendUTF8toUTF16(fileURL, iconURL);
        iconURL.AppendLiteral("?size=32");

        nsCOMPtr<nsIRDFResource> result;
        gRDFService->GetUnicodeResource(iconURL, getter_AddRefs(result));

        *aResult = result;
        NS_IF_ADDREF(*aResult);

        return NS_OK;
      }
    }
  }
  
  
  return mInner->GetTarget(aSource, aProperty, aTruthValue, aResult);
}

NS_IMETHODIMP 
nsDownloadsDataSource::GetTargets(nsIRDFResource* aSource, nsIRDFResource* aProperty, PRBool aTruthValue, nsISimpleEnumerator** aResult)
{
  if (aProperty == gNC_IconURL) {
    nsCOMPtr<nsIRDFNode> target;
    nsresult rv = GetTarget(aSource, aProperty, aTruthValue, getter_AddRefs(target));
    if (NS_FAILED(rv)) return rv;

    if (NS_SUCCEEDED(rv)) 
      return NS_NewSingletonEnumerator(aResult, target);
  }
  return mInner->GetTargets(aSource, aProperty, aTruthValue, aResult);
}

NS_IMETHODIMP 
nsDownloadsDataSource::Assert(nsIRDFResource* aSource, nsIRDFResource* aProperty, nsIRDFNode* aTarget, PRBool aTruthValue)
{
  return mInner->Assert(aSource, aProperty, aTarget, aTruthValue);
}

NS_IMETHODIMP 
nsDownloadsDataSource::Unassert(nsIRDFResource* aSource, nsIRDFResource* aProperty, nsIRDFNode* aTarget)
{
  return mInner->Unassert(aSource, aProperty, aTarget);
}

NS_IMETHODIMP 
nsDownloadsDataSource::Change(nsIRDFResource* aSource, nsIRDFResource* aProperty, nsIRDFNode* aOldTarget, nsIRDFNode* aNewTarget)
{
  return mInner->Change(aSource, aProperty, aOldTarget, aNewTarget);
}

NS_IMETHODIMP 
nsDownloadsDataSource::Move(nsIRDFResource* aOldSource, nsIRDFResource* aNewSource, nsIRDFResource* aProperty, nsIRDFNode* aTarget)
{
  return mInner->Move(aOldSource, aNewSource, aProperty, aTarget);
}

NS_IMETHODIMP 
nsDownloadsDataSource::HasAssertion(nsIRDFResource* aSource, nsIRDFResource* aProperty, nsIRDFNode* aTarget, PRBool aTruthValue, PRBool* aResult)
{
  return mInner->HasAssertion(aSource, aProperty, aTarget, aTruthValue, aResult);
}

NS_IMETHODIMP 
nsDownloadsDataSource::AddObserver(nsIRDFObserver* aObserver)
{
  return mInner->AddObserver(aObserver);
}

NS_IMETHODIMP 
nsDownloadsDataSource::RemoveObserver(nsIRDFObserver* aObserver)
{
  return mInner->RemoveObserver(aObserver);
}

NS_IMETHODIMP 
nsDownloadsDataSource::ArcLabelsIn(nsIRDFNode* aNode, nsISimpleEnumerator** aResult)
{
  return mInner->ArcLabelsIn(aNode, aResult);
}

NS_IMETHODIMP 
nsDownloadsDataSource::ArcLabelsOut(nsIRDFResource* aSource, nsISimpleEnumerator** aResult)
{
  return mInner->ArcLabelsOut(aSource, aResult);
}

NS_IMETHODIMP 
nsDownloadsDataSource::GetAllResources(nsISimpleEnumerator** aResult)
{
  return mInner->GetAllResources(aResult);
}

NS_IMETHODIMP 
nsDownloadsDataSource::IsCommandEnabled(nsISupportsArray* aSources, nsIRDFResource* aCommand, nsISupportsArray* aArguments, PRBool* aResult)
{
  return mInner->IsCommandEnabled(aSources, aCommand, aArguments, aResult);
}

NS_IMETHODIMP 
nsDownloadsDataSource::DoCommand(nsISupportsArray* aSources, nsIRDFResource* aCommand, nsISupportsArray* aArguments)
{
  return mInner->DoCommand(aSources, aCommand, aArguments);
}

NS_IMETHODIMP 
nsDownloadsDataSource::GetAllCmds(nsIRDFResource* aSource, nsISimpleEnumerator** aResult)
{
  return mInner->GetAllCmds(aSource, aResult);
}

NS_IMETHODIMP 
nsDownloadsDataSource::HasArcIn(nsIRDFNode* aNode, nsIRDFResource* aArc, PRBool* aResult)
{
  return mInner->HasArcIn(aNode, aArc, aResult);
}

NS_IMETHODIMP 
nsDownloadsDataSource::HasArcOut(nsIRDFResource* aSource, nsIRDFResource* aArc, PRBool* aResult)
{
  return mInner->HasArcOut(aSource, aArc, aResult);
}

NS_IMETHODIMP 
nsDownloadsDataSource::BeginUpdateBatch()
{
  return mInner->BeginUpdateBatch();
}

NS_IMETHODIMP 
nsDownloadsDataSource::EndUpdateBatch()
{
  return mInner->EndUpdateBatch();
}




NS_IMETHODIMP
nsDownloadsDataSource::GetLoaded(PRBool* aResult)
{
  nsCOMPtr<nsIRDFRemoteDataSource> rds(do_QueryInterface(mInner));
  return rds->GetLoaded(aResult);
}

NS_IMETHODIMP
nsDownloadsDataSource::Init(const char* aURI)
{
  nsCOMPtr<nsIRDFRemoteDataSource> rds(do_QueryInterface(mInner));
  return rds->Init(aURI);
}

NS_IMETHODIMP
nsDownloadsDataSource::Refresh(PRBool aBlocking)
{
  nsCOMPtr<nsIRDFRemoteDataSource> rds(do_QueryInterface(mInner));
  return rds->Refresh(aBlocking);
}

NS_IMETHODIMP
nsDownloadsDataSource::Flush()
{
  nsCOMPtr<nsIRDFRemoteDataSource> rds(do_QueryInterface(mInner));
  return rds->Flush();
}

NS_IMETHODIMP
nsDownloadsDataSource::FlushTo(const char* aURI)
{
  nsCOMPtr<nsIRDFRemoteDataSource> rds(do_QueryInterface(mInner));
  return rds->FlushTo(aURI);
}




NS_IMPL_ISUPPORTS4(nsDownload, nsIDownload, nsITransfer, nsIWebProgressListener,
                   nsIWebProgressListener2)

nsDownload::nsDownload():mDownloadState(nsIDownloadManager::DOWNLOAD_NOTSTARTED),
                         mPercentComplete(0),
                         mCurrBytes(LL_ZERO),
                         mMaxBytes(LL_ZERO),
                         mStartTime(LL_ZERO),
                         mLastUpdate(PR_Now() - (PRUint32)gInterval),
                         mPaused(PR_FALSE),
                         mSpeed(0)
{
}

nsDownload::~nsDownload()
{  
}

nsresult
nsDownload::SetDownloadManager(nsDownloadManager* aDownloadManager)
{
  mDownloadManager = aDownloadManager;
  return NS_OK;
}

nsresult
nsDownload::SetDialogListener(nsIWebProgressListener* aDialogListener)
{
  mDialogListener = aDialogListener;
  return NS_OK;
}

nsresult
nsDownload::GetDialogListener(nsIWebProgressListener** aDialogListener)
{
  *aDialogListener = mDialogListener;
  NS_IF_ADDREF(*aDialogListener);
  return NS_OK;
}

nsresult
nsDownload::SetDialog(nsIProgressDialog* aDialog)
{
  mDialog = aDialog;
  return NS_OK;
}

nsresult
nsDownload::GetDialog(nsIProgressDialog** aDialog)
{
  *aDialog = mDialog;
  NS_IF_ADDREF(*aDialog);
  return NS_OK;
}

nsresult
nsDownload::SetTempFile(nsILocalFile* aTempFile)
{
  mTempFile = aTempFile;
  return NS_OK;
}

nsresult
nsDownload::GetTempFile(nsILocalFile** aTempFile)
{
  *aTempFile = mTempFile;
  NS_IF_ADDREF(*aTempFile);
  return NS_OK;
}

DownloadState
nsDownload::GetDownloadState()
{
  return mDownloadState;
}

void
nsDownload::SetDownloadState(DownloadState aState)
{
  mDownloadState = aState;
}

DownloadType
nsDownload::GetDownloadType()
{
  return mDownloadType;
}

void
nsDownload::SetDownloadType(DownloadType aType)
{
  mDownloadType = aType;
}

nsresult
nsDownload::SetCancelable(nsICancelable* aCancelable)
{
  mCancelable = aCancelable;
  return NS_OK;
}

nsresult
nsDownload::SetSource(nsIURI* aSource)
{
  mSource = aSource;
  return NS_OK;
}

nsresult
nsDownload::SetTarget(nsIURI* aTarget)
{
  mTarget = aTarget;
  return NS_OK;
}


nsresult
nsDownload::SetDisplayName(const PRUnichar* aDisplayName)
{
  mDisplayName = aDisplayName;

  nsCOMPtr<nsIRDFDataSource> ds;
  mDownloadManager->GetDatasource(getter_AddRefs(ds));

  nsCOMPtr<nsIRDFLiteral> nameLiteral;
  nsCOMPtr<nsIRDFResource> res;
  nsAutoString path;
  nsresult rv = GetFilePathFromURI(mTarget, path);
  if (NS_FAILED(rv)) return rv;

  gRDFService->GetUnicodeResource(path, getter_AddRefs(res));
  
  gRDFService->GetLiteral(aDisplayName, getter_AddRefs(nameLiteral));
  ds->Assert(res, gNC_Name, nameLiteral, PR_TRUE);

  return NS_OK;
}

nsDownload::TransferInformation
nsDownload::GetTransferInformation()
{
  return TransferInformation(mCurrBytes, mMaxBytes);
}

nsresult
nsDownload::SetStartTime(PRInt64 aStartTime)
{
  mStartTime = aStartTime;
  mLastUpdate = aStartTime;
  return NS_OK;
}

nsresult
nsDownload::SetMIMEInfo(nsIMIMEInfo *aMIMEInfo)
{
  mMIMEInfo = aMIMEInfo;
  return NS_OK;
}




NS_IMETHODIMP
nsDownload::OnProgressChange64(nsIWebProgress *aWebProgress,
                               nsIRequest *aRequest,
                               PRInt64 aCurSelfProgress,
                               PRInt64 aMaxSelfProgress,
                               PRInt64 aCurTotalProgress,
                               PRInt64 aMaxTotalProgress)
{
  if (!mRequest)
    mRequest = aRequest; 

  
  PRTime now = PR_Now();
  nsInt64 delta = now - mLastUpdate;
  if (delta < gInterval)
    return NS_OK;

  mLastUpdate = now;

  if (mDownloadState == nsIDownloadManager::DOWNLOAD_NOTSTARTED) {
    nsAutoString path;
    nsresult rv = GetFilePathFromURI(mTarget, path);
    if (NS_FAILED(rv)) return rv;

    mDownloadState = nsIDownloadManager::DOWNLOAD_DOWNLOADING;
    mDownloadManager->DownloadStarted(path.get());
  }

  
  
  double elapsedSecs = double(delta) / PR_USEC_PER_SEC;
  if (elapsedSecs > 0) {
    nsUint64 curTotalProgress = (PRUint64)aCurTotalProgress;
    nsUint64 diffBytes = curTotalProgress - nsUint64(mCurrBytes);
    double speed = double(diffBytes) / elapsedSecs;
    if (LL_IS_ZERO(mCurrBytes))
      mSpeed = speed;
    else {
      
      mSpeed = mSpeed * 0.9 + speed * 0.1;
    }
  }

  if (aMaxTotalProgress > 0)
    mPercentComplete = (PRInt32)((PRFloat64)aCurTotalProgress * 100 / aMaxTotalProgress + .5);
  else
    mPercentComplete = -1;

  mCurrBytes = aCurTotalProgress;
  mMaxBytes = aMaxTotalProgress;

  if (mDownloadManager->NeedsUIUpdate()) {
    nsCOMPtr<nsIDownloadProgressListener> dpl;
    mDownloadManager->GetInternalListener(getter_AddRefs(dpl));
    if (dpl) {
      dpl->OnProgressChange(aWebProgress, aRequest, aCurSelfProgress, aMaxSelfProgress,
                            aCurTotalProgress, aMaxTotalProgress, this);
    }
  }

  return NS_OK;

}

NS_IMETHODIMP
nsDownload::OnRefreshAttempted(nsIWebProgress *aWebProgress,
                               nsIURI *aUri,
                               PRInt32 aDelay,
                               PRBool aSameUri,
                               PRBool *allowRefresh)
{
  *allowRefresh = PR_TRUE;
  return NS_OK;
}




NS_IMETHODIMP
nsDownload::OnProgressChange(nsIWebProgress *aWebProgress,
                             nsIRequest *aRequest,
                             PRInt32 aCurSelfProgress,
                             PRInt32 aMaxSelfProgress,
                             PRInt32 aCurTotalProgress,
                             PRInt32 aMaxTotalProgress)
{
  return OnProgressChange64(aWebProgress, aRequest,
                            aCurSelfProgress, aMaxSelfProgress,
                            aCurTotalProgress, aMaxTotalProgress);
}

NS_IMETHODIMP
nsDownload::OnLocationChange(nsIWebProgress *aWebProgress,
                             nsIRequest *aRequest, nsIURI *aLocation)
{
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::OnStatusChange(nsIWebProgress *aWebProgress,
                           nsIRequest *aRequest, nsresult aStatus,
                           const PRUnichar *aMessage)
{   
  if (NS_FAILED(aStatus)) {
    mDownloadState = nsIDownloadManager::DOWNLOAD_FAILED;
    nsAutoString path;
    nsresult rv = GetFilePathFromURI(mTarget, path);
    if (NS_SUCCEEDED(rv)) {
      mDownloadManager->DownloadEnded(path.get(), nsnull);
      gObserverService->NotifyObservers(NS_STATIC_CAST(nsIDownload *, this), "dl-failed", nsnull);                     
    }

    
    nsXPIDLString title;
    
    nsCOMPtr<nsIStringBundleService> bundleService = do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
    nsCOMPtr<nsIStringBundle> bundle;
    if (bundleService)
      rv = bundleService->CreateBundle(DOWNLOAD_MANAGER_BUNDLE, getter_AddRefs(bundle));
    if (bundle)
      bundle->GetStringFromName(NS_LITERAL_STRING("downloadErrorAlertTitle").get(), getter_Copies(title));    

    
    nsCOMPtr<nsIWindowMediator> wm = do_GetService(NS_WINDOWMEDIATOR_CONTRACTID, &rv);
    nsCOMPtr<nsIDOMWindowInternal> dmWindow;
    if (wm)
      wm->GetMostRecentWindow(NS_LITERAL_STRING("Download:Manager").get(), getter_AddRefs(dmWindow));

    
    nsCOMPtr<nsIPromptService> prompter(do_GetService("@mozilla.org/embedcomp/prompt-service;1"));
    if (prompter)
      prompter->Alert(dmWindow, title, aMessage);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDownload::OnStateChange(nsIWebProgress* aWebProgress,
                          nsIRequest* aRequest, PRUint32 aStateFlags,
                          nsresult aStatus)
{
  
  if (LL_IS_ZERO(mStartTime) && (aStateFlags & STATE_START))
    SetStartTime(PR_Now());

  
  
  nsCOMPtr<nsIDownload> kungFuDeathGrip;
  CallQueryInterface(this, NS_STATIC_CAST(nsIDownload**,
                                          getter_AddRefs(kungFuDeathGrip)));

  
  
  nsresult rv = NS_OK;

  nsCOMPtr<nsIPrefBranch> pref(do_GetService(NS_PREFSERVICE_CONTRACTID));

  if (aStateFlags & STATE_STOP) {
    if (nsDownloadManager::IsInFinalStage(mDownloadState)) {
      if (mDownloadState != nsIXPInstallManagerUI::INSTALL_INSTALLING)
        mDownloadState = nsIDownloadManager::DOWNLOAD_FINISHED;
      else
        mDownloadState = nsIXPInstallManagerUI::INSTALL_FINISHED;

      
      if (mMaxBytes == -1)
        mMaxBytes = mCurrBytes;

      
      if (mMaxBytes < 1024) {
        mCurrBytes = 1024;
        mMaxBytes  = 1024;
      }

      mPercentComplete = 100;

      nsAutoString path;
      rv = GetFilePathFromURI(mTarget, path);
      
      if (NS_SUCCEEDED(rv))
        mDownloadManager->DownloadEnded(path.get(), nsnull);

      
      PRBool showTaskbarAlert = PR_FALSE;
      if (pref)
        pref->GetBoolPref(PREF_BDM_SHOWALERTONCOMPLETE, &showTaskbarAlert);

      if (showTaskbarAlert) {
        PRInt32 alertInterval = -1;
        if (pref)
          pref->GetIntPref(PREF_BDM_SHOWALERTINTERVAL, &alertInterval);

        PRInt64 temp, uSecPerMSec, alertIntervalUSec;
        LL_I2L(temp, alertInterval);
        LL_I2L(uSecPerMSec, PR_USEC_PER_MSEC);
        LL_MUL(alertIntervalUSec, temp, uSecPerMSec);
        
        PRInt64 goat = PR_Now() - mStartTime;
        showTaskbarAlert = goat > alertIntervalUSec;
        
        if (showTaskbarAlert && (mDownloadManager->mCurrDownloads).Count() == 0) {
          nsCOMPtr<nsIAlertsService> alerts(do_GetService("@mozilla.org/alerts-service;1"));
          if (alerts) {
            nsXPIDLString title, message;

            mDownloadManager->mBundle->GetStringFromName(NS_LITERAL_STRING("downloadsCompleteTitle").get(), getter_Copies(title));
            mDownloadManager->mBundle->GetStringFromName(NS_LITERAL_STRING("downloadsCompleteMsg").get(), getter_Copies(message));

            PRBool removeWhenDone = mDownloadManager->GetRetentionBehavior() == 0;


            
            
            
            
            alerts->ShowAlertNotification(NS_LITERAL_STRING(DOWNLOAD_MANAGER_ALERT_ICON), title, message, !removeWhenDone, 
                                          EmptyString(), mDownloadManager);
          }
        }
      }

      gObserverService->NotifyObservers(NS_STATIC_CAST(nsIDownload *, this), "dl-done", nsnull);
    }

    nsAutoString path;
    rv = GetFilePathFromURI(mTarget, path);
    if (NS_FAILED(rv))
      return rv;

#ifdef XP_WIN
    PRBool addToRecentDocs = PR_TRUE;
    if (pref)
      pref->GetBoolPref(PREF_BDM_ADDTORECENTDOCS, &addToRecentDocs);

    if (addToRecentDocs) {
      LPSHELLFOLDER lpShellFolder = NULL;

      if (SUCCEEDED(::SHGetDesktopFolder(&lpShellFolder))) {
        PRUnichar *filePath = ToNewUnicode(path);
        LPITEMIDLIST lpItemIDList = NULL;
        if (SUCCEEDED(lpShellFolder->ParseDisplayName(NULL, NULL, filePath, NULL, &lpItemIDList, NULL))) {
          ::SHAddToRecentDocs(SHARD_PIDL, lpItemIDList);
          ::CoTaskMemFree(lpItemIDList);
        }
        nsMemory::Free(filePath);
        lpShellFolder->Release();
      }
    }
#endif

    
    mCancelable = nsnull;

    
    if (mDownloadManager->GetRetentionBehavior() == 0)
      mDownloadManager->RemoveDownload(path.get());
  }

  if (mDownloadManager->NeedsUIUpdate()) {
    nsCOMPtr<nsIDownloadProgressListener> dpl;
    mDownloadManager->GetInternalListener(getter_AddRefs(dpl));
    if (dpl)
      dpl->OnStateChange(aWebProgress, aRequest, aStateFlags, aStatus, this);
  }

  return rv;
}

NS_IMETHODIMP
nsDownload::OnSecurityChange(nsIWebProgress *aWebProgress,
                             nsIRequest *aRequest, PRUint32 aState)
{
  return NS_OK;
}




NS_IMETHODIMP
nsDownload::Init(nsIURI* aSource,
                 nsIURI* aTarget,
                 const nsAString& aDisplayName,
                 nsIMIMEInfo *aMIMEInfo,
                 PRTime aStartTime,
                 nsILocalFile* aTempFile,
                 nsICancelable* aCancelable)
{
  NS_WARNING("Huh...how did we get here?!");
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetDisplayName(PRUnichar** aDisplayName)
{
  *aDisplayName = ToNewUnicode(mDisplayName);
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetCancelable(nsICancelable** aCancelable)
{
  *aCancelable = mCancelable;
  NS_IF_ADDREF(*aCancelable);
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetTarget(nsIURI** aTarget)
{
  *aTarget = mTarget;
  NS_IF_ADDREF(*aTarget);
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetSource(nsIURI** aSource)
{
  *aSource = mSource;
  NS_IF_ADDREF(*aSource);
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetStartTime(PRInt64* aStartTime)
{
  *aStartTime = mStartTime;
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetPercentComplete(PRInt32* aPercentComplete)
{
  *aPercentComplete = mPercentComplete;
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetAmountTransferred(PRUint64* aAmountTransferred)
{
  *aAmountTransferred = mCurrBytes;
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetSize(PRUint64* aSize)
{
  *aSize = mMaxBytes;
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetMIMEInfo(nsIMIMEInfo** aMIMEInfo)
{
  *aMIMEInfo = mMIMEInfo;
  NS_IF_ADDREF(*aMIMEInfo);
  return NS_OK;
}

NS_IMETHODIMP
nsDownload::GetTargetFile(nsILocalFile** aTargetFile)
{
  nsresult rv;

  nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(mTarget, &rv);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIFile> file;
  rv = fileURL->GetFile(getter_AddRefs(file));
  if (NS_SUCCEEDED(rv))
    rv = CallQueryInterface(file, aTargetFile);
  return rv;
}

NS_IMETHODIMP
nsDownload::GetSpeed(double* aSpeed)
{
  *aSpeed = mSpeed;
  return NS_OK;
}

void
nsDownload::Pause(PRBool aPaused)
{
  if (mPaused != aPaused) {
    if (mRequest) {
      if (aPaused) {
        mRequest->Suspend();
        mPaused = PR_TRUE;
        mDownloadState = nsIDownloadManager::DOWNLOAD_PAUSED;
      }
      else {
        mRequest->Resume();
        mPaused = PR_FALSE;
        mDownloadState = nsIDownloadManager::DOWNLOAD_DOWNLOADING;
      }
    }
  }
}

PRBool
nsDownload::IsPaused()
{
  return mPaused;
}
