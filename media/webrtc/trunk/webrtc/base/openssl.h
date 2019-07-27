









#ifndef WEBRTC_BASE_OPENSSL_H_
#define WEBRTC_BASE_OPENSSL_H_

#include <openssl/ssl.h>

#if (OPENSSL_VERSION_NUMBER < 0x10000000L)
#error OpenSSL is older than 1.0.0, which is the minimum supported version.
#endif

#endif  
