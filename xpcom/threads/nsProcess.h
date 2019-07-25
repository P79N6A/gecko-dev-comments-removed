




#ifndef _nsPROCESSWIN_H_
#define _nsPROCESSWIN_H_

#if defined(XP_WIN)
#define PROCESSMODEL_WINAPI
#endif

#include "mozilla/Attributes.h"
#include "mozilla/Mutex.h"
#include "nsIProcess.h"
#include "nsIFile.h"
#include "nsIThread.h"
#include "nsIObserver.h"
#include "nsIWeakReferenceUtils.h"
#include "nsIObserver.h"
#include "nsString.h"
#ifndef XP_MACOSX
#include "prproces.h"
#endif
#if defined(PROCESSMODEL_WINAPI)
#include <windows.h>
#include <shellapi.h>
#endif

#define NS_PROCESS_CID \
{0x7b4eeb20, 0xd781, 0x11d4, \
   {0x8A, 0x83, 0x00, 0x10, 0xa4, 0xe0, 0xc9, 0xca}}

class nsProcess MOZ_FINAL : public nsIProcess,
                            public nsIObserver
{
public:

  NS_DECL_ISUPPORTS
  NS_DECL_NSIPROCESS
  NS_DECL_NSIOBSERVER

  nsProcess();

private:
  ~nsProcess();
  static void PR_CALLBACK Monitor(void *arg);
  void ProcessComplete();
  nsresult CopyArgsAndRunProcess(bool blocking, const char** args,
                                 uint32_t count, nsIObserver* observer,
                                 bool holdWeak);
  nsresult CopyArgsAndRunProcessw(bool blocking, const PRUnichar** args,
                                  uint32_t count, nsIObserver* observer,
                                  bool holdWeak);
  
  nsresult RunProcess(bool blocking, char **args, nsIObserver* observer,
                      bool holdWeak, bool argsUTF8);

  PRThread* mThread;
  mozilla::Mutex mLock;
  bool mShutdown;
  bool mBlocking;

  nsCOMPtr<nsIFile> mExecutable;
  nsString mTargetPath;
  int32_t mPid;
  nsCOMPtr<nsIObserver> mObserver;
  nsWeakPtr mWeakObserver;

  
  
  int32_t mExitValue;
#if defined(PROCESSMODEL_WINAPI)
  HANDLE mProcess;
#elif !defined(XP_MACOSX)
  PRProcess *mProcess;
#endif
};

#endif
