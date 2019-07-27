



#ifndef MP4_DEMUXER_ANNEX_B_H_
#define MP4_DEMUXER_ANNEX_B_H_

template <class T> struct already_AddRefed;

namespace mozilla {
class MediaRawData;
class MediaByteBuffer;
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

  
  static already_AddRefed<mozilla::MediaByteBuffer> ConvertExtraDataToAnnexB(
    const mozilla::MediaByteBuffer* aExtraData);
  
  
  
  static already_AddRefed<mozilla::MediaByteBuffer> ExtractExtraData(
    const mozilla::MediaRawData* aSample);
  static bool HasSPS(const mozilla::MediaRawData* aSample);
  static bool HasSPS(const mozilla::MediaByteBuffer* aExtraData);
  
  static bool IsAVCC(const mozilla::MediaRawData* aSample);
  
  static bool IsAnnexB(const mozilla::MediaRawData* aSample);
  
  static bool CompareExtraData(const mozilla::MediaByteBuffer* aExtraData1,
                               const mozilla::MediaByteBuffer* aExtraData2);

private:
  
  static void ConvertSPSOrPPS(ByteReader& aReader, uint8_t aCount,
                              mozilla::MediaByteBuffer* aAnnexB);
};

} 

#endif 
