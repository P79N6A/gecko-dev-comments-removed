



#include "OggWriter.h"
#include "prtime.h"

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
OggWriter::WriteEncodedTrack(const EncodedFrameContainer& aData,
                             uint32_t aFlags)
{
  for (uint32_t i = 0; i < aData.GetEncodedFrames().Length(); i++) {
    if (aData.GetEncodedFrames()[i]->GetFrameType() != EncodedFrame::AUDIO_FRAME) {
      LOG("[OggWriter] wrong encoded data type!");
      return NS_ERROR_FAILURE;
    }

    nsresult rv = WriteEncodedData(aData.GetEncodedFrames()[i]->GetFrameData(),
                                   aData.GetEncodedFrames()[i]->GetDuration(),
                                   aFlags);
    if (NS_FAILED(rv)) {
      LOG("%p Failed to WriteEncodedTrack!", this);
      return rv;
    }
  }
  return NS_OK;
}

nsresult
OggWriter::WriteEncodedData(const nsTArray<uint8_t>& aBuffer, int aDuration,
                            uint32_t aFlags)
{
  if (!mInitialized) {
    LOG("[OggWriter] OggWriter has not initialized!");
    return NS_ERROR_FAILURE;
  }

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

void
OggWriter::ProduceOggPage(nsTArray<nsTArray<uint8_t> >* aOutputBufs)
{
  aOutputBufs->AppendElement();
  aOutputBufs->LastElement().SetLength(mOggPage.header_len +
                                       mOggPage.body_len);
  memcpy(aOutputBufs->LastElement().Elements(), mOggPage.header,
         mOggPage.header_len);
  memcpy(aOutputBufs->LastElement().Elements() + mOggPage.header_len,
         mOggPage.body, mOggPage.body_len);
}

nsresult
OggWriter::GetContainerData(nsTArray<nsTArray<uint8_t> >* aOutputBufs,
                            uint32_t aFlags)
{
  int rc = -1;
  
  if (aFlags & ContainerWriter::GET_HEADER) {
    OpusMetadata* meta = static_cast<OpusMetadata*>(mMetadata.get());
    NS_ASSERTION(meta, "should have meta data");
    NS_ASSERTION(meta->GetKind() == TrackMetadataBase::METADATA_OPUS,
                 "should have Opus meta data");

    nsresult rv = WriteEncodedData(meta->mIdHeader, 0);
    NS_ENSURE_SUCCESS(rv, rv);

    rc = ogg_stream_flush(&mOggStreamState, &mOggPage);
    NS_ENSURE_TRUE(rc > 0, NS_ERROR_FAILURE);
    ProduceOggPage(aOutputBufs);

    rv = WriteEncodedData(meta->mCommentHeader, 0);
    NS_ENSURE_SUCCESS(rv, rv);

    rc = ogg_stream_flush(&mOggStreamState, &mOggPage);
    NS_ENSURE_TRUE(rc > 0, NS_ERROR_FAILURE);

    ProduceOggPage(aOutputBufs);
    return NS_OK;

  
  
  } else if (aFlags & ContainerWriter::FLUSH_NEEDED) {
    
    rc = ogg_stream_flush(&mOggStreamState, &mOggPage);
  } else {
    
    
    rc = ogg_stream_pageout(&mOggStreamState, &mOggPage);
  }

  if (rc) {
    ProduceOggPage(aOutputBufs);
  }

  return (rc > 0) ? NS_OK : NS_ERROR_FAILURE;
}

nsresult
OggWriter::SetMetadata(TrackMetadataBase* aMetadata)
{
  MOZ_ASSERT(aMetadata);
  if (aMetadata->GetKind() != TrackMetadataBase::METADATA_OPUS) {
    LOG("wrong meta data type!");
    return NS_ERROR_FAILURE;
  }
  
  mMetadata = static_cast<OpusMetadata*>(aMetadata);
  if (mMetadata->mIdHeader.Length() == 0) {
    LOG("miss mIdHeader!");
    return NS_ERROR_FAILURE;
  }
  if (mMetadata->mCommentHeader.Length() == 0) {
    LOG("miss mCommentHeader!");
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

}
