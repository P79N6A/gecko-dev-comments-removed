





#ifndef tls_parser_h_
#define tls_parser_h_

#include <memory>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include "databuffer.h"

const uint8_t kTlsChangeCipherSpecType = 0x14;
const uint8_t kTlsHandshakeType = 0x16;

const uint8_t kTlsHandshakeCertificate = 0x0b;
const uint8_t kTlsHandshakeServerKeyExchange = 0x0c;

const uint8_t kTlsFakeChangeCipherSpec[] = {
    kTlsChangeCipherSpecType,        
    0xfe,                     0xff,  
    0x00,                     0x00, 0x00, 0x00,
    0x00,                     0x00, 0x00, 0x10,  
    0x00,                     0x01,              
    0x01                                         
};

class TlsParser {
 public:
  TlsParser(const unsigned char *data, size_t len)
      : buffer_(data, len), offset_(0) {}

  bool Read(unsigned char *val);

  
  bool Read(uint32_t *val, size_t len) {
    if (len > sizeof(uint32_t)) return false;

    *val = 0;

    for (size_t i = 0; i < len; ++i) {
      unsigned char tmp;

      (*val) <<= 8;
      if (!Read(&tmp)) return false;

      *val += tmp;
    }

    return true;
  }

  bool Read(unsigned char *val, size_t len);
  size_t remaining() const { return buffer_.len() - offset_; }

 private:
  void consume(size_t len) { offset_ += len; }
  const uint8_t *ptr() const { return buffer_.data() + offset_; }

  DataBuffer buffer_;
  size_t offset_;
};

class TlsRecordParser {
 public:
  TlsRecordParser(const unsigned char *data, size_t len)
      : buffer_(data, len), offset_(0) {}

  bool NextRecord(uint8_t *ct, std::auto_ptr<DataBuffer> *buffer);

 private:
  size_t remaining() const { return buffer_.len() - offset_; }
  const uint8_t *ptr() const { return buffer_.data() + offset_; }
  void consume(size_t len) { offset_ += len; }

  DataBuffer buffer_;
  size_t offset_;
};

#endif
