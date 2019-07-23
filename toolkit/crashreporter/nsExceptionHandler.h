




































#ifndef nsExceptionHandler_h__
#define nsExceptionHandler_h__

#include "nscore.h"
#include "nsXPCOM.h"
#include "nsStringGlue.h"

#if defined(XP_WIN32)
#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace CrashReporter {
nsresult SetExceptionHandler(nsILocalFile* aXREDirectory,
                             const char* aServerURL);
nsresult SetMinidumpPath(const nsAString& aPath);
nsresult UnsetExceptionHandler();
nsresult AnnotateCrashReport(const nsACString& key, const nsACString& data);
nsresult AppendAppNotesToCrashReport(const nsACString& data);
nsresult SetRestartArgs(int argc, char** argv);
nsresult SetupExtraData(nsILocalFile* aAppDataDirectory,
                        const nsACString& aBuildID);
#ifdef XP_WIN32
  nsresult WriteMinidumpForException(EXCEPTION_POINTERS* aExceptionInfo);
#endif
}

#endif 
