




#ifndef OggWriter_h_
#define OggWriter_h_

#include "ContainerWriter.h"
#include "OpusTrackEncoder.h"
#include <ogg/ogg.h>

namespace mozilla {







class OggWriter : public ContainerWriter
{
public:
  OggWriter();
  ~OggWriter();

  nsresult WriteEncodedTrack(const EncodedFrameContainer &aData,
                             uint32_t aFlags = 0) override;

  nsresult GetContainerData(nsTArray<nsTArray<uint8_t> >* aOutputBufs,
                            uint32_t aFlags = 0) override;

  
  nsresult SetMetadata(TrackMetadataBase* aMetadata) override;
private:
  nsresult Init();

  nsresult WriteEncodedData(const nsTArray<uint8_t>& aBuffer, int aDuration,
                            uint32_t aFlags = 0);

  void ProduceOggPage(nsTArray<nsTArray<uint8_t> >* aOutputBufs);
  
  nsRefPtr<OpusMetadata> mMetadata;

  ogg_stream_state mOggStreamState;
  ogg_page mOggPage;
  ogg_packet mPacket;
};

} 

#endif
