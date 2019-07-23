






































#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_LONGHORN

#include "nsDownloadScanner.h"
#include <comcat.h>
#include <process.h>
#include "nsDownloadManager.h"
#include "nsIXULAppInfo.h"
#include "nsXULAppAPI.h"
#include "nsIPrefService.h"
#include "nsNetUtil.h"
#include "nsDeque.h"
#include "nsIFileURL.h"
#include "nsIPrefBranch2.h"
#include "nsXPCOMCIDInternal.h"



























































































#ifndef MOZ_VIRUS_SCANNER_PROMPT_GUID
#define MOZ_VIRUS_SCANNER_PROMPT_GUID \
  { 0xb50563d1, 0x16b6, 0x43c2, { 0xa6, 0x6a, 0xfa, 0xe6, 0xd2, 0x11, 0xf2, \
  0xea } }
#endif
static const GUID GUID_MozillaVirusScannerPromptGeneric =
  MOZ_VIRUS_SCANNER_PROMPT_GUID;


#define WATCHDOG_TIMEOUT (30*PR_USEC_PER_SEC)


#define MAX_IAEURILENGTH 1683

class nsDownloadScannerWatchdog 
{
  typedef nsDownloadScanner::Scan Scan;
public:
  nsDownloadScannerWatchdog();
  ~nsDownloadScannerWatchdog();

  nsresult Init();
  nsresult Shutdown();

  void Watch(Scan *scan);
private:
  static unsigned int __stdcall WatchdogThread(void *p);
  CRITICAL_SECTION mQueueSync;
  nsDeque mScanQueue;
  HANDLE mThread;
  HANDLE mNewItemEvent;
  HANDLE mQuitEvent;
};

nsDownloadScanner::nsDownloadScanner() :
  mAESExists(PR_FALSE)
{
}
 




nsDownloadScanner::~nsDownloadScanner() {
  if (mWatchdog)
    (void)mWatchdog->Shutdown();
}

nsresult
nsDownloadScanner::Init()
{
  
  
  nsresult rv = NS_OK;
  CoInitialize(NULL);

  if (!IsAESAvailable()) {
    CoUninitialize();
    return NS_ERROR_NOT_AVAILABLE;
  }

  mAESExists = PR_TRUE;

  
  mWatchdog = new nsDownloadScannerWatchdog();
  if (mWatchdog) {
    rv = mWatchdog->Init();
    if (FAILED(rv))
      mWatchdog = nsnull;
  } else {
    rv = NS_ERROR_OUT_OF_MEMORY;
  }
  
  if (NS_FAILED(rv))
    return rv;

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
  return PR_TRUE;
}



AVCheckPolicyState
nsDownloadScanner::CheckPolicy(nsIURI *aSource, nsIURI *aTarget)
{
  nsresult rv;

  if (!mAESExists || !aSource || !aTarget)
    return AVPOLICY_DOWNLOAD;

  nsCAutoString source;
  rv = aSource->GetSpec(source);
  if (NS_FAILED(rv))
    return AVPOLICY_DOWNLOAD;

  nsCOMPtr<nsIFileURL> fileUrl(do_QueryInterface(aTarget));
  if (!fileUrl)
    return AVPOLICY_DOWNLOAD;

  nsCOMPtr<nsIFile> theFile;
  nsAutoString aFileName;
  if (NS_FAILED(fileUrl->GetFile(getter_AddRefs(theFile))) ||
      NS_FAILED(theFile->GetLeafName(aFileName)))
    return AVPOLICY_DOWNLOAD;

  
  
  
  PRBool isDataScheme(PR_FALSE);
  nsCOMPtr<nsIURI> innerURI = NS_GetInnermostURI(aSource);
  if (innerURI)
    (void)innerURI->SchemeIs("data", &isDataScheme);
  if (isDataScheme)
    return AVPOLICY_DOWNLOAD;

  nsRefPtr<IAttachmentExecute> ae;
  HRESULT hr;
  hr = CoCreateInstance(CLSID_AttachmentServices, NULL, CLSCTX_INPROC,
                        IID_IAttachmentExecute, getter_AddRefs(ae));
  if (FAILED(hr))
    return AVPOLICY_DOWNLOAD;

  (void)ae->SetClientGuid(GUID_MozillaVirusScannerPromptGeneric);
  (void)ae->SetSource(NS_ConvertUTF8toUTF16(source).get());
  (void)ae->SetFileName(aFileName.get());

  
  
  hr = ae->CheckPolicy();

  if (hr == S_OK)
    return AVPOLICY_DOWNLOAD;

  if (hr == S_FALSE)
    return AVPOLICY_PROMPT;

  if (hr == E_INVALIDARG)
    return AVPOLICY_PROMPT;

  return AVPOLICY_BLOCKED;
}

