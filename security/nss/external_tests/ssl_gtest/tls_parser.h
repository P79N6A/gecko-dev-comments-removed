





#ifndef tls_parser_h_
#define tls_parser_h_

#include <memory>
#include <cstdint>
#include <cstring>
#include <arpa/inet.h>
#include "databuffer.h"

namespace nss_test {

const uint8_t kTlsChangeCipherSpecType = 20;
const uint8_t kTlsAlertType = 21;
const uint8_t kTlsHandshakeType = 22;

const uint8_t kTlsHandshakeClientHello = 1;
const uint8_t kTlsHandshakeServerHello = 2;
const uint8_t kTlsHandshakeCertificate = 11;
const uint8_t kTlsHandshakeServerKeyExchange = 12;

const uint8_t kTlsAlertWarning = 1;
const uint8_t kTlsAlertFatal = 2;

const uint8_t kTlsAlertUnexpectedMessage = 10;
const uint8_t kTlsAlertHandshakeFailure = 40;
const uint8_t kTlsAlertIllegalParameter = 47;
const uint8_t kTlsAlertDecodeError = 50;
const uint8_t kTlsAlertUnsupportedExtension = 110;
const uint8_t kTlsAlertNoApplicationProtocol = 120;

const uint8_t kTlsFakeChangeCipherSpec[] = {
    kTlsChangeCipherSpecType,        
    0xfe,                     0xff,  
    0x00,                     0x00, 0x00, 0x00,
    0x00,                     0x00, 0x00, 0x10,  
    0x00,                     0x01,              
    0x01                                         
};

inline bool IsDtls(uint16_t version) {
  return (version & 0x8000) == 0x8000;
}

inline uint16_t NormalizeTlsVersion(uint16_t version) {
  if (version == 0xfeff) {
    return 0x0302; 
  }
  if (IsDtls(version)) {
    return (version ^ 0xffff) + 0x0201;
  }
  return version;
}

inline void WriteVariable(DataBuffer* target, size_t index,
                          const DataBuffer& buf, size_t len_size) {
  target->Write(index, static_cast<uint32_t>(buf.len()), len_size);
  target->Write(index + len_size, buf.data(), buf.len());
}

class TlsParser {
 public:
  TlsParser(const uint8_t* data, size_t len)
      : buffer_(data, len), offset_(0) {}
  explicit TlsParser(const DataBuffer& buf)
      : buffer_(buf), offset_(0) {}

  bool Read(uint8_t* val);
  
  bool Read(uint32_t* val, size_t size);
  
  bool Read(DataBuffer* dest, size_t len);
  
  
  bool ReadVariable(DataBuffer* dest, size_t len_size);

  bool Skip(size_t len);
  bool SkipVariable(size_t len_size);

  size_t consumed() const { return offset_; }
  size_t remaining() const { return buffer_.len() - offset_; }

 private:
  void consume(size_t len) { offset_ += len; }
  const uint8_t* ptr() const { return buffer_.data() + offset_; }

  DataBuffer buffer_;
  size_t offset_;
};

} 

#endif
