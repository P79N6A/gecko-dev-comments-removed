




#ifndef OggWriter_h_
#define OggWriter_h_

#include "ContainerWriter.h"
#include <ogg/ogg.h>

namespace mozilla {







class OggWriter : public ContainerWriter
{
public:
  OggWriter();

  nsresult WriteEncodedTrack(const nsTArray<uint8_t>& aBuffer, int aDuration,
                             uint32_t aFlags = 0) MOZ_OVERRIDE;

  nsresult GetContainerData(nsTArray<nsTArray<uint8_t> >* aOutputBufs,
                            uint32_t aFlags = 0) MOZ_OVERRIDE;

private:
  nsresult Init();

  ogg_stream_state mOggStreamState;
  ogg_page mOggPage;
  ogg_packet mPacket;
};
}
#endif
