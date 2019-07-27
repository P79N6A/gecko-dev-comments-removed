




#ifndef EbmlComposer_h_
#define EbmlComposer_h_
#include "nsTArray.h"
#include "ContainerWriter.h"

namespace mozilla {




class EbmlComposer {
public:
  EbmlComposer();
  


  void SetVideoConfig(uint32_t aWidth, uint32_t aHeight, uint32_t aDisplayWidth,
                      uint32_t aDisplayHeight, float aFrameRate);

  void SetAudioConfig(uint32_t aSampleFreq, uint32_t aChannels,
                      uint32_t bitDepth);
  


  void SetAudioCodecPrivateData(nsTArray<uint8_t>& aBufs)
  {
    mCodecPrivateData.AppendElements(aBufs);
  }
  


  void GenerateHeader();
  



  void WriteSimpleBlock(EncodedFrame* aFrame);
  


  void ExtractBuffer(nsTArray<nsTArray<uint8_t> >* aDestBufs,
                     uint32_t aFlag = 0);
private:
  
  void FinishMetadata();
  
  void FinishCluster();
  
  nsTArray<nsTArray<uint8_t> > mClusterBuffs;
  
  nsTArray<nsTArray<uint8_t> > mClusterCanFlushBuffs;

  
  enum {
    FLUSH_NONE = 0,
    FLUSH_METADATA = 1 << 0,
    FLUSH_CLUSTER = 1 << 1
  };
  uint32_t mFlushState;
  
  uint32_t mClusterHeaderIndex;
  
  uint64_t mClusterLengthLoc;
  
  nsTArray<uint8_t> mCodecPrivateData;

  
  uint64_t mClusterTimecode;

  
  int mWidth;
  int mHeight;
  int mDisplayWidth;
  int mDisplayHeight;
  float mFrameRate;
  
  float mSampleFreq;
  int mBitDepth;
  int mChannels;
};

}
#endif
