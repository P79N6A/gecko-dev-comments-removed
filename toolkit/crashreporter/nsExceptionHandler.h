




































#ifndef nsExceptionHandler_h__
#define nsExceptionHandler_h__

#include "nscore.h"
#include "nsXPCOM.h"
#include "nsStringGlue.h"

namespace CrashReporter {
nsresult SetExceptionHandler(nsILocalFile* aXREDirectory,
                             const char* aServerURL);
nsresult SetMinidumpPath(const nsAString& aPath);
nsresult UnsetExceptionHandler();
nsresult AnnotateCrashReport(const nsACString &key, const nsACString &data);
nsresult SetRestartArgs(int argc, char **argv);
nsresult SetupExtraData(nsILocalFile* aAppDataDirectory,
                        const nsACString& aBuildID);
}

#endif 
