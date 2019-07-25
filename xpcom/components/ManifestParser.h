




































#ifndef ManifestParser_h
#define ManifestParser_h

#include "nsComponentManager.h"
#include "nsChromeRegistry.h"

class nsILocalFile;
class nsIZipReader;

void ParseManifest(NSLocationType type, nsILocalFile* file,
                   char* buf, bool aChromeOnly);

void ParseManifest(NSLocationType type, nsIZipReader* reader,
                   const char* jarPath, char* buf, bool aChromeOnly);

void LogMessage(const char* aMsg, ...);

void LogMessageWithContext(nsILocalFile* aFile, const char* aPath,
                           PRUint32 aLineNumber, const char* aMsg, ...);

#endif 
