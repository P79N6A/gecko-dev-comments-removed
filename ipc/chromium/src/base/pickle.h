



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
  bool ReadInt16(void** iter, int16* result) const;
  bool ReadUInt16(void** iter, uint16* result) const;
  bool ReadShort(void** iter, short* result) const;
  bool ReadInt(void** iter, int* result) const;
  bool ReadLong(void** iter, long* result) const;
  bool ReadULong(void** iter, unsigned long* result) const;
  bool ReadSize(void** iter, size_t* result) const;
  bool ReadInt32(void** iter, int32* result) const;
  bool ReadUInt32(void** iter, uint32* result) const;
  bool ReadInt64(void** iter, int64* result) const;
  bool ReadUInt64(void** iter, uint64* result) const;
  bool ReadDouble(void** iter, double* result) const;
  bool ReadIntPtr(void** iter, intptr_t* result) const;
  bool ReadUnsignedChar(void** iter, unsigned char* result) const;
  bool ReadString(void** iter, std::string* result) const;
  bool ReadWString(void** iter, std::wstring* result) const;
  bool ReadString16(void** iter, string16* result) const;
  bool ReadData(void** iter, const char** data, int* length) const;
  bool ReadBytes(void** iter, const char** data, int length) const;

  
  
  bool ReadLength(void** iter, int* result) const;

  
  
  
  
  bool WriteBool(bool value) {
    return WriteInt(value ? 1 : 0);
  }
  bool WriteInt16(int16 value) {
    return WriteBytes(&value, sizeof(value));
  }
  bool WriteUInt16(uint16 value) {
    return WriteBytes(&value, sizeof(value));
  }
  bool WriteInt(int value) {
    return WriteBytes(&value, sizeof(value));
  }
  bool WriteLong(long value) {
    
    
    return WriteInt64(int64(value));
  }
  bool WriteULong(unsigned long value) {
    
    
    return WriteUInt64(uint64(value));
  }
  bool WriteSize(size_t value) {
    
    
    return WriteUInt64(uint64(value));
  }
  bool WriteInt32(int32 value) {
    return WriteBytes(&value, sizeof(value));
  }
  bool WriteUInt32(uint32 value) {
    return WriteBytes(&value, sizeof(value));
  }
  bool WriteInt64(int64 value) {
    return WriteBytes(&value, sizeof(value));
  }
  bool WriteUInt64(uint64 value) {
    return WriteBytes(&value, sizeof(value));
  }
  bool WriteDouble(double value) {
    return WriteBytes(&value, sizeof(value));
  }
  bool WriteIntPtr(intptr_t value) {
    
    
    return WriteInt64(int64(value));
  }
  bool WriteUnsignedChar(unsigned char value) {
    return WriteBytes(&value, sizeof(value));
  }
  bool WriteString(const std::string& value);
  bool WriteWString(const std::wstring& value);
  bool WriteString16(const string16& value);
  bool WriteData(const char* data, int length);
  bool WriteBytes(const void* data, int data_len);

  
  
  
  
  
  
  
  
  char* BeginWriteData(int length);

  
  
  
  
  
  
  
  
  
  void TrimWriteData(int length);

#if defined(CHROMIUM_MOZILLA_BUILD)
  void EndRead(void* iter) const {
    DCHECK(iter == end_of_payload());
  }
#endif

  
  struct Header {
    uint32 payload_size;  
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
#ifdef CHROMIUM_MOZILLA_BUILD
  uint32 payload_size() const { return header_->payload_size; }
#else
  size_t payload_size() const { return header_->payload_size; }
#endif

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

#ifdef CHROMIUM_MOZILLA_BUILD
  uint32 capacity() const {
#else
  size_t capacity() const {
#endif
    return capacity_;
  }

  
  
  
  
#ifdef CHROMIUM_MOZILLA_BUILD
  char* BeginWrite(uint32 length);
#else
  char* BeginWrite(size_t length);
#endif

  
  
  
  void EndWrite(char* dest, int length);

  
  
  
  
#ifdef CHROMIUM_MOZILLA_BUILD
  bool Resize(uint32 new_capacity);
#else
  bool Resize(size_t new_capacity);
#endif

  
#ifdef CHROMIUM_MOZILLA_BUILD
  static uint32 AlignInt(uint32 i, int alignment) {
#else
  static size_t AlignInt(size_t i, int alignment) {
#endif
    return i + (alignment - (i % alignment)) % alignment;
  }

  
  
  
  static void UpdateIter(void** iter, int bytes) {
    *iter = static_cast<char*>(*iter) + AlignInt(bytes, sizeof(uint32));
  }

  
  
#ifdef CHROMIUM_MOZILLA_BUILD
  static const char* FindNext(uint32 header_size,
#else
  static const char* FindNext(size_t header_size,
#endif
                              const char* range_start,
                              const char* range_end);

  
  static const int kPayloadUnit;

 private:
  Header* header_;
#ifdef CHROMIUM_MOZILLA_BUILD
  uint32 header_size_;
  uint32 capacity_;
  uint32 variable_buffer_offset_;
#else
  size_t header_size_;  
  
  size_t capacity_;
  size_t variable_buffer_offset_;  
#endif

  FRIEND_TEST(PickleTest, Resize);
  FRIEND_TEST(PickleTest, FindNext);
  FRIEND_TEST(PickleTest, IteratorHasRoom);
};

#endif  
