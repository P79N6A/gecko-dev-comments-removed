









#ifndef WEBRTC_BASE_BYTEBUFFER_H_
#define WEBRTC_BASE_BYTEBUFFER_H_

#include <string>

#include "webrtc/base/basictypes.h"
#include "webrtc/base/constructormagic.h"

namespace rtc {

class ByteBuffer {
 public:

  enum ByteOrder {
    ORDER_NETWORK = 0,  
    ORDER_HOST,         
  };

  
  ByteBuffer();
  explicit ByteBuffer(ByteOrder byte_order);
  ByteBuffer(const char* bytes, size_t len);
  ByteBuffer(const char* bytes, size_t len, ByteOrder byte_order);

  
  explicit ByteBuffer(const char* bytes);

  ~ByteBuffer();

  const char* Data() const { return bytes_ + start_; }
  size_t Length() const { return end_ - start_; }
  size_t Capacity() const { return size_ - start_; }
  ByteOrder Order() const { return byte_order_; }

  
  
  bool ReadUInt8(uint8* val);
  bool ReadUInt16(uint16* val);
  bool ReadUInt24(uint32* val);
  bool ReadUInt32(uint32* val);
  bool ReadUInt64(uint64* val);
  bool ReadBytes(char* val, size_t len);

  
  
  bool ReadString(std::string* val, size_t len);

  
  
  void WriteUInt8(uint8 val);
  void WriteUInt16(uint16 val);
  void WriteUInt24(uint32 val);
  void WriteUInt32(uint32 val);
  void WriteUInt64(uint64 val);
  void WriteString(const std::string& val);
  void WriteBytes(const char* val, size_t len);

  
  
  
  char* ReserveWriteBuffer(size_t len);

  
  
  void Resize(size_t size);

  
  
  
  
  bool Consume(size_t size);

  
  void Clear();

  
  class ReadPosition {
    friend class ByteBuffer;
    ReadPosition(size_t start, int version)
        : start_(start), version_(version) { }
    size_t start_;
    int version_;
  };

  
  
  ReadPosition GetReadPosition() const;

  
  bool SetReadPosition(const ReadPosition &position);

 private:
  void Construct(const char* bytes, size_t size, ByteOrder byte_order);

  char* bytes_;
  size_t size_;
  size_t start_;
  size_t end_;
  int version_;
  ByteOrder byte_order_;

  
  
  DISALLOW_COPY_AND_ASSIGN(ByteBuffer);
};

}  

#endif
