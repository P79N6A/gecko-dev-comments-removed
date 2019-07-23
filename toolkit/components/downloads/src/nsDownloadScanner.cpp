





































 
#include "nsDownloadScanner.h"
#include <comcat.h>
#include <process.h>
#include "nsDownloadManager.h"
#include "nsIXULAppInfo.h"
#include "nsXULAppAPI.h"
#include "nsIPrefService.h"
#include "nsNetUtil.h"








































#define PREF_BDA_DONTCLEAN "browser.download.antivirus.dontclean"





#ifndef MOZ_VIRUS_SCANNER_PROMPT_GUID
#define MOZ_VIRUS_SCANNER_PROMPT_GUID \
  { 0xb50563d1, 0x16b6, 0x43c2, { 0xa6, 0x6a, 0xfa, 0xe6, 0xd2, 0x11, 0xf2, \
  0xea } }
#endif
static const GUID GUID_MozillaVirusScannerPromptGeneric =
  MOZ_VIRUS_SCANNER_PROMPT_GUID;

nsDownloadScanner::nsDownloadScanner()
  : mHaveAVScanner(PR_FALSE), mHaveAttachmentExecute(PR_FALSE)
{
}

nsresult
nsDownloadScanner::Init()
{
  
  
  nsresult rv = NS_OK;
  CoInitialize(NULL);
  if (!IsAESAvailable() && ListCLSID() < 0)
    rv = NS_ERROR_NOT_AVAILABLE;
  CoUninitialize();

  return rv;
}

PRBool
nsDownloadScanner::IsAESAvailable()
{
  nsRefPtr<IAttachmentExecute> ae;
  HRESULT hr;
  hr = CoCreateInstance(CLSID_AttachmentServices, NULL, CLSCTX_INPROC,
                        IID_IAttachmentExecute, getter_AddRefs(ae));
  if (FAILED(hr)) {
    NS_WARNING("Could not instantiate attachment execution service\n");
    return PR_FALSE;
  }

  mHaveAVScanner = PR_TRUE;
  mHaveAttachmentExecute = PR_TRUE;
  return PR_TRUE;
}

PRInt32
nsDownloadScanner::ListCLSID()
{
  nsRefPtr<ICatInformation> catInfo;
  HRESULT hr;
  hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_INPROC,
                        IID_ICatInformation, getter_AddRefs(catInfo));
  if (FAILED(hr)) {
    NS_WARNING("Could not create category information class\n");
    return -1;
  }
  nsRefPtr<IEnumCLSID> clsidEnumerator;
  GUID guids [1] = { CATID_MSOfficeAntiVirus };
  hr = catInfo->EnumClassesOfCategories(1, guids, 0, NULL,
      getter_AddRefs(clsidEnumerator));
  if (FAILED(hr)) {
    NS_WARNING("Could not get class enumerator for category\n");
    return -2;
  }

  ULONG nReceived;
  CLSID clsid;
  while(clsidEnumerator->Next(1, &clsid, &nReceived) == S_OK && nReceived == 1)
    mScanCLSID.AppendElement(clsid);

  if (mScanCLSID.Length() == 0) {
    
    return -3;
  }

  mHaveAVScanner = PR_TRUE;
  return 0;
}

unsigned int __stdcall
nsDownloadScanner::ScannerThreadFunction(void *p)
{
  NS_ASSERTION(!NS_IsMainThread(), "Antivirus scan should not be run on the main thread");
  nsDownloadScanner::Scan *scan = static_cast<nsDownloadScanner::Scan*>(p);
  scan->DoScan();
  _endthreadex(0);
  return 0;
}

nsDownloadScanner::Scan::Scan(nsDownloadScanner *scanner, nsDownload *download)
  : mDLScanner(scanner), mThread(NULL), 
    mDownload(download), mStatus(AVSCAN_NOTSTARTED)
{
}

