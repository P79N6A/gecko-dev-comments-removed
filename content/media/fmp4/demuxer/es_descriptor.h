



#ifndef MEDIA_MP4_ES_DESCRIPTOR_H_
#define MEDIA_MP4_ES_DESCRIPTOR_H_

#include <vector>

#include "mp4_demuxer/basictypes.h"

namespace mp4_demuxer {

class BitReader;



enum ObjectType {
  kForbidden = 0,
  kISO_14496_3 = 0x40,  
  kISO_13818_7_AAC_LC = 0x67  
};




class ESDescriptor {
 public:
  ESDescriptor();
  ~ESDescriptor();

  bool Parse(const std::vector<uint8_t>& data);

  uint8_t object_type() const;
  const std::vector<uint8_t>& decoder_specific_info() const;

 private:
  enum Tag {
    kESDescrTag = 0x03,
    kDecoderConfigDescrTag = 0x04,
    kDecoderSpecificInfoTag = 0x05
  };

  bool ParseDecoderConfigDescriptor(BitReader* reader);
  bool ParseDecoderSpecificInfo(BitReader* reader);

  uint8_t object_type_;
  std::vector<uint8_t> decoder_specific_info_;
};

}  

#endif
