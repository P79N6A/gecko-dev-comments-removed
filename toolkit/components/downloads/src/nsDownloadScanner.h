

#ifndef nsDownloadScanner_h_
#define nsDownloadScanner_h_

#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif
#define INITGUID
#include <Windows.h>
#define AVVENDOR
#include <msoav.h>

#ifdef _WIN32_IE_IE60SP2
#undef _WIN32_IE
#define _WIN32_IE _WIN32_IE_IE60SP2
#endif
#include <shlobj.h>

#include "nsAutoPtr.h"
#include "nsThreadUtils.h"
#include "nsDownloadManager.h"
#include "nsTArray.h"

enum AVScanState
{
  AVSCAN_NOTSTARTED = 0,
  AVSCAN_SCANNING,
  AVSCAN_GOOD,
  AVSCAN_BAD,
  AVSCAN_UGLY,
  AVSCAN_FAILED,
  AVSCAN_TIMEDOUT
};


class nsDownloadScannerWatchdog;

class nsDownloadScanner
{
public:
  nsDownloadScanner();
  ~nsDownloadScanner();
  nsresult Init();
  nsresult ScanDownload(nsDownload *download);

private:
  PRBool mHaveAVScanner;
  PRBool mHaveAttachmentExecute;
  nsTArray<CLSID> mScanCLSID;
  PRBool IsAESAvailable();
  PRInt32 ListCLSID();

  nsAutoPtr<nsDownloadScannerWatchdog> mWatchdog;

  static unsigned int __stdcall ScannerThreadFunction(void *p);
  class Scan : public nsRunnable
  {
  public:
    Scan(nsDownloadScanner *scanner, nsDownload *download);
    ~Scan();
    nsresult Start();

    
    PRTime GetStartTime() const { return mStartTime; }
    
    
    
    
    
    HANDLE GetWaitableThreadHandle() const;

    
    
    PRBool NotifyTimeout();

  private:
    nsDownloadScanner *mDLScanner;
    PRTime mStartTime;
    HANDLE mThread;
    nsRefPtr<nsDownload> mDownload;
    
    CRITICAL_SECTION mStateSync;
    AVScanState mStatus;
    nsString mPath;
    nsString mName;
    nsString mOrigin;
    
    PRBool mIsHttpDownload;
    PRBool mIsReadOnlyRequest;

    





    PRBool CheckAndSetState(AVScanState newState, AVScanState expectedState);

    NS_IMETHOD Run();

    void DoScan();
    PRBool DoScanAES();
    PRBool DoScanOAV();

    friend unsigned int __stdcall nsDownloadScanner::ScannerThreadFunction(void *);
  };
  
  friend class nsDownloadScannerWatchdog;
};
#endif
