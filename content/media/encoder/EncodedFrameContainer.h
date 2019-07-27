




#ifndef EncodedFrameContainer_H_
#define EncodedFrameContainer_H_

#include "nsAutoPtr.h"
#include "nsTArray.h"

namespace mozilla {

class EncodedFrame;






class EncodedFrameContainer
{
public:
  
  void AppendEncodedFrame(EncodedFrame* aEncodedFrame)
  {
    mEncodedFrames.AppendElement(aEncodedFrame);
  }
  
  const nsTArray<nsRefPtr<EncodedFrame> >& GetEncodedFrames() const
  {
    return mEncodedFrames;
  }
private:
  
  
  nsTArray<nsRefPtr<EncodedFrame> > mEncodedFrames;
};


class EncodedFrame MOZ_FINAL
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(EncodedFrame)
public:
  EncodedFrame() :
    mTimeStamp(0),
    mDuration(0),
    mFrameType(UNKNOWN)
  {}
  enum FrameType {
    VP8_I_FRAME,      
    VP8_P_FRAME,      
    OPUS_AUDIO_FRAME, 
    VORBIS_AUDIO_FRAME,
    AVC_I_FRAME,
    AVC_P_FRAME,
    AVC_B_FRAME,
    AVC_CSD,          
    AAC_AUDIO_FRAME,
    AAC_CSD,          
    AMR_AUDIO_CSD,
    AMR_AUDIO_FRAME,
    UNKNOWN           
  };
  void SwapInFrameData(nsTArray<uint8_t>& aData)
  {
    mFrameData.SwapElements(aData);
  }
  nsresult SwapOutFrameData(nsTArray<uint8_t>& aData)
  {
    if (mFrameType != UNKNOWN) {
      
      mFrameData.SwapElements(aData);
      mFrameType = UNKNOWN;
      return NS_OK;
    }
    return NS_ERROR_FAILURE;
  }
  const nsTArray<uint8_t>& GetFrameData() const
  {
    return mFrameData;
  }
  uint64_t GetTimeStamp() const { return mTimeStamp; }
  void SetTimeStamp(uint64_t aTimeStamp) { mTimeStamp = aTimeStamp; }

  uint64_t GetDuration() const { return mDuration; }
  void SetDuration(uint64_t aDuration) { mDuration = aDuration; }

  FrameType GetFrameType() const { return mFrameType; }
  void SetFrameType(FrameType aFrameType) { mFrameType = aFrameType; }
private:
  
  ~EncodedFrame()
  {
  }

  
  nsTArray<uint8_t> mFrameData;
  uint64_t mTimeStamp;
  
  uint64_t mDuration;
  
  FrameType mFrameType;
};

}
#endif
