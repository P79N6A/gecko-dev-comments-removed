



#include "OggWriter.h"

#undef LOG
#ifdef MOZ_WIDGET_GONK
#include <android/log.h>
#define LOG(args...) __android_log_print(ANDROID_LOG_INFO, "MediaEncoder", ## args);
#else
#define LOG(args, ...)
#endif

namespace mozilla {

OggWriter::OggWriter() : ContainerWriter()
{
  if (NS_FAILED(Init())) {
    LOG("ERROR! Fail to initialize the OggWriter.");
  }
}

nsresult
OggWriter::Init()
{
  MOZ_ASSERT(!mInitialized);

  
  
  
  srand(static_cast<unsigned>(PR_Now()));
  int rc = ogg_stream_init(&mOggStreamState, rand());

  mPacket.b_o_s = 1;
  mPacket.e_o_s = 0;
  mPacket.granulepos = 0;
  mPacket.packet = nullptr;
  mPacket.packetno = 0;
  mPacket.bytes = 0;

  mInitialized = (rc == 0);

  return (rc == 0) ? NS_OK : NS_ERROR_NOT_INITIALIZED;
}

nsresult
OggWriter::WriteEncodedTrack(const nsTArray<uint8_t>& aBuffer, int aDuration,
                             uint32_t aFlags)
{
  MOZ_ASSERT(!ogg_stream_eos(&mOggStreamState),
             "No data can be written after eos has marked.");

  
  
  if (aFlags & ContainerWriter::END_OF_STREAM) {
    LOG("[OggWriter] Set e_o_s flag to true.");
    mPacket.e_o_s = 1;
  }

  mPacket.packet = const_cast<uint8_t*>(aBuffer.Elements());
  mPacket.bytes = aBuffer.Length();
  mPacket.granulepos += aDuration;

  
  
  
  
  int rc = ogg_stream_packetin(&mOggStreamState, &mPacket);
  if (rc < 0) {
    LOG("[OggWriter] Failed in ogg_stream_packetin! (%d).", rc);
    return NS_ERROR_FAILURE;
  }

  if (mPacket.b_o_s) {
    mPacket.b_o_s = 0;
  }
  mPacket.packetno++;
  mPacket.packet = nullptr;

  return NS_OK;
}

nsresult
OggWriter::GetContainerData(nsTArray<nsTArray<uint8_t> >* aOutputBufs,
                            uint32_t aFlags)
{
  int rc = -1;
  
  
  if (aFlags & ContainerWriter::FLUSH_NEEDED) {
    
    rc = ogg_stream_flush(&mOggStreamState, &mOggPage);
  } else {
    
    
    rc = ogg_stream_pageout(&mOggStreamState, &mOggPage);
  }

  if (rc) {
    aOutputBufs->AppendElement();
    aOutputBufs->LastElement().SetLength(mOggPage.header_len +
                                         mOggPage.body_len);
    memcpy(aOutputBufs->LastElement().Elements(), mOggPage.header,
           mOggPage.header_len);
    memcpy(aOutputBufs->LastElement().Elements() + mOggPage.header_len,
           mOggPage.body, mOggPage.body_len);
  }

  return (rc > 0) ? NS_OK : NS_ERROR_FAILURE;
}

}
