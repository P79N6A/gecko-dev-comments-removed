






























#ifndef CLIENT_MINIDUMP_FILE_WRITER_H__
#define CLIENT_MINIDUMP_FILE_WRITER_H__

#include <string>

#include "google_airbag/common/minidump_format.h"

namespace google_airbag {

class MinidumpFileWriter {
 public:
  
  
  static const MDRVA kInvalidMDRVA = static_cast<MDRVA>(-1);

  MinidumpFileWriter();
  ~MinidumpFileWriter();

  
  
  
  bool Open(const std::string &path);

  
  
  bool Close();

  
  
  
  
  
  
  
  bool WriteString(const wchar_t *str, unsigned int length,
                   MDLocationDescriptor *location);

  
  bool WriteString(const char *str, unsigned int length,
                   MDLocationDescriptor *location);

  
  
  bool WriteMemory(const void *src, size_t size, MDMemoryDescriptor *output);

  
  
  bool Copy(MDRVA position, const void *src, ssize_t size);

  
  MDRVA position() const { return position_; }

 private:
  friend class UntypedMDRVA;

  
  
  
  MDRVA Allocate(size_t size);

  
  int file_;

  
  MDRVA position_;

  
  size_t size_;
};


class UntypedMDRVA {
 public:
  explicit UntypedMDRVA(MinidumpFileWriter *writer)
      : writer_(writer),
        position_(writer->position()),
        size_(0) {}

  
  
  bool Allocate(size_t size);

  
  MDRVA position() const { return position_; }

  
  size_t size() const { return size_; }

  
  MDLocationDescriptor location() const {
    MDLocationDescriptor location = { size_, position_ };
    return location;
  }

  
  
  bool Copy(MDRVA position, const void *src, size_t size);

  
  bool Copy(const void *src, size_t size) {
    return Copy(position_, src, size);
  }

 protected:
  
  MinidumpFileWriter *writer_;

  
  MDRVA position_;

  
  size_t size_;
};






template<typename MDType>
class TypedMDRVA : public UntypedMDRVA {
 public:
  
  explicit TypedMDRVA(MinidumpFileWriter *writer)
      : UntypedMDRVA(writer),
        data_(),
        allocation_state_(UNALLOCATED) {}

  ~TypedMDRVA() {
    
    if (allocation_state_ != ARRAY)
      Flush();
  }

  
  
  
  MDType *get() { return &data_; }

  
  
  
  bool Allocate();

  
  
  
  bool Allocate(size_t additional);

  
  
  
  bool AllocateArray(size_t count);

  
  
  
  bool AllocateObjectAndArray(unsigned int count, size_t size);

  
  
  
  bool CopyIndex(unsigned int index, MDType *item);

  
  
  
  bool CopyIndexAfterObject(unsigned int index, void *src, size_t size);

  
  bool Flush();

 private:
  enum AllocationState {
    UNALLOCATED = 0,
    SINGLE_OBJECT,
    ARRAY,
    SINGLE_OBJECT_WITH_ARRAY
  };

  MDType data_;
  AllocationState allocation_state_;
};

}  

#endif
