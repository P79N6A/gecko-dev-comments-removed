



#ifndef MP4_DEMUXER_ANNEX_B_H_
#define MP4_DEMUXER_ANNEX_B_H_

template <class T> struct already_AddRefed;

namespace mozilla {
class MediaRawData;
class DataBuffer;
}
namespace mp4_demuxer
{
class ByteReader;

class AnnexB
{
public:
  
  
  static bool ConvertSampleToAnnexB(mozilla::MediaRawData* aSample);
  
  
  static bool ConvertSampleToAVCC(mozilla::MediaRawData* aSample);
  static bool ConvertSampleTo4BytesAVCC(mozilla::MediaRawData* aSample);

  
  static already_AddRefed<mozilla::DataBuffer> ConvertExtraDataToAnnexB(
    const mozilla::DataBuffer* aExtraData);
  
  
  
  static already_AddRefed<mozilla::DataBuffer> ExtractExtraData(
    const mozilla::MediaRawData* aSample);
  static bool HasSPS(const mozilla::MediaRawData* aSample);
  static bool HasSPS(const mozilla::DataBuffer* aExtraData);
  
  static bool IsAVCC(const mozilla::MediaRawData* aSample);
  
  static bool IsAnnexB(const mozilla::MediaRawData* aSample);
  
  static bool CompareExtraData(const mozilla::DataBuffer* aExtraData1,
                               const mozilla::DataBuffer* aExtraData2);

private:
  
  static void ConvertSPSOrPPS(ByteReader& aReader, uint8_t aCount,
                              mozilla::DataBuffer* aAnnexB);
};

} 

#endif 
