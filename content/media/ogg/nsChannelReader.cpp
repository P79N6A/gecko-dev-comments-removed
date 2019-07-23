




































#include "nsAString.h"
#include "nsThreadUtils.h"
#include "nsNetUtil.h"
#include "prlog.h"
#include "nsOggDecoder.h"
#include "nsChannelReader.h"
#include "nsIScriptSecurityManager.h"

OggPlayErrorCode nsChannelReader::initialise(int aBlock)
{
  return E_OGGPLAY_OK;
}

OggPlayErrorCode nsChannelReader::destroy()
{
  
  return E_OGGPLAY_OK;
}

void nsChannelReader::SetLastFrameTime(PRInt64 aTime)
{
  mLastFrameTime = aTime;
}

size_t nsChannelReader::io_read(char* aBuffer, size_t aCount)
{
  PRUint32 bytes = 0;
  nsresult rv = mStream->Read(aBuffer, aCount, &bytes);
  if (!NS_SUCCEEDED(rv)) {
    return static_cast<size_t>(OGGZ_ERR_SYSTEM);
  }
  nsOggDecoder* decoder =
    static_cast<nsOggDecoder*>(mStream->Decoder());
  decoder->NotifyBytesConsumed(bytes);
  return bytes;
}

int nsChannelReader::io_seek(long aOffset, int aWhence)
{
  nsresult rv = mStream->Seek(aWhence, aOffset);
  if (NS_SUCCEEDED(rv))
    return aOffset;
  
  return OGGZ_STOP_ERR;
}

long nsChannelReader::io_tell()
{
  return mStream->Tell();
}

ogg_int64_t nsChannelReader::duration()
{
  return mLastFrameTime;
}

static OggPlayErrorCode oggplay_channel_reader_initialise(OggPlayReader* aReader, int aBlock) 
{
  nsChannelReader * me = static_cast<nsChannelReader*>(aReader);

  if (me == NULL) {
    return E_OGGPLAY_BAD_READER;
  }
  return me->initialise(aBlock);
}

static OggPlayErrorCode oggplay_channel_reader_destroy(OggPlayReader* aReader) 
{
  nsChannelReader* me = static_cast<nsChannelReader*>(aReader);
  return me->destroy();
}

static size_t oggplay_channel_reader_io_read(void* aReader, void* aBuffer, size_t aCount) 
{
  nsChannelReader* me = static_cast<nsChannelReader*>(aReader);
  return me->io_read(static_cast<char*>(aBuffer), aCount);
}

static int oggplay_channel_reader_io_seek(void* aReader, long aOffset, int aWhence) 
{
  nsChannelReader* me = static_cast<nsChannelReader*>(aReader);
  return me->io_seek(aOffset, aWhence);
}

static long oggplay_channel_reader_io_tell(void* aReader) 
{
  nsChannelReader* me = static_cast<nsChannelReader*>(aReader);
  return me->io_tell();
}

static ogg_int64_t oggplay_channel_reader_duration(struct _OggPlayReader *aReader)
{
  nsChannelReader* me = static_cast<nsChannelReader*>(aReader);
  return me->duration();
}

void nsChannelReader::Init(nsMediaStream* aStream)
{
  mStream = aStream;
}

nsChannelReader::~nsChannelReader()
{
  MOZ_COUNT_DTOR(nsChannelReader);
}

nsChannelReader::nsChannelReader() :
  mLastFrameTime(-1)
{
  MOZ_COUNT_CTOR(nsChannelReader);
  OggPlayReader* reader = this;
  reader->initialise = &oggplay_channel_reader_initialise;
  reader->destroy = &oggplay_channel_reader_destroy;
  reader->seek = 0;
  reader->io_read  = &oggplay_channel_reader_io_read;
  reader->io_seek  = &oggplay_channel_reader_io_seek;
  reader->io_tell  = &oggplay_channel_reader_io_tell;
  reader->duration = &oggplay_channel_reader_duration;
}
