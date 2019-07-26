








#ifndef CocoaFileUtils_h_
#define CocoaFileUtils_h_

#include "nscore.h"
#include <CoreFoundation/CoreFoundation.h>

namespace CocoaFileUtils {

nsresult RevealFileInFinder(CFURLRef aUrl);
nsresult OpenURL(CFURLRef aUrl);
nsresult GetFileCreatorCode(CFURLRef aUrl, OSType* aCreatorCode);
nsresult SetFileCreatorCode(CFURLRef aUrl, OSType aCreatorCode);
nsresult GetFileTypeCode(CFURLRef aUrl, OSType* aTypeCode);
nsresult SetFileTypeCode(CFURLRef aUrl, OSType aTypeCode);

} 

#endif
