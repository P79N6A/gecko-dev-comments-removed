




#if !defined(OpusParser_h_)
#define OpusParser_h_

#include <stdint.h>

#include <opus/opus.h>
#include "opus/opus_multistream.h"

#include "nsTArray.h"
#include "nsString.h"

namespace mozilla {

class OpusParser
{
public:
  OpusParser();

  bool DecodeHeader(unsigned char* aData, size_t aLength);
  bool DecodeTags(unsigned char* aData, size_t aLength);

  
  int mRate;        
  uint32_t mNominalRate; 
  int mChannels;    
  uint16_t mPreSkip; 
#ifdef MOZ_SAMPLE_TYPE_FLOAT32
  float mGain;      
#else
  int32_t mGain_Q16; 
#endif
  int mChannelMapping; 
  int mStreams;     
  int mCoupledStreams; 
  unsigned char mMappingTable[255]; 

  
  
  int64_t mPrevPacketGranulepos;

  nsTArray<nsCString> mTags; 

  nsCString mVendorString;   

};

} 

#endif
