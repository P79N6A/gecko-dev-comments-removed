




#ifndef EncodedFrameContainer_H_
#define EncodedFrameContainer_H_

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


class EncodedFrame
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(EncodedFrame)
public:
  EncodedFrame() :
    mTimeStamp(0),
    mDuration(0),
    mFrameType(UNKNOW)
  {}
  enum FrameType {
    I_FRAME,      
    P_FRAME,      
    B_FRAME,      
    AUDIO_FRAME,  
    UNKNOW        
  };
  const nsTArray<uint8_t>& GetFrameData() const
  {
    return mFrameData;
  }
  void SetFrameData(nsTArray<uint8_t> *aData)
  {
    mFrameData.SwapElements(*aData);
  }
  uint64_t GetTimeStamp() const { return mTimeStamp; }
  void SetTimeStamp(uint64_t aTimeStamp) { mTimeStamp = aTimeStamp; }

  uint64_t GetDuration() const { return mDuration; }
  void SetDuration(uint64_t aDuration) { mDuration = aDuration; }

  FrameType GetFrameType() const { return mFrameType; }
  void SetFrameType(FrameType aFrameType) { mFrameType = aFrameType; }
private:
  
  nsTArray<uint8_t> mFrameData;
  uint64_t mTimeStamp;
  
  uint64_t mDuration;
  
  FrameType mFrameType;
};

}
#endif
