



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
  
  
  static bool ConvertSampleToAnnexB(MP4Sample* aSample);
  
  
  static bool ConvertSampleToAVCC(MP4Sample* aSample);
  static bool ConvertSampleTo4BytesAVCC(MP4Sample* aSample);

  
  static already_AddRefed<ByteBuffer> ConvertExtraDataToAnnexB(
    const ByteBuffer* aExtraData);
  
  
  
  static already_AddRefed<ByteBuffer> ExtractExtraData(
    const MP4Sample* aSample);
  static bool HasSPS(const MP4Sample* aSample);
  static bool HasSPS(const ByteBuffer* aExtraData);
  
  static bool IsAVCC(const MP4Sample* aSample);
  
  static bool IsAnnexB(const MP4Sample* aSample);
  
  static bool CompareExtraData(const ByteBuffer* aExtraData1,
                               const ByteBuffer* aExtraData2);

private:
  
  static void ConvertSPSOrPPS(ByteReader& aReader, uint8_t aCount,
                              ByteBuffer* aAnnexB);
};

} 

#endif 
