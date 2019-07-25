









































#ifndef CocoaFileUtils_h_
#define CocoaFileUtils_h_

#include "nscore.h"
#include <CoreFoundation/CoreFoundation.h>

namespace CocoaFileUtils {

nsresult RevealFileInFinder(CFURLRef url);
nsresult OpenURL(CFURLRef url);
nsresult GetFileCreatorCode(CFURLRef url, OSType *creatorCode);
nsresult SetFileCreatorCode(CFURLRef url, OSType creatorCode);
nsresult GetFileTypeCode(CFURLRef url, OSType *typeCode);
nsresult SetFileTypeCode(CFURLRef url, OSType typeCode);

} 

#endif
