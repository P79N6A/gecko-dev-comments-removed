




#ifndef ManifestParser_h
#define ManifestParser_h

#include "nsComponentManager.h"
#include "nsChromeRegistry.h"
#include "mozilla/FileLocation.h"

class nsIFile;

void ParseManifest(NSLocationType aType, mozilla::FileLocation& aFile,
                   char* aBuf, bool aChromeOnly, bool aXPTOnly = false);

void LogMessage(const char* aMsg, ...);

void LogMessageWithContext(mozilla::FileLocation& aFile,
                           uint32_t aLineNumber, const char* aMsg, ...);

#endif 
