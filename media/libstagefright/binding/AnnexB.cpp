



#include "mozilla/ArrayUtils.h"
#include "mp4_demuxer/AnnexB.h"
#include "mp4_demuxer/ByteReader.h"
#include "mp4_demuxer/DecoderData.h"

using namespace mozilla;

namespace mp4_demuxer
{

static const uint8_t kAnnexBDelimiter[] = { 0, 0, 0, 1 };

void
AnnexB::ConvertSample(MP4Sample* aSample)
{
  MOZ_ASSERT(aSample);
  if (!aSample->size) {
    return;
  }
  MOZ_ASSERT(aSample->data);
  MOZ_ASSERT(aSample->size >= ArrayLength(kAnnexBDelimiter));
  MOZ_ASSERT(aSample->extra_data);

  uint8_t* d = aSample->data;
  while (d + 4 < aSample->data + aSample->size) {
    uint32_t nalLen = (uint32_t(d[0]) << 24) +
                      (uint32_t(d[1]) << 16) +
                      (uint32_t(d[2]) << 8) +
                       uint32_t(d[3]);
    
    memcpy(d, kAnnexBDelimiter, ArrayLength(kAnnexBDelimiter));
    d += 4 + nalLen;
  }

  
  if (aSample->is_sync_point) {
    nsRefPtr<ByteBuffer> annexB = ConvertExtraDataToAnnexB(aSample->extra_data);
    aSample->Prepend(annexB->Elements(), annexB->Length());
  }
}

already_AddRefed<ByteBuffer>
AnnexB::ConvertExtraDataToAnnexB(const ByteBuffer* aExtraData)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  nsRefPtr<ByteBuffer> annexB = new ByteBuffer;

  ByteReader reader(*aExtraData);
  const uint8_t* ptr = reader.Read(5);
  if (ptr && ptr[0] == 1) {
    
    ConvertSPSOrPPS(reader, reader.ReadU8() & 31, annexB);
    ConvertSPSOrPPS(reader, reader.ReadU8(), annexB);

    
  }
  reader.DiscardRemaining();

  return annexB.forget();
}

void
AnnexB::ConvertSPSOrPPS(ByteReader& aReader, uint8_t aCount,
                        ByteBuffer* aAnnexB)
{
  for (int i = 0; i < aCount; i++) {
    uint16_t length = aReader.ReadU16();

    const uint8_t* ptr = aReader.Read(length);
    if (!ptr) {
      MOZ_ASSERT(false);
      return;
    }
    aAnnexB->AppendElements(kAnnexBDelimiter, ArrayLength(kAnnexBDelimiter));
    aAnnexB->AppendElements(ptr, length);
  }
}

} 
