



#ifndef MP4_DEMUXER_ANNEX_B_H_
#define MP4_DEMUXER_ANNEX_B_H_

#include "mozilla/Vector.h"

namespace mp4_demuxer
{
class ByteReader;
class MP4Sample;

class AnnexB
{
public:
  
  
  static void ConvertSample(MP4Sample* aSample,
                            const mozilla::Vector<uint8_t>& annexB);

  
  static mozilla::Vector<uint8_t> ConvertExtraDataToAnnexB(
    mozilla::Vector<uint8_t>& aExtraData);

private:
  
  static void ConvertSPSOrPPS(ByteReader& aReader, uint8_t aCount,
                              mozilla::Vector<uint8_t>* aAnnexB);
};

} 

#endif 
