



#ifndef MEDIA_MP4_CENC_H_
#define MEDIA_MP4_CENC_H_

#include <vector>

#include "mp4_demuxer/basictypes.h"
#include "mp4_demuxer/decrypt_config.h"

namespace mp4_demuxer {

class StreamReader;

struct FrameCENCInfo {
  uint8_t iv[16];
  std::vector<SubsampleEntry> subsamples;

  FrameCENCInfo();
  ~FrameCENCInfo();
  bool Parse(int iv_size, StreamReader* r);
  size_t GetTotalSizeOfSubsamples() const;
};

}  

#endif  
