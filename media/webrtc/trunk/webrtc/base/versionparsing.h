









#ifndef WEBRTC_BASE_VERSIONPARSING_H_
#define WEBRTC_BASE_VERSIONPARSING_H_

#include <string>

namespace rtc {






bool ParseVersionString(const std::string& version_str,
                        int num_expected_segments,
                        int version[]);



int CompareVersions(const int version1[],
                    const int version2[],
                    int num_segments);

}  

#endif  