nsresult
nsDownloadScanner::Scan::Start()
{
  mThread = (HANDLE)_beginthreadex(NULL, 0, ScannerThreadFunction,
      this, CREATE_SUSPENDED, NULL);
  if (!mThread)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = NS_OK;

  
  mIsReadOnlyRequest = PR_FALSE;

  nsCOMPtr<nsIPrefBranch> pref =
    do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (pref)
    rv = pref->GetBoolPref(PREF_BDA_DONTCLEAN, &mIsReadOnlyRequest);

  
  nsCOMPtr<nsILocalFile> file;
  rv = mDownload->GetTargetFile(getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = file->GetPath(mPath);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIXULAppInfo> appinfo =
    do_GetService(XULAPPINFO_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString name;
  rv = appinfo->GetName(name);
  NS_ENSURE_SUCCESS(rv, rv);
  CopyUTF8toUTF16(name, mName);

  
  nsCOMPtr<nsIURI> uri;
  rv = mDownload->GetSource(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString origin;
  rv = uri->GetSpec(origin);
  NS_ENSURE_SUCCESS(rv, rv);

  CopyUTF8toUTF16(origin, mOrigin);

  
  PRBool isHttp(PR_FALSE), isFtp(PR_FALSE), isHttps(PR_FALSE);
  nsCOMPtr<nsIURI> innerURI = NS_GetInnermostURI(uri);
  (void)innerURI->SchemeIs("http", &isHttp);
  (void)innerURI->SchemeIs("ftp", &isFtp);
  (void)innerURI->SchemeIs("https", &isHttps);
  mIsHttpDownload = isHttp || isFtp || isHttps;

  
  if (1 != ::ResumeThread(mThread)) {
    CloseHandle(mThread);
    return NS_ERROR_UNEXPECTED;
  }
  return NS_OK;
}

nsresult
nsDownloadScanner::Scan::Run()
{
  
  WaitForSingleObject(mThread, INFINITE);
  CloseHandle(mThread);

  DownloadState downloadState = 0;
  switch (mStatus) {
    case AVSCAN_BAD:
      downloadState = nsIDownloadManager::DOWNLOAD_DIRTY;
      break;
    default:
    case AVSCAN_FAILED:
    case AVSCAN_GOOD:
    case AVSCAN_UGLY:
      downloadState = nsIDownloadManager::DOWNLOAD_FINISHED;
      break;
  }
  (void)mDownload->SetState(downloadState);

  NS_RELEASE_THIS();
  return NS_OK;
}

void
nsDownloadScanner::Scan::DoScanAES()
{
  HRESULT hr;
  nsRefPtr<IAttachmentExecute> ae;
  hr = CoCreateInstance(CLSID_AttachmentServices, NULL, CLSCTX_ALL,
                        IID_IAttachmentExecute, getter_AddRefs(ae));

  mStatus = AVSCAN_SCANNING;

  if (SUCCEEDED(hr)) {
    (void)ae->SetClientGuid(GUID_MozillaVirusScannerPromptGeneric);
    (void)ae->SetLocalPath(mPath.BeginWriting());
    (void)ae->SetSource(mOrigin.BeginWriting());

    
    hr = ae->Save();

    if (SUCCEEDED(hr)) { 
      mStatus = AVSCAN_GOOD;
    }
    else if (HRESULT_CODE(hr) == ERROR_FILE_NOT_FOUND) {
      NS_WARNING("Downloaded file disappeared before it could be scanned");
      mStatus = AVSCAN_FAILED;
    }
    else { 
      mStatus = AVSCAN_UGLY;
    }
  }
  else {
    mStatus = AVSCAN_FAILED;
  }
}

void
nsDownloadScanner::Scan::DoScanOAV()
{
  HRESULT hr;
  MSOAVINFO info;
  info.cbsize = sizeof(MSOAVINFO);
  info.fPath = TRUE;
  info.fInstalled = FALSE;
  info.fReadOnlyRequest = mIsReadOnlyRequest;
  info.fHttpDownload = mIsHttpDownload;
  info.hwnd = NULL;

  info.pwzHostName = mName.BeginWriting();
  info.u.pwzFullPath = mPath.BeginWriting();
  info.pwzOrigURL = mOrigin.BeginWriting();

  for (PRUint32 i = 0; i < mDLScanner->mScanCLSID.Length(); i++) {
    nsRefPtr<IOfficeAntiVirus> vScanner;
    hr = CoCreateInstance(mDLScanner->mScanCLSID[i], NULL, CLSCTX_ALL,
                          IID_IOfficeAntiVirus, getter_AddRefs(vScanner));
    if (FAILED(hr)) {
      NS_WARNING("Could not instantiate antivirus scanner");
      mStatus = AVSCAN_FAILED;
    } else {
      mStatus = AVSCAN_SCANNING;

      hr = vScanner->Scan(&info);

      if (hr == S_OK) { 
        mStatus = AVSCAN_GOOD;
        continue;
      }
      else if (hr == S_FALSE) { 
        mStatus = AVSCAN_UGLY;
        continue;
      }
      else if (HRESULT_CODE(hr) == ERROR_FILE_NOT_FOUND) {
        NS_WARNING("Downloaded file disappeared before it could be scanned");
        mStatus = AVSCAN_FAILED;
        break;
      }
      else if (hr == E_FAIL) { 
        mStatus = AVSCAN_BAD;
        break;
      }
      else {
        mStatus = AVSCAN_FAILED;
        break;
      }
    }
  }
}

void
nsDownloadScanner::Scan::DoScan()
{
  CoInitialize(NULL);

  if (mDLScanner->mHaveAttachmentExecute)
    DoScanAES();
  else
    DoScanOAV();

  CoUninitialize();

  
  NS_DispatchToMainThread(this);
}

nsresult
nsDownloadScanner::ScanDownload(nsDownload *download)
{
  if (!mHaveAVScanner)
    return NS_ERROR_NOT_AVAILABLE;

  
  Scan *scan = new Scan(this, download);
  if (!scan)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(scan);

  nsresult rv = scan->Start();

  
  
  if (NS_FAILED(rv))
    NS_RELEASE(scan);

  return rv;
}
