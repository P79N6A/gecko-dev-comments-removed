




#include "OMXCodecDescriptorUtil.h"

namespace android {



















































static const uint8_t kNALUnitStartCode[] = { 0x00, 0x00, 0x00, 0x01 };


enum {
  kNALUnitTypeSPS = 0x07,   
  kNALUnitTypePPS = 0x08,   
  kNALUnitTypeBad = -1,     
};


struct AVCParamSet {
  AVCParamSet(const uint8_t* aPtr, const size_t aSize)
    : mPtr(aPtr)
    , mSize(aSize)
  {
    MOZ_ASSERT(mPtr && mSize > 0);
  }

  size_t Size() {
    return mSize + 2; 
  }

  
  void AppendTo(nsTArray<uint8_t>* aOutputBuf)
  {
    
    uint8_t size[] = {
      (mSize & 0xFF00) >> 8, 
      mSize & 0x00FF,        
    };
    aOutputBuf->AppendElements(size, sizeof(size));

    aOutputBuf->AppendElements(mPtr, mSize);
  }

  const uint8_t* mPtr; 
  const size_t mSize;  
};



static status_t
ConvertParamSetsToDescriptorBlob(sp<ABuffer>& aSPS, sp<ABuffer>& aPPS,
                                 nsTArray<uint8_t>* aOutputBuf)
{
  
  AVCParamSet sps(aSPS->data() + sizeof(kNALUnitStartCode),
                  aSPS->size() - sizeof(kNALUnitStartCode));
  AVCParamSet pps(aPPS->data() + sizeof(kNALUnitStartCode),
                  aPPS->size() - sizeof(kNALUnitStartCode));
  size_t paramSetsSize = sps.Size() + pps.Size();

  
  uint8_t* info = aSPS->data() + 5;

  uint8_t header[] = {
    0x01,     
    info[0],  
    info[1],  
    info[2],  
    0xFF,     
  };

  
  aOutputBuf->SetCapacity(sizeof(header) + paramSetsSize + 2);
  
  aOutputBuf->AppendElements(header, sizeof(header)); 
  aOutputBuf->AppendElement(0xE0 | 1); 
  sps.AppendTo(aOutputBuf); 
  aOutputBuf->AppendElement(1); 
  pps.AppendTo(aOutputBuf); 

  return OK;
}

static int
NALType(sp<ABuffer>& aBuffer)
{
  if (aBuffer == nullptr) {
    return kNALUnitTypeBad;
  }
  
  uint8_t* data = aBuffer->data();
  if (aBuffer->size() <= 4 ||
      memcmp(data, kNALUnitStartCode, sizeof(kNALUnitStartCode))) {
    return kNALUnitTypeBad;
  }

  return data[4] & 0x1F;
}




status_t
GenerateAVCDescriptorBlob(sp<AMessage>& aConfigData,
                          nsTArray<uint8_t>* aOutputBuf,
                          OMXVideoEncoder::BlobFormat aFormat)
{
  
  char key[6] = "csd-";
  sp<ABuffer> sps;
  sp<ABuffer> pps;
  for (int i = 0; i < 2; i++) {
    snprintf(key + 4, 2, "%d", i);
    sp<ABuffer> paramSet;
    bool found = aConfigData->findBuffer(key, &paramSet);
    int type = NALType(paramSet);
    bool valid = ((type == kNALUnitTypeSPS) || (type == kNALUnitTypePPS));

    MOZ_ASSERT(found && valid);
    if (!found || !valid) {
      return ERROR_MALFORMED;
    }

    switch (type) {
      case kNALUnitTypeSPS:
        sps = paramSet;
        break;
      case kNALUnitTypePPS:
        pps = paramSet;
        break;
      default:
        NS_NOTREACHED("Should not get here!");
    }
  }

  MOZ_ASSERT(sps != nullptr && pps != nullptr);
  if (sps == nullptr || pps == nullptr) {
    return ERROR_MALFORMED;
  }

  status_t result = OK;
  if (aFormat == OMXVideoEncoder::BlobFormat::AVC_NAL) {
    
    aOutputBuf->AppendElements(sps->data(), sps->size());
    aOutputBuf->AppendElements(pps->data(), pps->size());
    return OK;
  } else {
    status_t result = ConvertParamSetsToDescriptorBlob(sps, pps, aOutputBuf);
    MOZ_ASSERT(result == OK);
    return result;
  }
}

} 
