



#ifndef BASE_MAC_UTIL_H_
#define BASE_MAC_UTIL_H_

struct FSRef;
class FilePath;

#ifdef __OBJC__
@class NSBundle;
#else
class NSBundle;
#endif

#include <string>

namespace mac_util {

std::string PathFromFSRef(const FSRef& ref);
bool FSRefFromPath(const std::string& path, FSRef* ref);


bool AmIBundled();




NSBundle* MainAppBundle();



void SetOverrideAppBundle(NSBundle* bundle);
void SetOverrideAppBundlePath(const FilePath& file_path);

}  

#endif  
