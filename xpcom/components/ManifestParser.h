




































#ifndef ManifestParser_h
#define ManifestParser_h

#include "nsComponentManager.h"
#include "nsChromeRegistry.h"

class nsILocalFile;

void ParseManifest(NSLocationType type, nsILocalFile* file,
                   char* buf, bool aChromeOnly);

#ifdef MOZ_OMNIJAR
void ParseManifest(NSLocationType type, const char* jarPath,
                   char* buf, bool aChromeOnly);
#endif

void LogMessage(const char* aMsg, ...);

void LogMessageWithContext(nsILocalFile* aFile, const char* aPath,
                           PRUint32 aLineNumber, const char* aMsg, ...);

#endif 
