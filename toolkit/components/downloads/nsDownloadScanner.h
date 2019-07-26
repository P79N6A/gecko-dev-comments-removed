






#ifndef nsDownloadScanner_h_
#define nsDownloadScanner_h_

#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#define AVVENDOR
#include <objidl.h>
#include <msoav.h>
#include <shlobj.h>

#include "nsAutoPtr.h"
#include "nsThreadUtils.h"
#include "nsTArray.h"
#include "nsIObserver.h"
#include "nsIURI.h"

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

enum AVCheckPolicyState
{
  AVPOLICY_DOWNLOAD,
  AVPOLICY_PROMPT,
  AVPOLICY_BLOCKED
};


class nsDownloadScannerWatchdog;
class nsDownload;

class nsDownloadScanner
{
public:
  nsDownloadScanner();
  ~nsDownloadScanner();
  nsresult Init();
  nsresult ScanDownload(nsDownload *download);
  AVCheckPolicyState CheckPolicy(nsIURI *aSource, nsIURI *aTarget);

private:
  bool mAESExists;
  nsTArray<CLSID> mScanCLSID;
  bool IsAESAvailable();
  bool EnumerateOAVProviders();

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

    
    
    bool NotifyTimeout();

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
    
    bool mIsHttpDownload;
    bool mSkipSource;

    





    bool CheckAndSetState(AVScanState newState, AVScanState expectedState);

    NS_IMETHOD Run();

    void DoScan();
    bool DoScanAES();
    bool DoScanOAV();

    friend unsigned int __stdcall nsDownloadScanner::ScannerThreadFunction(void *);
  };
  
  friend class nsDownloadScannerWatchdog;
};
#endif

