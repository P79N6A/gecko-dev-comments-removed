



#ifndef BASE_MAC_UTIL_H_
#define BASE_MAC_UTIL_H_

struct FSRef;

#include <string>

namespace mac_util {

std::string PathFromFSRef(const FSRef& ref);
bool FSRefFromPath(const std::string& path, FSRef* ref);


bool AmIBundled();

}  

#endif  
