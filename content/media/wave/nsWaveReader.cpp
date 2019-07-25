




































#include "nsError.h"
#include "nsBuiltinDecoderStateMachine.h"
#include "nsBuiltinDecoder.h"
#include "nsMediaStream.h"
#include "nsWaveReader.h"
#include "nsTimeRanges.h"
#include "VideoUtils.h"

using namespace mozilla;




#ifdef PR_LOGGING
extern PRLogModuleInfo* gBuiltinDecoderLog;
#define LOG(type, msg) PR_LOG(gBuiltinDecoderLog, type, msg)
#ifdef SEEK_LOGGING
#define SEEK_LOG(type, msg) PR_LOG(gBuiltinDecoderLog, type, msg)
#else
#define SEEK_LOG(type, msg)
#endif
#else
#define LOG(type, msg)
#define SEEK_LOG(type, msg)
#endif


#define RIFF_CHUNK_MAGIC 0x52494646
#define WAVE_CHUNK_MAGIC 0x57415645
#define FRMT_CHUNK_MAGIC 0x666d7420
#define DATA_CHUNK_MAGIC 0x64617461


#define RIFF_CHUNK_HEADER_SIZE 8


#define RIFF_INITIAL_SIZE (RIFF_CHUNK_HEADER_SIZE + 4)



#define WAVE_FORMAT_CHUNK_SIZE 16



#define WAVE_FORMAT_ENCODING_PCM 1


#define MAX_CHANNELS 2

namespace {
  PRUint32
  ReadUint32BE(const char** aBuffer)
  {
    PRUint32 result =
      PRUint8((*aBuffer)[0]) << 24 |
      PRUint8((*aBuffer)[1]) << 16 |
      PRUint8((*aBuffer)[2]) << 8 |
      PRUint8((*aBuffer)[3]);
    *aBuffer += sizeof(PRUint32);
    return result;
  }

  PRUint32
  ReadUint32LE(const char** aBuffer)
  {
    PRUint32 result =
      PRUint8((*aBuffer)[3]) << 24 |
      PRUint8((*aBuffer)[2]) << 16 |
      PRUint8((*aBuffer)[1]) << 8 |
      PRUint8((*aBuffer)[0]);
    *aBuffer += sizeof(PRUint32);
    return result;
  }

  PRUint16
  ReadUint16LE(const char** aBuffer)
  {
    PRUint16 result =
      PRUint8((*aBuffer)[1]) << 8 |
      PRUint8((*aBuffer)[0]) << 0;
    *aBuffer += sizeof(PRUint16);
    return result;
  }

  PRInt16
  ReadInt16LE(const char** aBuffer)
  {
    return static_cast<PRInt16>(ReadUint16LE(aBuffer));
  }

  PRUint8
  ReadUint8(const char** aBuffer)
  {
    PRUint8 result = PRUint8((*aBuffer)[0]);
    *aBuffer += sizeof(PRUint8);
    return result;
  }
}

nsWaveReader::nsWaveReader(nsBuiltinDecoder* aDecoder)
  : nsBuiltinDecoderReader(aDecoder)
{
  MOZ_COUNT_CTOR(nsWaveReader);
}

nsWaveReader::~nsWaveReader()
{
  MOZ_COUNT_DTOR(nsWaveReader);
}

nsresult nsWaveReader::Init(nsBuiltinDecoderReader* aCloneDonor)
{
  return NS_OK;
}

nsresult nsWaveReader::ReadMetadata(nsVideoInfo* aInfo)
{
  NS_ASSERTION(mDecoder->OnStateMachineThread(), "Should be on state machine thread.");
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  PRBool loaded = LoadRIFFChunk() && LoadFormatChunk() && FindDataOffset();
  if (!loaded) {
    return NS_ERROR_FAILURE;
  }

  mInfo.mHasAudio = PR_TRUE;
  mInfo.mHasVideo = PR_FALSE;
  mInfo.mAudioRate = mSampleRate;
  mInfo.mAudioChannels = mChannels;

  *aInfo = mInfo;

  ReentrantMonitorAutoExit exitReaderMon(mReentrantMonitor);
  ReentrantMonitorAutoEnter decoderMon(mDecoder->GetReentrantMonitor());

  mDecoder->GetStateMachine()->SetDuration(
    static_cast<PRInt64>(BytesToTime(GetDataLength()) * USECS_PER_S));

  return NS_OK;
}

