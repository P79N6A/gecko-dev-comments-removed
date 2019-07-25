







































#ifndef nsAppStartup_h__
#define nsAppStartup_h__

#include "nsIAppStartup.h"
#include "nsIWindowCreator2.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"

#include "nsINativeAppSupport.h"
#include "nsIAppShell.h"

#if defined(XP_WIN)

#include "mozilla/perfprobe.h"
#include "nsAutoPtr.h"
#endif 


struct PLEvent;


#define NS_TOOLKIT_APPSTARTUP_CID \
{ 0x7dd4d320, 0xc84b, 0x4624, { 0x8d, 0x45, 0x7b, 0xb9, 0xb2, 0x35, 0x69, 0x77 } }


class nsAppStartup : public nsIAppStartup,
                     public nsIWindowCreator2,
                     public nsIObserver,
                     public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIAPPSTARTUP
  NS_DECL_NSIWINDOWCREATOR
  NS_DECL_NSIWINDOWCREATOR2
  NS_DECL_NSIOBSERVER

  nsAppStartup();
  nsresult Init();

private:
  ~nsAppStartup() { }

  void CloseAllWindows();

  friend class nsAppExitEvent;

  nsCOMPtr<nsIAppShell> mAppShell;

  PRInt32      mConsiderQuitStopper; 
  bool mRunning;        
  bool mShuttingDown;   
  bool mAttemptingQuit; 
  bool mRestart;        
  bool mInterrupted;    
  bool mIsSafeModeNecessary;       
  bool mStartupCrashTrackingEnded; 

#if defined(XP_WIN)
  
  typedef mozilla::probes::ProbeManager ProbeManager;
  typedef mozilla::probes::Probe        Probe;
  nsRefPtr<ProbeManager> mProbesManager;
  nsRefPtr<Probe> mPlacesInitCompleteProbe;
  nsRefPtr<Probe> mSessionWindowRestoredProbe;
#endif
};

#endif 
