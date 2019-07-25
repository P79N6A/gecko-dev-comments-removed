




































#ifndef ManifestParser_h
#define ManifestParser_h

#include "nsComponentManager.h"
#include "nsChromeRegistry.h"

class nsILocalFile;

void ParseManifest(NSLocationType type, nsILocalFile* file, char* buf);

#endif 