PRBool nsWaveReader::DecodeAudioData()
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  NS_ASSERTION(mDecoder->OnStateMachineThread() || mDecoder->OnDecodeThread(),
               "Should be on state machine thread or decode thread.");

  PRInt64 pos = GetPosition() - mWavePCMOffset;
  PRInt64 len = GetDataLength();
  PRInt64 remaining = len - pos;
  NS_ASSERTION(remaining >= 0, "Current wave position is greater than wave file length");

  static const PRInt64 BLOCK_SIZE = 4096;
  PRInt64 readSize = NS_MIN(BLOCK_SIZE, remaining);
  PRInt64 samples = readSize / mSampleSize;

  PR_STATIC_ASSERT(PRUint64(BLOCK_SIZE) < UINT_MAX / sizeof(SoundDataValue) / MAX_CHANNELS);
  const size_t bufferSize = static_cast<size_t>(samples * mChannels);
  nsAutoArrayPtr<SoundDataValue> sampleBuffer(new SoundDataValue[bufferSize]);

  PR_STATIC_ASSERT(PRUint64(BLOCK_SIZE) < UINT_MAX / sizeof(char));
  nsAutoArrayPtr<char> dataBuffer(new char[static_cast<size_t>(readSize)]);

  if (!ReadAll(dataBuffer, readSize)) {
    mAudioQueue.Finish();
    return PR_FALSE;
  }

  
  const char* d = dataBuffer.get();
  SoundDataValue* s = sampleBuffer.get();
  for (int i = 0; i < samples; ++i) {
    for (unsigned int j = 0; j < mChannels; ++j) {
      if (mSampleFormat == nsAudioStream::FORMAT_U8) {
        PRUint8 v =  ReadUint8(&d);
#if defined(MOZ_SAMPLE_TYPE_S16LE)
        *s++ = (v * (1.F/PR_UINT8_MAX)) * PR_UINT16_MAX + PR_INT16_MIN;
#elif defined(MOZ_SAMPLE_TYPE_FLOAT32)
        *s++ = (v * (1.F/PR_UINT8_MAX)) * 2.F - 1.F;
#endif
      }
      else if (mSampleFormat == nsAudioStream::FORMAT_S16_LE) {
        PRInt16 v =  ReadInt16LE(&d);
#if defined(MOZ_SAMPLE_TYPE_S16LE)
        *s++ = v;
#elif defined(MOZ_SAMPLE_TYPE_FLOAT32)
        *s++ = (PRInt32(v) - PR_INT16_MIN) / float(PR_UINT16_MAX) * 2.F - 1.F;
#endif
      }
    }
  }

  double posTime = BytesToTime(pos);
  double readSizeTime = BytesToTime(readSize);
  NS_ASSERTION(posTime <= PR_INT64_MAX / USECS_PER_S, "posTime overflow");
  NS_ASSERTION(readSizeTime <= PR_INT64_MAX / USECS_PER_S, "readSizeTime overflow");
  NS_ASSERTION(samples < PR_INT32_MAX, "samples overflow");

  mAudioQueue.Push(new SoundData(pos,
                                 static_cast<PRInt64>(posTime * USECS_PER_S),
                                 static_cast<PRInt64>(readSizeTime * USECS_PER_S),
                                 static_cast<PRInt32>(samples),
                                 sampleBuffer.forget(),
                                 mChannels));

  return PR_TRUE;
}

PRBool nsWaveReader::DecodeVideoFrame(PRBool &aKeyframeSkip,
                                      PRInt64 aTimeThreshold)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  NS_ASSERTION(mDecoder->OnStateMachineThread() || mDecoder->OnDecodeThread(),
               "Should be on state machine or decode thread.");

  return PR_FALSE;
}

nsresult nsWaveReader::Seek(PRInt64 aTarget, PRInt64 aStartTime, PRInt64 aEndTime, PRInt64 aCurrentTime)
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  NS_ASSERTION(mDecoder->OnStateMachineThread(),
               "Should be on state machine thread.");
  LOG(PR_LOG_DEBUG, ("%p About to seek to %lld", mDecoder, aTarget));
  if (NS_FAILED(ResetDecode())) {
    return NS_ERROR_FAILURE;
  }
  double d = BytesToTime(GetDataLength());
  NS_ASSERTION(d < PR_INT64_MAX / USECS_PER_S, "Duration overflow"); 
  PRInt64 duration = static_cast<PRInt64>(d * USECS_PER_S);
  double seekTime = NS_MIN(aTarget, duration) / static_cast<double>(USECS_PER_S);
  PRInt64 position = RoundDownToSample(static_cast<PRInt64>(TimeToBytes(seekTime)));
  NS_ASSERTION(PR_INT64_MAX - mWavePCMOffset > position, "Integer overflow during wave seek");
  position += mWavePCMOffset;
  return mDecoder->GetCurrentStream()->Seek(nsISeekableStream::NS_SEEK_SET, position);
}

static double RoundToUsecs(double aSeconds) {
  return floor(aSeconds * USECS_PER_S) / USECS_PER_S;
}

