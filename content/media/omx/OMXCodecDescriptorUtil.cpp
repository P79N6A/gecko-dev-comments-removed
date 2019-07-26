




#include "OMXCodecDescriptorUtil.h"

namespace android {


static const uint8_t kNALUnitStartCode[] = { 0x00, 0x00, 0x00, 0x01 };






































class AVCDecodeConfigDescMaker {
public:
  
  
  
  status_t ConvertParamSetsToDescriptorBlob(ABuffer* aParamSets,
                                            nsTArray<uint8_t>* aOutputBuf)
  {
    uint8_t header[] = {
      0x01, 
      0x00, 
      0x00, 
      0x00, 
      0xFF, 
    };

    size_t paramSetsSize = ParseParamSets(aParamSets, header);
    NS_ENSURE_TRUE(paramSetsSize > 0, ERROR_MALFORMED);

    
    aOutputBuf->SetCapacity(sizeof(header) + paramSetsSize + 2);
    
    aOutputBuf->AppendElements(header, sizeof(header));
    
    uint8_t n = mSPS.Length();
    aOutputBuf->AppendElement(0xE0 | n);
    
    for (int i = 0; i < n; i++) {
      mSPS.ElementAt(i).AppendTo(aOutputBuf);
    }
    
    n = mPPS.Length();
    aOutputBuf->AppendElement(n);
    
    for (int i = 0; i < n; i++) {
      mPPS.ElementAt(i).AppendTo(aOutputBuf);
    }

    return OK;
  }

private:
  
  struct AVCParamSet {
    AVCParamSet(const uint8_t* aPtr, const size_t aSize)
      : mPtr(aPtr)
      , mSize(aSize)
    {}

    
    void AppendTo(nsTArray<uint8_t>* aOutputBuf)
    {
      MOZ_ASSERT(mPtr && mSize > 0);

      
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

  
  enum {
    kNALUnitTypeSPS = 0x07, 
    kNALUnitTypePPS = 0x08, 
  };

  
  
  
  
  
  
  
  
  
  uint8_t* ParseParamSet(uint8_t* aPtr, size_t aSize, uint8_t aType,
                         size_t* aParamSetSize)
  {
    MOZ_ASSERT(aPtr && aSize > 0);
    MOZ_ASSERT(aType == kNALUnitTypeSPS || aType == kNALUnitTypePPS);
    MOZ_ASSERT(aParamSetSize);

    
    size_t index = 0;
    size_t end = aSize - sizeof(kNALUnitStartCode);
    uint8_t* nextStartCode = aPtr;
    while (index <= end &&
            memcmp(kNALUnitStartCode, aPtr + index, sizeof(kNALUnitStartCode))) {
      ++index;
    }
    if (index <= end) {
      
      nextStartCode = aPtr + index;
    } else {
      nextStartCode = aPtr + aSize;
    }

    *aParamSetSize = nextStartCode - aPtr;
    NS_ENSURE_TRUE(*aParamSetSize > 0, nullptr);

    AVCParamSet paramSet(aPtr, *aParamSetSize);
    if (aType == kNALUnitTypeSPS) {
      
      NS_ENSURE_TRUE(*aParamSetSize >= 4, nullptr);
      mSPS.AppendElement(paramSet);
    } else {
      mPPS.AppendElement(paramSet);
    }
    return nextStartCode;
  }

  
  
  
  size_t ParseParamSets(ABuffer* aParamSets, uint8_t* aHeader)
  {
    
    
    
    uint8_t type = kNALUnitTypeSPS;
    bool hasSPS = false;
    bool hasPPS = false;
    uint8_t* ptr = aParamSets->data();
    uint8_t* nextStartCode = ptr;
    size_t remain = aParamSets->size();
    size_t paramSetSize = 0;
    size_t totalSize = 0;
    
    while (remain > sizeof(kNALUnitStartCode) &&
            !memcmp(kNALUnitStartCode, ptr, sizeof(kNALUnitStartCode))) {
      ptr += sizeof(kNALUnitStartCode);
      remain -= sizeof(kNALUnitStartCode);
      
      
      
      
      
      
      type = (ptr[0] & 0x1F);
      if (type == kNALUnitTypeSPS) {
        
        NS_ENSURE_FALSE(hasPPS, 0);
        if (!hasSPS) {
          
          aHeader[1] = ptr[1]; 
          aHeader[2] = ptr[2]; 
          aHeader[3] = ptr[3]; 

          hasSPS = true;
        }
        nextStartCode = ParseParamSet(ptr, remain, type, &paramSetSize);
      } else if (type == kNALUnitTypePPS) {
        
        NS_ENSURE_TRUE(hasSPS, 0);
        if (!hasPPS) {
          hasPPS = true;
        }
        nextStartCode = ParseParamSet(ptr, remain, type, &paramSetSize);
      } else {
        
        NS_ENSURE_TRUE(false, 0);
      }
      NS_ENSURE_TRUE(nextStartCode, 0);

      
      remain -= (nextStartCode - ptr);
      ptr = nextStartCode;
      totalSize += (2 + paramSetSize); 
    }

    
    size_t n = mSPS.Length();
    NS_ENSURE_TRUE(n > 0 && n <= 0x1F, 0); 
    n = mPPS.Length();
    NS_ENSURE_TRUE(n > 0 && n <= 0xFF, 0); 

    return totalSize;
  }

  nsTArray<AVCParamSet> mSPS;
  nsTArray<AVCParamSet> mPPS;
};






status_t
GenerateAVCDescriptorBlob(ABuffer* aData, nsTArray<uint8_t>* aOutputBuf)
{
  const size_t csdSize = aData->size();
  const uint8_t* csd = aData->data();

  MOZ_ASSERT(csdSize > sizeof(kNALUnitStartCode),
             "Size of codec specific data is too short. "
             "There could be a serious problem in MediaCodec.");

  NS_ENSURE_TRUE(csdSize > sizeof(kNALUnitStartCode), ERROR_MALFORMED);

  if (memcmp(csd, kNALUnitStartCode, sizeof(kNALUnitStartCode))) {
    
    NS_ENSURE_TRUE(csdSize >= 13, ERROR_MALFORMED);

    aOutputBuf->AppendElements(aData->data(), csdSize);
  } else {
    
    AVCDecodeConfigDescMaker maker;
    status_t result = maker.ConvertParamSetsToDescriptorBlob(aData, aOutputBuf);
    NS_ENSURE_TRUE(result == OK, result);
  }

  return OK;
}

} 