#ifndef THREAD_MODE_BACKGROUND_BEGIN
#define THREAD_MODE_BACKGROUND_BEGIN 0x00010000
#endif

#ifndef THREAD_MODE_BACKGROUND_END
#define THREAD_MODE_BACKGROUND_END 0x00020000
#endif

unsigned int __stdcall
nsDownloadScanner::ScannerThreadFunction(void *p)
{
  HANDLE currentThread = GetCurrentThread();
  NS_ASSERTION(!NS_IsMainThread(), "Antivirus scan should not be run on the main thread");
  nsDownloadScanner::Scan *scan = static_cast<nsDownloadScanner::Scan*>(p);
  if (!SetThreadPriority(currentThread, THREAD_MODE_BACKGROUND_BEGIN))
    (void)SetThreadPriority(currentThread, THREAD_PRIORITY_IDLE);
  scan->DoScan();
  (void)SetThreadPriority(currentThread, THREAD_MODE_BACKGROUND_END);
  _endthreadex(0);
  return 0;
}




class ReleaseDispatcher : public nsRunnable {
public:
  ReleaseDispatcher(nsISupports *ptr)
    : mPtr(ptr) {}
  NS_IMETHOD Run();
private:
  nsISupports *mPtr;
};

nsresult ReleaseDispatcher::Run() {
  NS_ASSERTION(NS_IsMainThread(), "Antivirus scan release dispatch should be run on the main thread");
  NS_RELEASE(mPtr);
  NS_RELEASE_THIS();
  return NS_OK;
}

nsDownloadScanner::Scan::Scan(nsDownloadScanner *scanner, nsDownload *download)
  : mDLScanner(scanner), mThread(NULL), 
    mDownload(download), mStatus(AVSCAN_NOTSTARTED),
    mSkipSource(PR_FALSE)
{
  InitializeCriticalSection(&mStateSync);
}

nsDownloadScanner::Scan::~Scan() {
  DeleteCriticalSection(&mStateSync);
}