nsresult nsWaveReader::GetBuffered(nsTimeRanges* aBuffered, PRInt64 aStartTime)
{
  PRInt64 startOffset = mDecoder->GetCurrentStream()->GetNextCachedData(mWavePCMOffset);
  while (startOffset >= 0) {
    PRInt64 endOffset = mDecoder->GetCurrentStream()->GetCachedDataEnd(startOffset);
    
    NS_ASSERTION(startOffset >= mWavePCMOffset, "Integer underflow in GetBuffered");
    NS_ASSERTION(endOffset >= mWavePCMOffset, "Integer underflow in GetBuffered");

    
    
    
    aBuffered->Add(RoundToUsecs(BytesToTime(startOffset - mWavePCMOffset)),
                   RoundToUsecs(BytesToTime(endOffset - mWavePCMOffset)));
    startOffset = mDecoder->GetCurrentStream()->GetNextCachedData(endOffset);
  }
  return NS_OK;
}

PRBool
nsWaveReader::ReadAll(char* aBuf, PRInt64 aSize, PRInt64* aBytesRead)
{
  PRUint32 got = 0;
  if (aBytesRead) {
    *aBytesRead = 0;
  }
  do {
    PRUint32 read = 0;
    if (NS_FAILED(mDecoder->GetCurrentStream()->Read(aBuf + got, PRUint32(aSize - got), &read))) {
      NS_WARNING("Stream read failed");
      return PR_FALSE;
    }
    if (read == 0) {
      return PR_FALSE;
    }
    mDecoder->NotifyBytesConsumed(read);
    got += read;
    if (aBytesRead) {
      *aBytesRead = got;
    }
  } while (got != aSize);
  return PR_TRUE;
}

PRBool
nsWaveReader::LoadRIFFChunk()
{
  char riffHeader[RIFF_INITIAL_SIZE];
  const char* p = riffHeader;

  NS_ABORT_IF_FALSE(mDecoder->GetCurrentStream()->Tell() == 0,
                    "LoadRIFFChunk called when stream in invalid state");

  if (!ReadAll(riffHeader, sizeof(riffHeader))) {
    return PR_FALSE;
  }

  PR_STATIC_ASSERT(sizeof(PRUint32) * 2 <= RIFF_INITIAL_SIZE);
  if (ReadUint32BE(&p) != RIFF_CHUNK_MAGIC) {
    NS_WARNING("Stream data not in RIFF format");
    return PR_FALSE;
  }

  
  p += 4;

  if (ReadUint32BE(&p) != WAVE_CHUNK_MAGIC) {
    NS_WARNING("Expected WAVE chunk");
    return PR_FALSE;
  }

  return PR_TRUE;
}

PRBool
nsWaveReader::ScanForwardUntil(PRUint32 aWantedChunk, PRUint32* aChunkSize)
{
  NS_ABORT_IF_FALSE(aChunkSize, "Require aChunkSize argument");
  *aChunkSize = 0;

  for (;;) {
    static const unsigned int CHUNK_HEADER_SIZE = 8;
    char chunkHeader[CHUNK_HEADER_SIZE];
    const char* p = chunkHeader;

    if (!ReadAll(chunkHeader, sizeof(chunkHeader))) {
      return PR_FALSE;
    }

    PR_STATIC_ASSERT(sizeof(PRUint32) * 2 <= CHUNK_HEADER_SIZE);
    PRUint32 magic = ReadUint32BE(&p);
    PRUint32 chunkSize = ReadUint32LE(&p);

    if (magic == aWantedChunk) {
      *aChunkSize = chunkSize;
      return PR_TRUE;
    }

    
    chunkSize += chunkSize % 2;

    static const unsigned int MAX_CHUNK_SIZE = 1 << 16;
    PR_STATIC_ASSERT(MAX_CHUNK_SIZE < UINT_MAX / sizeof(char));
    nsAutoArrayPtr<char> chunk(new char[MAX_CHUNK_SIZE]);
    while (chunkSize > 0) {
      PRUint32 size = NS_MIN(chunkSize, MAX_CHUNK_SIZE);
      if (!ReadAll(chunk.get(), size)) {
        return PR_FALSE;
      }
      chunkSize -= size;
    }
  }
}

