









#ifndef WEBRTC_BASE_SSLFINGERPRINT_H_
#define WEBRTC_BASE_SSLFINGERPRINT_H_

#include <string>

#include "webrtc/base/buffer.h"
#include "webrtc/base/sslidentity.h"

namespace rtc {

class SSLCertificate;

struct SSLFingerprint {
  static SSLFingerprint* Create(const std::string& algorithm,
                                const rtc::SSLIdentity* identity);

  static SSLFingerprint* Create(const std::string& algorithm,
                                const rtc::SSLCertificate* cert);

  static SSLFingerprint* CreateFromRfc4572(const std::string& algorithm,
                                           const std::string& fingerprint);

  SSLFingerprint(const std::string& algorithm, const uint8* digest_in,
                 size_t digest_len);

  SSLFingerprint(const SSLFingerprint& from);

  bool operator==(const SSLFingerprint& other) const;

  std::string GetRfc4572Fingerprint() const;

  std::string ToString();

  std::string algorithm;
  rtc::Buffer digest;
};

}  

#endif  
