





#include "tls_parser.h"


#define CHECK_LENGTH(expected)                \
  do {                                        \
    if (remaining() < expected) return false; \
  } while (0)

bool TlsParser::Read(unsigned char* val) {
  if (remaining() < 1) {
    return false;
  }
  *val = *ptr();
  consume(1);
  return true;
}

bool TlsParser::Read(unsigned char* val, size_t len) {
  if (remaining() < len) {
    return false;
  }

  if (val) {
    memcpy(val, ptr(), len);
  }
  consume(len);

  return true;
}

bool TlsRecordParser::NextRecord(uint8_t* ct,
                                 std::auto_ptr<DataBuffer>* buffer) {
  if (!remaining()) return false;

  CHECK_LENGTH(5U);
  const uint8_t* ctp = reinterpret_cast<const uint8_t*>(ptr());
  consume(3);  

  const uint16_t* tmp = reinterpret_cast<const uint16_t*>(ptr());
  size_t length = ntohs(*tmp);
  consume(2);

  CHECK_LENGTH(length);
  DataBuffer* db = new DataBuffer(ptr(), length);
  consume(length);

  *ct = *ctp;
  buffer->reset(db);

  return true;
}
