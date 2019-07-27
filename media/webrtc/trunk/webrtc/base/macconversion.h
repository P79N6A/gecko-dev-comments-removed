









#ifndef WEBRTC_BASE_MACCONVERSION_H_
#define WEBRTC_BASE_MACCONVERSION_H_

#if defined(WEBRTC_MAC) && !defined(WEBRTC_IOS)

#include <CoreFoundation/CoreFoundation.h>

#include <string>






bool p_convertHostCFStringRefToCPPString(const CFStringRef cfstr,
                                         std::string& cppstr);





bool p_convertCFNumberToInt(CFNumberRef cfn, int* i);


bool p_isCFNumberTrue(CFNumberRef cfn);

#endif  

#endif  
