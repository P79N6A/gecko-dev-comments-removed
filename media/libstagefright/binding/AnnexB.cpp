



#include "mozilla/ArrayUtils.h"
#include "mozilla/Endian.h"
#include "mp4_demuxer/AnnexB.h"
#include "mp4_demuxer/ByteReader.h"
#include "mp4_demuxer/ByteWriter.h"
#include "mp4_demuxer/DecoderData.h"

using namespace mozilla;

namespace mp4_demuxer
{

static const uint8_t kAnnexBDelimiter[] = { 0, 0, 0, 1 };

bool
AnnexB::ConvertSampleToAnnexB(MP4Sample* aSample)
{
  MOZ_ASSERT(aSample);

  if (!IsAVCC(aSample)) {
    return true;
  }
  MOZ_ASSERT(aSample->data);

  if (!ConvertSampleTo4BytesAVCC(aSample)) {
    return false;
  }

  if (aSample->size < 4) {
    
    return true;
  }

  ByteReader reader(aSample->data, aSample->size);

  mozilla::Vector<uint8_t> tmp;
  ByteWriter writer(tmp);

  while (reader.Remaining() >= 4) {
    uint32_t nalLen = reader.ReadU32();
    const uint8_t* p = reader.Read(nalLen);

    writer.Write(kAnnexBDelimiter, ArrayLength(kAnnexBDelimiter));
    if (!p) {
      break;
    }
    writer.Write(p, nalLen);
  }

  if (!aSample->Replace(tmp.begin(), tmp.length())) {
    return false;
  }

  
  if (aSample->is_sync_point) {
    nsRefPtr<ByteBuffer> annexB =
      ConvertExtraDataToAnnexB(aSample->extra_data);
    if (!aSample->Prepend(annexB->Elements(), annexB->Length())) {
      return false;
    }
  }

  return true;
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

static bool
FindStartCodeInternal(ByteReader& aBr) {
  size_t offset = aBr.Offset();

  for (uint32_t i = 0; i < aBr.Align() && aBr.Remaining() >= 3; i++) {
    if (aBr.PeekU24() == 0x000001) {
      return true;
    }
    aBr.Read(1);
  }

  while (aBr.Remaining() >= 6) {
    uint32_t x32 = aBr.PeekU32();
    if ((x32 - 0x01010101) & (~x32) & 0x80808080) {
      if ((x32 >> 8) == 0x000001) {
        return true;
      }
      if (x32 == 0x000001) {
        aBr.Read(1);
        return true;
      }
      if ((x32 & 0xff) == 0) {
        const uint8_t* p = aBr.Peek(1);
        if ((x32 & 0xff00) == 0 && p[4] == 1) {
          aBr.Read(2);
          return true;
        }
        if (p[4] == 0 && p[5] == 1) {
          aBr.Read(3);
          return true;
        }
      }
    }
    aBr.Read(4);
  }

  while (aBr.Remaining() >= 3) {
    if (aBr.PeekU24() == 0x000001) {
      return true;
    }
    aBr.Read(1);
  }

  
  aBr.Seek(offset);
  return false;
}

static bool
FindStartCode(ByteReader& aBr, size_t& aStartSize)
{
  if (!FindStartCodeInternal(aBr)) {
    aStartSize = 0;
    return false;
  }

  aStartSize = 3;
  if (aBr.Offset()) {
    
    aBr.Rewind(1);
    if (aBr.ReadU8() == 0) {
      aStartSize = 4;
    }
  }
  aBr.Read(3);
  return true;
}

static void
ParseNALUnits(ByteWriter& aBw, ByteReader& aBr)
{
  size_t startSize;

  bool rv = FindStartCode(aBr, startSize);
  if (rv) {
    size_t startOffset = aBr.Offset();
    while (FindStartCode(aBr, startSize)) {
      size_t offset = aBr.Offset();
      size_t sizeNAL = offset - startOffset - startSize;
      aBr.Seek(startOffset);
      aBw.WriteU32(sizeNAL);
      aBw.Write(aBr.Read(sizeNAL), sizeNAL);
      aBr.Read(startSize);
      startOffset = offset;
    }
  }
  size_t sizeNAL = aBr.Remaining();
  if (sizeNAL) {
    aBw.WriteU32(sizeNAL);
    aBw.Write(aBr.Read(sizeNAL), sizeNAL);
  }
}

bool
AnnexB::ConvertSampleToAVCC(MP4Sample* aSample)
{
  if (IsAVCC(aSample)) {
    return ConvertSampleTo4BytesAVCC(aSample);
  }
  if (!IsAnnexB(aSample)) {
    
    return false;
  }

  mozilla::Vector<uint8_t> nalu;
  ByteWriter writer(nalu);
  ByteReader reader(aSample->data, aSample->size);

  ParseNALUnits(writer, reader);
  return aSample->Replace(nalu.begin(), nalu.length());
}

already_AddRefed<ByteBuffer>
AnnexB::ExtractExtraData(const MP4Sample* aSample)
{
  nsRefPtr<ByteBuffer> extradata = new ByteBuffer;
  if (IsAVCC(aSample) && HasSPS(aSample->extra_data)) {
    
    extradata = aSample->extra_data;
    return extradata.forget();
  }

  if (IsAnnexB(aSample)) {
    return extradata.forget();
  }
  
  mozilla::Vector<uint8_t> sps;
  ByteWriter spsw(sps);
  int numSps = 0;
  
  mozilla::Vector<uint8_t> pps;
  ByteWriter ppsw(pps);
  int numPps = 0;

  int nalLenSize;
  if (IsAVCC(aSample)) {
    nalLenSize = ((*aSample->extra_data)[4] & 3) + 1;
  } else {
    
    
    nalLenSize = 4;
  }
  ByteReader reader(aSample->data, aSample->size);

  
  uint8_t* d = aSample->data;
  while (reader.Remaining() > nalLenSize) {
    uint32_t nalLen;
    switch (nalLenSize) {
      case 1: nalLen = reader.ReadU8();  break;
      case 2: nalLen = reader.ReadU16(); break;
      case 3: nalLen = reader.ReadU24(); break;
      case 4: nalLen = reader.ReadU32(); break;
    }
    uint8_t nalType = reader.PeekU8();
    const uint8_t* p = reader.Read(nalLen);
    if (!p) {
      return extradata.forget();
    }

    if (nalType == 0x67) { 
      numSps++;
      spsw.WriteU16(nalLen);
      spsw.Write(p, nalLen);
    } else if (nalType == 0x68) { 
      numPps++;
      ppsw.WriteU16(nalLen);
      ppsw.Write(p, nalLen);
    }
  }

  if (numSps && sps.length() > 5) {
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

bool
AnnexB::ConvertSampleTo4BytesAVCC(MP4Sample* aSample)
{
  MOZ_ASSERT(IsAVCC(aSample));

  int nalLenSize = ((*aSample->extra_data)[4] & 3) + 1;

  if (nalLenSize == 4) {
    return true;
  }
  mozilla::Vector<uint8_t> dest;
  ByteWriter writer(dest);
  ByteReader reader(aSample->data, aSample->size);
  while (reader.Remaining() > nalLenSize) {
    uint32_t nalLen;
    switch (nalLenSize) {
      case 1: nalLen = reader.ReadU8();  break;
      case 2: nalLen = reader.ReadU16(); break;
      case 3: nalLen = reader.ReadU24(); break;
      case 4: nalLen = reader.ReadU32(); break;
    }
    const uint8_t* p = reader.Read(nalLen);
    if (!p) {
      return true;
    }
    writer.WriteU32(nalLen);
    writer.Write(p, nalLen);
  }
  return aSample->Replace(dest.begin(), dest.length());
}

bool
AnnexB::IsAVCC(const MP4Sample* aSample)
{
  return aSample->size >= 3 && aSample->extra_data &&
    aSample->extra_data->Length() >= 7 && (*aSample->extra_data)[0] == 1;
}

bool
AnnexB::IsAnnexB(const MP4Sample* aSample)
{
  if (aSample->size < 4) {
    return false;
  }
  uint32_t header = mozilla::BigEndian::readUint32(aSample->data);
  return header == 0x00000001 || (header >> 8) == 0x000001;
}

bool
AnnexB::CompareExtraData(const ByteBuffer* aExtraData1,
                         const ByteBuffer* aExtraData2)
{
  
  return aExtraData1 == aExtraData2 || *aExtraData1 == *aExtraData2;
}

} 
