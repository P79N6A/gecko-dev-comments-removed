






































#ifndef _nsPROCESSWIN_H_
#define _nsPROCESSWIN_H_

#if defined(XP_WIN) && !defined (WINCE) 
#define PROCESSMODEL_WINAPI
#endif

#include "nsIProcess.h"
#include "nsIFile.h"
#include "nsString.h"
#include "prproces.h"
#if defined(PROCESSMODEL_WINAPI) 
#include <windows.h>
#endif

#define NS_PROCESS_CID \
{0x7b4eeb20, 0xd781, 0x11d4, \
   {0x8A, 0x83, 0x00, 0x10, 0xa4, 0xe0, 0xc9, 0xca}}

class nsProcess : public nsIProcess
{
public:

  NS_DECL_ISUPPORTS
  NS_DECL_NSIPROCESS

  nsProcess();

private:
  ~nsProcess();

  nsCOMPtr<nsIFile> mExecutable;
  PRInt32 mExitValue;
  nsCString mTargetPath;
  PRProcess *mProcess;

#if defined(PROCESSMODEL_WINAPI) 
  PROCESS_INFORMATION procInfo;
#endif
};

#endif
