#pragma once

#include <stdint.h>

namespace mp4_demuxer {

class Stream {
public:

  
  
  
  virtual bool ReadAt(int64_t offset,
                      uint8_t* buffer,
                      uint32_t count,
                      uint32_t* out_bytes_read) = 0;

  virtual int64_t Length() const = 0;
};

} 