PRBool
nsWaveReader::LoadFormatChunk()
{
  PRUint32 fmtSize, rate, channels, sampleSize, sampleFormat;
  char waveFormat[WAVE_FORMAT_CHUNK_SIZE];
  const char* p = waveFormat;

  
  NS_ABORT_IF_FALSE(mDecoder->GetCurrentStream()->Tell() % 2 == 0,
                    "LoadFormatChunk called with unaligned stream");

  
  
  if (!ScanForwardUntil(FRMT_CHUNK_MAGIC, &fmtSize)) {
    return PR_FALSE;
  }

  if (!ReadAll(waveFormat, sizeof(waveFormat))) {
    return PR_FALSE;
  }

  PR_STATIC_ASSERT(sizeof(PRUint16) +
                   sizeof(PRUint16) +
                   sizeof(PRUint32) +
                   4 +
                   sizeof(PRUint16) +
                   sizeof(PRUint16) <= sizeof(waveFormat));
  if (ReadUint16LE(&p) != WAVE_FORMAT_ENCODING_PCM) {
    NS_WARNING("WAVE is not uncompressed PCM, compressed encodings are not supported");
    return PR_FALSE;
  }

  channels = ReadUint16LE(&p);
  rate = ReadUint32LE(&p);

  
  p += 4;

  sampleSize = ReadUint16LE(&p);

  sampleFormat = ReadUint16LE(&p);

  
  
  
  
  
  if (fmtSize > WAVE_FORMAT_CHUNK_SIZE) {
    char extLength[2];
    const char* p = extLength;

    if (!ReadAll(extLength, sizeof(extLength))) {
      return PR_FALSE;
    }

    PR_STATIC_ASSERT(sizeof(PRUint16) <= sizeof(extLength));
    PRUint16 extra = ReadUint16LE(&p);
    if (fmtSize - (WAVE_FORMAT_CHUNK_SIZE + 2) != extra) {
      NS_WARNING("Invalid extended format chunk size");
      return PR_FALSE;
    }
    extra += extra % 2;

    if (extra > 0) {
      PR_STATIC_ASSERT(PR_UINT16_MAX + (PR_UINT16_MAX % 2) < UINT_MAX / sizeof(char));
      nsAutoArrayPtr<char> chunkExtension(new char[extra]);
      if (!ReadAll(chunkExtension.get(), extra)) {
        return PR_FALSE;
      }
    }
  }

  
  NS_ABORT_IF_FALSE(mDecoder->GetCurrentStream()->Tell() % 2 == 0,
                    "LoadFormatChunk left stream unaligned");

  
  
  
  if (rate < 100 || rate > 96000 ||
      channels < 1 || channels > MAX_CHANNELS ||
      (sampleSize != 1 && sampleSize != 2 && sampleSize != 4) ||
      (sampleFormat != 8 && sampleFormat != 16)) {
    NS_WARNING("Invalid WAVE metadata");
    return PR_FALSE;
  }

  ReentrantMonitorAutoEnter monitor(mDecoder->GetReentrantMonitor());
  mSampleRate = rate;
  mChannels = channels;
  mSampleSize = sampleSize;
  if (sampleFormat == 8) {
    mSampleFormat = nsAudioStream::FORMAT_U8;
  } else {
    mSampleFormat = nsAudioStream::FORMAT_S16_LE;
  }
  return PR_TRUE;
}

PRBool
nsWaveReader::FindDataOffset()
{
  
  NS_ABORT_IF_FALSE(mDecoder->GetCurrentStream()->Tell() % 2 == 0,
                    "FindDataOffset called with unaligned stream");

  
  
  PRUint32 length;
  if (!ScanForwardUntil(DATA_CHUNK_MAGIC, &length)) {
    return PR_FALSE;
  }

  PRInt64 offset = mDecoder->GetCurrentStream()->Tell();
  if (offset <= 0 || offset > PR_UINT32_MAX) {
    NS_WARNING("PCM data offset out of range");
    return PR_FALSE;
  }

  ReentrantMonitorAutoEnter monitor(mDecoder->GetReentrantMonitor());
  mWaveLength = length;
  mWavePCMOffset = PRUint32(offset);
  return PR_TRUE;
}

double
nsWaveReader::BytesToTime(PRInt64 aBytes) const
{
  NS_ABORT_IF_FALSE(aBytes >= 0, "Must be >= 0");
  return float(aBytes) / mSampleRate / mSampleSize;
}

PRInt64
nsWaveReader::TimeToBytes(double aTime) const
{
  NS_ABORT_IF_FALSE(aTime >= 0.0f, "Must be >= 0");
  return RoundDownToSample(PRInt64(aTime * mSampleRate * mSampleSize));
}

PRInt64
nsWaveReader::RoundDownToSample(PRInt64 aBytes) const
{
  NS_ABORT_IF_FALSE(aBytes >= 0, "Must be >= 0");
  return aBytes - (aBytes % mSampleSize);
}

PRInt64
nsWaveReader::GetDataLength()
{
  PRInt64 length = mWaveLength;
  
  
  
  PRInt64 streamLength = mDecoder->GetCurrentStream()->GetLength();
  if (streamLength >= 0) {
    PRInt64 dataLength = NS_MAX<PRInt64>(0, streamLength - mWavePCMOffset);
    length = NS_MIN(dataLength, length);
  }
  return length;
}

PRInt64
nsWaveReader::GetPosition()
{
  return mDecoder->GetCurrentStream()->Tell();
}
