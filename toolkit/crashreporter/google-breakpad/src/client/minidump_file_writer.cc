
































#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "client/minidump_file_writer-inl.h"

namespace google_airbag {

MinidumpFileWriter::MinidumpFileWriter() : file_(-1), position_(0), size_(0) {
}

MinidumpFileWriter::~MinidumpFileWriter() {
  Close();
}

bool MinidumpFileWriter::Open(const std::string &path) {
  assert(file_ == -1);
  file_ = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);

  return file_ != -1;
}

bool MinidumpFileWriter::Close() {
  bool result = true;

  if (file_ != -1) {
    ftruncate(file_, position_);
    result = close(file_) == 0;
    file_ = -1;
  }

  return result;
}

bool MinidumpFileWriter::WriteString(const wchar_t *str,
                                     unsigned int length,
                                     MDLocationDescriptor *location) {
  assert(str);
  assert(location);
  
  
  if (!length)
    length = INT_MAX;

  unsigned int mdstring_length = 0;
  for (; mdstring_length < length && str[mdstring_length]; ++mdstring_length) {
  }

  
  TypedMDRVA<MDString> mdstring(this);

  if (!mdstring.AllocateObjectAndArray(mdstring_length + 1, sizeof(u_int16_t)))
    return false;

  
  mdstring.get()->length = mdstring_length * sizeof(u_int16_t);

  u_int16_t ch;
  bool result = true;

  if (sizeof(wchar_t) == sizeof(u_int16_t)) {
    
    result = mdstring.Copy(str, mdstring.get()->length);
  } else {
    
    for (unsigned int c = 0; c < mdstring_length && result == true; c++) {
      ch = str[c];
      
      
      
      
      
      result = mdstring.CopyIndexAfterObject(c, &ch, sizeof(ch));
    }
  }

  
  if (result) {
    ch = 0;
    result = mdstring.CopyIndexAfterObject(mdstring_length, &ch, sizeof(ch));

    if (result)
      *location = mdstring.location();
  }

  return result;
}

bool MinidumpFileWriter::WriteString(const char *str, unsigned int length,
                                     MDLocationDescriptor *location) {
  assert(str);
  assert(location);
  
  
  if (!length)
    length = INT_MAX;

  unsigned int mdstring_length = 0;
  for (; mdstring_length < length && str[mdstring_length]; ++mdstring_length) {
  }

  
  TypedMDRVA<MDString> mdstring(this);

  if (!mdstring.AllocateObjectAndArray(mdstring_length + 1, sizeof(u_int16_t)))
    return false;

  
  mdstring.get()->length = mdstring_length * sizeof(u_int16_t);

  u_int16_t ch;
  bool result = true;

  
  for (unsigned int c = 0; c < mdstring_length && result == true; c++) {
    ch = str[c];
    
    
    
    
    
    result = mdstring.CopyIndexAfterObject(c, &ch, sizeof(ch));
  }

  
  if (result) {
    ch = 0;
    result = mdstring.CopyIndexAfterObject(mdstring_length, &ch, sizeof(ch));

    if (result)
      *location = mdstring.location();
  }

  return result;
}

bool MinidumpFileWriter::WriteMemory(const void *src, size_t size,
                                     MDMemoryDescriptor *output) {
  assert(src);
  assert(output);
  UntypedMDRVA mem(this);

  if (!mem.Allocate(size))
    return false;

  if (!mem.Copy(src, mem.size()))
    return false;

  output->start_of_memory_range = reinterpret_cast<u_int64_t>(src);
  output->memory = mem.location();

  return true;
}

MDRVA MinidumpFileWriter::Allocate(size_t size) {
  assert(size);
  assert(file_ != -1);

  size_t aligned_size = (size + 7) & ~7;  

  if (position_ + aligned_size > size_) {
    size_t growth = aligned_size;
    size_t minimal_growth = getpagesize();

    
    if (growth < minimal_growth)
      growth = minimal_growth;

    size_t new_size = size_ + growth;

    if (ftruncate(file_, new_size) != 0)
      return kInvalidMDRVA;

    size_ = new_size;
  }

  MDRVA current_position = position_;
  position_ += static_cast<MDRVA>(aligned_size);

  return current_position;
}

bool MinidumpFileWriter::Copy(MDRVA position, const void* src, ssize_t size) {
  assert(src);
  assert(size);
  assert(file_ != -1);

  
  if (size + position > size_)
    return false;

  
  if (lseek(file_, position, SEEK_SET) == static_cast<off_t>(position))
    if (write(file_, src, size) == size)
      return true;

  return false;
}

bool UntypedMDRVA::Allocate(size_t size) {
  assert(size_ == 0);
  size_ = size;
  position_ = writer_->Allocate(size_);
  return position_ != MinidumpFileWriter::kInvalidMDRVA;
}

bool UntypedMDRVA::Copy(MDRVA position, const void *src, size_t size) {
  assert(src);
  assert(size);
  assert(position + size <= position_ + size_);
  return writer_->Copy(position, src, size);
}

}  
