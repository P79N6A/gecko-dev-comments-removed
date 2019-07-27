





#ifndef tls_parser_h_
#define tls_parser_h_

#include <memory>
#include <cstdint>
#include <cstring>
#include <arpa/inet.h>
#include "databuffer.h"

namespace nss_test {

const uint8_t kTlsChangeCipherSpecType = 0x14;
const uint8_t kTlsAlertType = 0x15;
const uint8_t kTlsHandshakeType = 0x16;

const uint8_t kTlsHandshakeClientHello = 0x01;
const uint8_t kTlsHandshakeServerHello = 0x02;
const uint8_t kTlsHandshakeCertificate = 0x0b;
const uint8_t kTlsHandshakeServerKeyExchange = 0x0c;

const uint8_t kTlsAlertWarning = 1;
const uint8_t kTlsAlertFatal = 2;

const uint8_t kTlsAlertHandshakeFailure = 0x28;
const uint8_t kTlsAlertIllegalParameter = 0x2f;
const uint8_t kTlsAlertDecodeError = 0x32;
const uint8_t kTlsAlertUnsupportedExtension = 0x6e;
const uint8_t kTlsAlertNoApplicationProtocol = 0x78;

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
