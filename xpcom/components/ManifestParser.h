





#ifndef ManifestParser_h
#define ManifestParser_h

#include "nsComponentManager.h"
#if !defined(MOZILLA_XPCOMRT_API)
#include "nsChromeRegistry.h"
#endif 
#include "mozilla/FileLocation.h"

void ParseManifest(NSLocationType aType, mozilla::FileLocation& aFile,
                   char* aBuf, bool aChromeOnly, bool aXPTOnly = false);

void LogMessage(const char* aMsg, ...);

void LogMessageWithContext(mozilla::FileLocation& aFile,
                           uint32_t aLineNumber, const char* aMsg, ...);

#endif 
