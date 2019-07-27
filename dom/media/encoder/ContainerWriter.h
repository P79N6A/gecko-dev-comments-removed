




#ifndef ContainerWriter_h_
#define ContainerWriter_h_

#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "EncodedFrameContainer.h"
#include "TrackMetadataBase.h"

namespace mozilla {



class ContainerWriter {
public:
  ContainerWriter()
    : mInitialized(false)
    , mIsWritingComplete(false)
  {}
  virtual ~ContainerWriter() {}
  
  enum {
    CREATE_AUDIO_TRACK = 1 << 0,
    CREATE_VIDEO_TRACK = 1 << 1,
  };
  enum {
    END_OF_STREAM = 1 << 0
  };

  






  virtual nsresult WriteEncodedTrack(const EncodedFrameContainer& aData,
                                     uint32_t aFlags = 0) = 0;

  





  virtual nsresult SetMetadata(TrackMetadataBase* aMetadata) = 0;

  


  virtual bool IsWritingComplete() { return mIsWritingComplete; }

  enum {
    FLUSH_NEEDED = 1 << 0,
    GET_HEADER = 1 << 1
  };

  







  virtual nsresult GetContainerData(nsTArray<nsTArray<uint8_t> >* aOutputBufs,
                                    uint32_t aFlags = 0) = 0;
protected:
  bool mInitialized;
  bool mIsWritingComplete;
};
}
#endif
