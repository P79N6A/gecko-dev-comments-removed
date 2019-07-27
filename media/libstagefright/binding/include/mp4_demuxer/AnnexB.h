



#ifndef MP4_DEMUXER_ANNEX_B_H_
#define MP4_DEMUXER_ANNEX_B_H_

#include "mp4_demuxer/DecoderData.h"

template <class T> struct already_AddRefed;

namespace mp4_demuxer
{
class ByteReader;
class MP4Sample;

class AnnexB
{
public:
  
  
  static void ConvertSample(MP4Sample* aSample);

  
  static already_AddRefed<nsRcTArray<uint8_t>> ConvertExtraDataToAnnexB(
    mozilla::Vector<uint8_t>& aExtraData);

private:
  
  static void ConvertSPSOrPPS(ByteReader& aReader, uint8_t aCount,
                              nsTArray<uint8_t>* aAnnexB);
};

} 

#endif 
