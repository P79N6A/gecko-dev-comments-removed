




#ifndef ISOMediaWriter_h_
#define ISOMediaWriter_h_

#include "ContainerWriter.h"
#include "nsIRunnable.h"

namespace mozilla {

class ISOControl;
class FragmentBuffer;

class ISOMediaWriter : public ContainerWriter
{
public:
  
  
  const static uint32_t TYPE_FRAG_MP4 = 1 << 0;

  
  
  
  const static uint32_t TYPE_FRAG_3GP = 1 << 1;

  
  
  
  
  
  
  ISOMediaWriter(uint32_t aType, uint32_t aHint = TYPE_FRAG_MP4);
  ~ISOMediaWriter();

  
  nsresult WriteEncodedTrack(const EncodedFrameContainer &aData,
                             uint32_t aFlags = 0) override;

  nsresult GetContainerData(nsTArray<nsTArray<uint8_t>>* aOutputBufs,
                            uint32_t aFlags = 0) override;

  nsresult SetMetadata(TrackMetadataBase* aMetadata) override;

protected:
  


















  enum MuxState {
    MUXING_HEAD,
    MUXING_FRAG,
    MUXING_DONE,
  };

private:
  nsresult RunState();

  
  
  
  
  bool ReadyToRunState(bool& aEOS);

  
  
  nsAutoPtr<ISOControl> mControl;

  
  
  
  nsAutoPtr<FragmentBuffer> mAudioFragmentBuffer;
  nsAutoPtr<FragmentBuffer> mVideoFragmentBuffer;

  MuxState mState;

  
  bool mBlobReady;

  
  uint32_t mType;
};

} 
#endif