nsresult
nsDownloadScanner::Scan::Start()
{
  mStartTime = PR_Now();

  mThread = (HANDLE)_beginthreadex(NULL, 0, ScannerThreadFunction,
      this, CREATE_SUSPENDED, NULL);
  if (!mThread)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = NS_OK;

  
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

  
  
  if (origin.Length() > MAX_IAEURILENGTH) {
    rv = uri->GetPrePath(origin);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  CopyUTF8toUTF16(origin, mOrigin);

  
  PRBool isHttp(PR_FALSE), isFtp(PR_FALSE), isHttps(PR_FALSE);
  nsCOMPtr<nsIURI> innerURI = NS_GetInnermostURI(uri);
  if (!innerURI) innerURI = uri;
  (void)innerURI->SchemeIs("http", &isHttp);
  (void)innerURI->SchemeIs("ftp", &isFtp);
  (void)innerURI->SchemeIs("https", &isHttps);
  mIsHttpDownload = isHttp || isFtp || isHttps;

  
  
  
  
  (void)innerURI->SchemeIs("data", &mSkipSource);

  
  if (1 != ::ResumeThread(mThread)) {
    CloseHandle(mThread);
    return NS_ERROR_UNEXPECTED;
  }
  return NS_OK;
}

nsresult
nsDownloadScanner::Scan::Run()
{
  NS_ASSERTION(NS_IsMainThread(), "Antivirus scan dispatch should be run on the main thread");

  
  if (mStatus != AVSCAN_TIMEDOUT)
    WaitForSingleObject(mThread, INFINITE);
  CloseHandle(mThread);

  DownloadState downloadState = 0;
  EnterCriticalSection(&mStateSync);
  switch (mStatus) {
    case AVSCAN_BAD:
      downloadState = nsIDownloadManager::DOWNLOAD_DIRTY;
      break;
    default:
    case AVSCAN_FAILED:
    case AVSCAN_GOOD:
    case AVSCAN_UGLY:
    case AVSCAN_TIMEDOUT:
      downloadState = nsIDownloadManager::DOWNLOAD_FINISHED;
      break;
  }
  LeaveCriticalSection(&mStateSync);
  
  if (mDownload)
    (void)mDownload->SetState(downloadState);

  
  
  mDownload = nsnull;

  NS_RELEASE_THIS();
  return NS_OK;
}

static DWORD
ExceptionFilterFunction(DWORD exceptionCode) {
  switch(exceptionCode) {
    case EXCEPTION_ACCESS_VIOLATION:
    case EXCEPTION_ILLEGAL_INSTRUCTION:
    case EXCEPTION_IN_PAGE_ERROR:
    case EXCEPTION_PRIV_INSTRUCTION:
    case EXCEPTION_STACK_OVERFLOW:
      return EXCEPTION_EXECUTE_HANDLER;
    default:
      return EXCEPTION_CONTINUE_SEARCH;
  }
}

PRBool
nsDownloadScanner::Scan::DoScanAES()
{
  
  
#pragma warning(disable: 4509)
  HRESULT hr;
  nsRefPtr<IAttachmentExecute> ae;
  __try {
    hr = CoCreateInstance(CLSID_AttachmentServices, NULL, CLSCTX_ALL,
                          IID_IAttachmentExecute, getter_AddRefs(ae));
  } __except(ExceptionFilterFunction(GetExceptionCode())) {
    return CheckAndSetState(AVSCAN_NOTSTARTED,AVSCAN_FAILED);
  }

  
  if (CheckAndSetState(AVSCAN_SCANNING, AVSCAN_NOTSTARTED)) {
    AVScanState newState;
    if (SUCCEEDED(hr)) {
      PRBool gotException = PR_FALSE;
      __try {
        (void)ae->SetClientGuid(GUID_MozillaVirusScannerPromptGeneric);
        (void)ae->SetLocalPath(mPath.BeginWriting());
        
        if (!mSkipSource)
          (void)ae->SetSource(mOrigin.BeginWriting());

        
        hr = ae->Save();
      } __except(ExceptionFilterFunction(GetExceptionCode())) {
        gotException = PR_TRUE;
      }

      __try {
        ae = NULL;
      } __except(ExceptionFilterFunction(GetExceptionCode())) {
        gotException = PR_TRUE;
      }

      if(gotException) {
        newState = AVSCAN_FAILED;
      }
      else if (SUCCEEDED(hr)) { 
        newState = AVSCAN_GOOD;
      }
      else if (HRESULT_CODE(hr) == ERROR_FILE_NOT_FOUND) {
        NS_WARNING("Downloaded file disappeared before it could be scanned");
        newState = AVSCAN_FAILED;
      }
      else if (hr == E_INVALIDARG) {
        NS_WARNING("IAttachementExecute returned invalid argument error");
        newState = AVSCAN_FAILED;
      }
      else { 
        newState = AVSCAN_UGLY;
      }
    }
    else {
      newState = AVSCAN_FAILED;
    }
    return CheckAndSetState(newState, AVSCAN_SCANNING);
  }
  return PR_FALSE;
}
#pragma warning(default: 4509)

void
nsDownloadScanner::Scan::DoScan()
{
  CoInitialize(NULL);

  if (DoScanAES()) {
    
    NS_DispatchToMainThread(this);
  } else {
    
    ReleaseDispatcher* releaser = new ReleaseDispatcher(this);
    if(releaser) {
      NS_ADDREF(releaser);
      NS_DispatchToMainThread(releaser);
    }
  }

  __try {
    CoUninitialize();
  } __except(ExceptionFilterFunction(GetExceptionCode())) {
    
  }
}

HANDLE
nsDownloadScanner::Scan::GetWaitableThreadHandle() const
{
  HANDLE targetHandle = INVALID_HANDLE_VALUE;
  (void)DuplicateHandle(GetCurrentProcess(), mThread,
                        GetCurrentProcess(), &targetHandle,
                        SYNCHRONIZE, 
                        FALSE, 
                        0);
  return targetHandle;
}

PRBool
nsDownloadScanner::Scan::NotifyTimeout()
{
  PRBool didTimeout = CheckAndSetState(AVSCAN_TIMEDOUT, AVSCAN_SCANNING) ||
                      CheckAndSetState(AVSCAN_TIMEDOUT, AVSCAN_NOTSTARTED);
  if (didTimeout) {
    
    NS_DispatchToMainThread(this);
  }
  return didTimeout;
}

PRBool
nsDownloadScanner::Scan::CheckAndSetState(AVScanState newState, AVScanState expectedState) {
  PRBool gotExpectedState = PR_FALSE;
  EnterCriticalSection(&mStateSync);
  if(gotExpectedState = (mStatus == expectedState))
    mStatus = newState;
  LeaveCriticalSection(&mStateSync);
  return gotExpectedState;
}

nsresult
nsDownloadScanner::ScanDownload(nsDownload *download)
{
  if (!mAESExists)
    return NS_ERROR_NOT_AVAILABLE;

  
  Scan *scan = new Scan(this, download);
  if (!scan)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(scan);

  nsresult rv = scan->Start();

  
  
  if (NS_FAILED(rv))
    NS_RELEASE(scan);
  else
    
    mWatchdog->Watch(scan);

  return rv;
}

nsDownloadScannerWatchdog::nsDownloadScannerWatchdog() 
  : mNewItemEvent(NULL), mQuitEvent(NULL) {
  InitializeCriticalSection(&mQueueSync);
}
nsDownloadScannerWatchdog::~nsDownloadScannerWatchdog() {
  DeleteCriticalSection(&mQueueSync);
}

nsresult
nsDownloadScannerWatchdog::Init() {
  
  mNewItemEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  if (INVALID_HANDLE_VALUE == mNewItemEvent)
    return NS_ERROR_OUT_OF_MEMORY;
  mQuitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  if (INVALID_HANDLE_VALUE == mQuitEvent) {
    (void)CloseHandle(mNewItemEvent);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  
  mThread = (HANDLE)_beginthreadex(NULL, 0, WatchdogThread,
                                   this, 0, NULL);
  if (!mThread) {
    (void)CloseHandle(mNewItemEvent);
    (void)CloseHandle(mQuitEvent);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

nsresult
nsDownloadScannerWatchdog::Shutdown() {
  
  (void)SetEvent(mQuitEvent);
  (void)WaitForSingleObject(mThread, INFINITE);
  (void)CloseHandle(mThread);
  
  while (mScanQueue.GetSize() != 0) {
    Scan *scan = reinterpret_cast<Scan*>(mScanQueue.Pop());
    NS_RELEASE(scan);
  }
  (void)CloseHandle(mNewItemEvent);
  (void)CloseHandle(mQuitEvent);
  return NS_OK;
}

void
nsDownloadScannerWatchdog::Watch(Scan *scan) {
  PRBool wasEmpty;
  
  
  
  
  NS_ADDREF(scan);
  EnterCriticalSection(&mQueueSync);
  wasEmpty = mScanQueue.GetSize()==0;
  mScanQueue.Push(scan);
  LeaveCriticalSection(&mQueueSync);
  
  if (wasEmpty)
    (void)SetEvent(mNewItemEvent);
}

unsigned int
__stdcall
nsDownloadScannerWatchdog::WatchdogThread(void *p) {
  NS_ASSERTION(!NS_IsMainThread(), "Antivirus scan watchdog should not be run on the main thread");
  nsDownloadScannerWatchdog *watchdog = (nsDownloadScannerWatchdog*)p;
  HANDLE waitHandles[3] = {watchdog->mNewItemEvent, watchdog->mQuitEvent, INVALID_HANDLE_VALUE};
  DWORD waitStatus;
  DWORD queueItemsLeft = 0;
  
  while (0 != queueItemsLeft ||
         (WAIT_OBJECT_0 + 1) !=
           (waitStatus =
              WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE)) &&
         waitStatus != WAIT_FAILED) {
    Scan *scan = NULL;
    PRTime startTime, expectedEndTime, now;
    DWORD waitTime;

    
    EnterCriticalSection(&watchdog->mQueueSync);
    scan = reinterpret_cast<Scan*>(watchdog->mScanQueue.Pop());
    queueItemsLeft = watchdog->mScanQueue.GetSize();
    LeaveCriticalSection(&watchdog->mQueueSync);

    
    startTime = scan->GetStartTime();
    expectedEndTime = WATCHDOG_TIMEOUT + startTime;
    now = PR_Now();
    
    
    if (now > expectedEndTime) {
      waitTime = 0;
    } else {
      
      
      
      waitTime = static_cast<DWORD>((expectedEndTime - now)/PR_USEC_PER_MSEC);
    }
    HANDLE hThread = waitHandles[2] = scan->GetWaitableThreadHandle();

    
    waitStatus = WaitForMultipleObjects(2, (waitHandles+1), FALSE, waitTime);
    CloseHandle(hThread);

    ReleaseDispatcher* releaser = new ReleaseDispatcher(scan);
    if(!releaser)
      continue;
    NS_ADDREF(releaser);
    
    if (waitStatus == WAIT_FAILED || waitStatus == WAIT_OBJECT_0) {
      NS_DispatchToMainThread(releaser);
      break;
    
    } else if (waitStatus == (WAIT_OBJECT_0+1)) {
      NS_DispatchToMainThread(releaser);
      continue;
    
    } else { 
      NS_ASSERTION(waitStatus == WAIT_TIMEOUT, "Unexpected wait status in dlmgr watchdog thread");
      if (!scan->NotifyTimeout()) {
        
        NS_DispatchToMainThread(releaser);
      } else {
        
        
        NS_RELEASE(releaser);
      }
    }
  }
  _endthreadex(0);
  return 0;
}

#endif 
