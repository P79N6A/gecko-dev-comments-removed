




































#ifndef ManifestParser_h
#define ManifestParser_h

#include "nsComponentManager.h"
#include "nsChromeRegistry.h"
#include "mozilla/FileLocation.h"

class nsILocalFile;

void ParseManifest(NSLocationType type, mozilla::FileLocation &file,
                   char* buf, bool aChromeOnly);

void LogMessage(const char* aMsg, ...);

void LogMessageWithContext(mozilla::FileLocation &aFile,
                           PRUint32 aLineNumber, const char* aMsg, ...);

#endif 
