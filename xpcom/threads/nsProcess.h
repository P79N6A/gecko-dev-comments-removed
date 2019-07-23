






































#ifndef _nsPROCESSWIN_H_
#define _nsPROCESSWIN_H_

#if defined(XP_WIN) && !defined (WINCE) 
#define PROCESSMODEL_WINAPI
#endif

#include "nsIProcess.h"
#include "nsIFile.h"
#include "nsIThread.h"
#include "nsIObserver.h"
#include "nsIWeakReference.h"
#include "nsIWeakReferenceUtils.h"
#include "nsIObserver.h"
#include "nsString.h"
#include "prproces.h"
#if defined(PROCESSMODEL_WINAPI) 
#include <windows.h>
#include <shellapi.h>
#endif

#define NS_PROCESS_CID \
{0x7b4eeb20, 0xd781, 0x11d4, \
   {0x8A, 0x83, 0x00, 0x10, 0xa4, 0xe0, 0xc9, 0xca}}

class nsProcess : public nsIProcess2,
                  public nsIObserver
{
public:

  NS_DECL_ISUPPORTS
  NS_DECL_NSIPROCESS
  NS_DECL_NSIPROCESS2
  NS_DECL_NSIOBSERVER

  nsProcess();

private:
  ~nsProcess();
  static void PR_CALLBACK Monitor(void *arg);
  void ProcessComplete();
  NS_IMETHOD RunProcess(PRBool blocking, const char **args, PRUint32 count,
                        nsIObserver* observer, PRBool holdWeak);

  PRThread* mThread;
  PRLock* mLock;
  PRBool mShutdown;

  nsCOMPtr<nsIFile> mExecutable;
  nsCString mTargetPath;
  PRInt32 mPid;
  nsCOMPtr<nsIObserver> mObserver;
  nsWeakPtr mWeakObserver;

  
  
  PRInt32 mExitValue;
#if defined(PROCESSMODEL_WINAPI) 
  typedef DWORD (WINAPI*GetProcessIdPtr)(HANDLE process);
  HANDLE mProcess;
#else
  PRProcess *mProcess;
#endif
};

#endif
