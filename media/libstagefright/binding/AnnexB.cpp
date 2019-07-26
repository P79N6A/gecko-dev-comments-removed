



#include "mp4_demuxer/AnnexB.h"
#include "mp4_demuxer/ByteReader.h"
#include "mp4_demuxer/DecoderData.h"

using namespace mozilla;

namespace mp4_demuxer
{

static const uint8_t kAnnexBDelimiter[] = { 0, 0, 0, 1 };

Vector<uint8_t>
AnnexB::ConvertExtraDataToAnnexB(mozilla::Vector<uint8_t>& aExtraData)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  Vector<uint8_t> annexB;

  ByteReader reader(aExtraData);
  const uint8_t* ptr = reader.Read(5);
  if (ptr && ptr[0] == 1) {
    
    ConvertSpsOrPsp(reader, reader.ReadU8() & 31, &annexB);
    ConvertSpsOrPsp(reader, reader.ReadU8(), &annexB);

    MOZ_ASSERT(!reader.Remaining());
  }

  return annexB;
}

void
AnnexB::ConvertSpsOrPsp(ByteReader& aReader, uint8_t aCount,
                        Vector<uint8_t>* aAnnexB)
{
  for (int i = 0; i < aCount; i++) {
    uint16_t length = aReader.ReadU16();

    const uint8_t* ptr = aReader.Read(length);
    if (!ptr) {
      MOZ_ASSERT(false);
      return;
    }
    aAnnexB->append(kAnnexBDelimiter, ArrayLength(kAnnexBDelimiter));
    aAnnexB->append(ptr, length);
  }
}
}
