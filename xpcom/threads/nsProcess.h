




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

class nsProcess MOZ_FINAL
  : public nsIProcess
  , public nsIObserver
{
public:

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIPROCESS
  NS_DECL_NSIOBSERVER

  nsProcess();

private:
  ~nsProcess();
  static void Monitor(void* aArg);
  void ProcessComplete();
  nsresult CopyArgsAndRunProcess(bool aBlocking, const char** aArgs,
                                 uint32_t aCount, nsIObserver* aObserver,
                                 bool aHoldWeak);
  nsresult CopyArgsAndRunProcessw(bool aBlocking, const char16_t** aArgs,
                                  uint32_t aCount, nsIObserver* aObserver,
                                  bool aHoldWeak);
  
  nsresult RunProcess(bool aBlocking, char** aArgs, nsIObserver* aObserver,
                      bool aHoldWeak, bool aArgsUTF8);

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
  PRProcess* mProcess;
#endif
};

#endif
