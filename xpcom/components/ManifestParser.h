




































#ifndef ManifestParser_h
#define ManifestParser_h

#include "nsComponentManager.h"
#include "nsChromeRegistry.h"

class nsILocalFile;

void ParseManifest(NSLocationType type, nsILocalFile* file, char* buf,
                   bool aChromeOnly);

void LogMessage(const char* aMsg, ...);

void LogMessageWithContext(nsILocalFile* aFile, PRUint32 aLineNumber, const char* aMsg, ...);

#endif 
