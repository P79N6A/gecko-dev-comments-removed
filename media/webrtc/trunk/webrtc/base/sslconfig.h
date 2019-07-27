









#ifndef WEBRTC_BASE_SSLCONFIG_H_
#define WEBRTC_BASE_SSLCONFIG_H_



#if !defined(SSL_USE_SCHANNEL) && !defined(SSL_USE_OPENSSL) && \
    !defined(SSL_USE_NSS)
#if defined(WEBRTC_WIN)

#define SSL_USE_SCHANNEL 1

#else  

#if defined(HAVE_OPENSSL_SSL_H)
#define SSL_USE_OPENSSL 1
#elif defined(HAVE_NSS_SSL_H)
#define SSL_USE_NSS 1
#endif

#endif  
#endif

#endif  
