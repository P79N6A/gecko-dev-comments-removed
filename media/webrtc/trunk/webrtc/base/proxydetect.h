









#ifndef _PROXYDETECT_H_
#define _PROXYDETECT_H_

#include "webrtc/base/proxyinfo.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

namespace rtc {




bool GetProxySettingsForUrl(const char* agent, const char* url,
                            rtc::ProxyInfo* proxy,
                            bool long_operation = false);

}  

#endif  
