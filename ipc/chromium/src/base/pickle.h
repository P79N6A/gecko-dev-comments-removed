



#ifndef BASE_PICKLE_H__
#define BASE_PICKLE_H__

#include <string>

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/string16.h"
#include "testing/gtest/include/gtest/gtest_prod.h"


















class Pickle {
 public:
  ~Pickle();

  
  Pickle();

  
  
  
  explicit Pickle(int header_size);

  
  
  
  
  Pickle(const char* data, int data_len);

  
  Pickle(const Pickle& other);

  
  Pickle& operator=(const Pickle& other);

  
  int size() const { return static_cast<int>(header_size_ +
                                             header_->payload_size); }

  
  const void* data() const { return header_; }

  
  
  
  
  bool ReadBool(void** iter, bool* result) const;
  bool ReadInt16(void** iter, int16_t* result) const;
  bool ReadUInt16(void** iter, uint16_t* result) const;
  bool ReadShort(void** iter, short* result) const;
  bool ReadInt(void** iter, int* result) const;
  bool ReadLong(void** iter, long* result) const;
  bool ReadULong(void** iter, unsigned long* result) const;
  bool ReadSize(void** iter, size_t* result) const;
  bool ReadInt32(void** iter, int32_t* result) const;
  bool ReadUInt32(void** iter, uint32_t* result) const;
  bool ReadInt64(void** iter, int64_t* result) const;
  bool ReadUInt64(void** iter, uint64_t* result) const;
  bool ReadDouble(void** iter, double* result) const;
  bool ReadIntPtr(void** iter, intptr_t* result) const;
  bool ReadUnsignedChar(void** iter, unsigned char* result) const;
  bool ReadString(void** iter, std::string* result) const;
  bool ReadWString(void** iter, std::wstring* result) const;
  bool ReadString16(void** iter, string16* result) const;
  bool ReadData(void** iter, const char** data, int* length) const;
  bool ReadBytes(void** iter, const char** data, int length,
                 uint32_t alignment = sizeof(uint32_t)) const;

  
  
  bool ReadLength(void** iter, int* result) const;

  
  
  
  
  bool WriteBool(bool value) {
    return WriteInt(value ? 1 : 0);
  }
  bool WriteInt16(int16_t value) {
    return WriteBytes(&value, sizeof(value));
  }
  bool WriteUInt16(uint16_t value) {
    return WriteBytes(&value, sizeof(value));
  }
  bool WriteInt(int value) {
    return WriteBytes(&value, sizeof(value));
  }
  bool WriteLong(long value) {
    
    
    return WriteInt64(int64_t(value));
  }
  bool WriteULong(unsigned long value) {
    
    
    return WriteUInt64(uint64_t(value));
  }
  bool WriteSize(size_t value) {
    
    
    return WriteUInt64(uint64_t(value));
  }
  bool WriteInt32(int32_t value) {
    return WriteBytes(&value, sizeof(value));
  }
  bool WriteUInt32(uint32_t value) {
    return WriteBytes(&value, sizeof(value));
  }
  bool WriteInt64(int64_t value) {
    return WriteBytes(&value, sizeof(value));
  }
  bool WriteUInt64(uint64_t value) {
    return WriteBytes(&value, sizeof(value));
  }
  bool WriteDouble(double value) {
    return WriteBytes(&value, sizeof(value));
  }
  bool WriteIntPtr(intptr_t value) {
    
    
    return WriteInt64(int64_t(value));
  }
  bool WriteUnsignedChar(unsigned char value) {
    return WriteBytes(&value, sizeof(value));
  }
  bool WriteString(const std::string& value);
  bool WriteWString(const std::wstring& value);
  bool WriteString16(const string16& value);
  bool WriteData(const char* data, int length);
  bool WriteBytes(const void* data, int data_len,
                  uint32_t alignment = sizeof(uint32_t));

  
  
  
  
  
  
  
  
  char* BeginWriteData(int length);

  
  
  
  
  
  
  
  
  
  void TrimWriteData(int length);

  void EndRead(void* iter) const {
    DCHECK(iter == end_of_payload());
  }

  
  struct Header {
    uint32_t payload_size;  
  };

  
  
  
  template <class T>
  T* headerT() {
    DCHECK(sizeof(T) == header_size_);
    return static_cast<T*>(header_);
  }
  template <class T>
  const T* headerT() const {
    DCHECK(sizeof(T) == header_size_);
    return static_cast<const T*>(header_);
  }

  
  
  
  bool IteratorHasRoomFor(const void* iter, int len) const {
    if ((len < 0) || (iter < header_) || iter > end_of_payload())
      return false;
    const char* end_of_region = reinterpret_cast<const char*>(iter) + len;
    
    return (iter <= end_of_region) && (end_of_region <= end_of_payload());
  }

 protected:
  uint32_t payload_size() const { return header_->payload_size; }

  char* payload() {
    return reinterpret_cast<char*>(header_) + header_size_;
  }
  const char* payload() const {
    return reinterpret_cast<const char*>(header_) + header_size_;
  }

  
  
  char* end_of_payload() {
    return payload() + payload_size();
  }
  const char* end_of_payload() const {
    return payload() + payload_size();
  }

  uint32_t capacity() const {
    return capacity_;
  }

  
  
  
  
  char* BeginWrite(uint32_t length, uint32_t alignment);

  
  
  
  void EndWrite(char* dest, int length);

  
  
  
  
  bool Resize(uint32_t new_capacity);

  
  static uint32_t AlignInt(uint32_t i, int alignment) {
    return i + (alignment - (i % alignment)) % alignment;
  }

  
  
  
  static void UpdateIter(void** iter, int bytes) {
    *iter = static_cast<char*>(*iter) + AlignInt(bytes, sizeof(uint32_t));
  }

  
  
  static const char* FindNext(uint32_t header_size,
                              const char* range_start,
                              const char* range_end);

  
  static const int kPayloadUnit;

 private:
  Header* header_;
  uint32_t header_size_;
  uint32_t capacity_;
  uint32_t variable_buffer_offset_;
  FRIEND_TEST(PickleTest, Resize);
  FRIEND_TEST(PickleTest, FindNext);
  FRIEND_TEST(PickleTest, IteratorHasRoom);
};

#endif  
