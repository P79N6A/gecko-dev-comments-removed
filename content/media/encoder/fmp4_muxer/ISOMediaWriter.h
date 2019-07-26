




#ifndef ISOMediaWriter_h_
#define ISOMediaWriter_h_

#include "ContainerWriter.h"
#include "nsIRunnable.h"

namespace mozilla {

class ISOControl;
class FragmentBuffer;
class AACTrackMetadata;
class AVCTrackMetadata;
class ISOMediaWriterRunnable;

class ISOMediaWriter : public ContainerWriter
{
public:
  nsresult WriteEncodedTrack(const EncodedFrameContainer &aData,
                             uint32_t aFlags = 0) MOZ_OVERRIDE;

  nsresult GetContainerData(nsTArray<nsTArray<uint8_t>>* aOutputBufs,
                            uint32_t aFlags = 0) MOZ_OVERRIDE;

  nsresult SetMetadata(TrackMetadataBase* aMetadata) MOZ_OVERRIDE;

  ISOMediaWriter(uint32_t aType);
  ~ISOMediaWriter();

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
