



#ifndef MEDIA_BASE_BIT_READER_H_
#define MEDIA_BASE_BIT_READER_H_

#include <sys/types.h>

#include "mp4_demuxer/basictypes.h"

namespace mp4_demuxer {


class BitReader {
 public:
  
  
  BitReader(const uint8_t* data, off_t size);
  ~BitReader();

  
  
  
  
  
  
  
  
  template<typename T> bool ReadBits(int num_bits, T *out) {
    DCHECK_LE(num_bits, static_cast<int>(sizeof(T) * 8));
    uint64_t temp;
    bool ret = ReadBitsInternal(num_bits, &temp);
    *out = static_cast<T>(temp);
    return ret;
  }

  
  int bits_available() const;

 private:
  
  bool ReadBitsInternal(int num_bits, uint64_t* out);

  
  
  
  void UpdateCurrByte();

  
  const uint8_t* data_;

  
  off_t bytes_left_;

  
  
  uint8_t curr_byte_;

  
  int num_remaining_bits_in_curr_byte_;

 private:
  DISALLOW_COPY_AND_ASSIGN(BitReader);
};

}  

#endif  
