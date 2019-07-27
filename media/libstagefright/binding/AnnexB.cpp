



#include "mozilla/ArrayUtils.h"
#include "mozilla/Endian.h"
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
    uint32_t nalLen = mozilla::BigEndian::readUint32(d);
    
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

already_AddRefed<ByteBuffer>
AnnexB::ExtractExtraData(const MP4Sample* aSample)
{
  nsRefPtr<ByteBuffer> extradata = new ByteBuffer;
  mozilla::Vector<uint8_t> sps;
  int numSps = 0;
  mozilla::Vector<uint8_t> pps;
  int numPps = 0;

  
  uint8_t* d = aSample->data;
  while (d + 4 < aSample->data + aSample->size) {
    uint32_t nalLen = mozilla::BigEndian::readUint32(d);
    uint8_t nalType = d[4] & 0x1f;
    if (nalType == 7) { 
      numSps++;
      uint8_t val[2];
      mozilla::BigEndian::writeInt16(&val[0], nalLen);
      sps.append(&val[0], 2); 
      sps.append(d + 4, nalLen);
    } else if (nalType == 8) { 
      numPps++;
      uint8_t val[2];
      mozilla::BigEndian::writeInt16(&val[0], nalLen);
      pps.append(&val[0], 2); 
      pps.append(d + 4, nalLen);
    }
    d += 4 + nalLen;
  }

  if (numSps) {
    extradata->AppendElement(1);        
    extradata->AppendElement(sps[3]);   
    extradata->AppendElement(sps[4]);   
    extradata->AppendElement(sps[5]);   
    extradata->AppendElement(0xfc | 3); 
    extradata->AppendElement(0xe0 | numSps);
    extradata->AppendElements(sps.begin(), sps.length());
    extradata->AppendElement(numPps);
    if (numPps) {
      extradata->AppendElements(pps.begin(), pps.length());
    }
  }

  return extradata.forget();
}

bool
AnnexB::HasSPS(const MP4Sample* aSample)
{
  return HasSPS(aSample->extra_data);
}

bool
AnnexB::HasSPS(const ByteBuffer* aExtraData)
{
  if (!aExtraData) {
    return false;
  }

  ByteReader reader(*aExtraData);
  const uint8_t* ptr = reader.Read(5);
  if (!ptr || !reader.CanRead8()) {
    return false;
  }
  uint8_t numSps = reader.ReadU8() & 0x1f;
  reader.DiscardRemaining();

  return numSps > 0;
}

} 
