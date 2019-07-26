



#ifndef __AppleMP3Reader_h__
#define __AppleMP3Reader_h__

#include "MediaDecoderReader.h"
#include "MP3FrameParser.h"
#include "VideoUtils.h"

#include <AudioToolbox/AudioToolbox.h>

namespace mozilla {

class AppleMP3Reader : public MediaDecoderReader
{
public:
  AppleMP3Reader(AbstractMediaDecoder *aDecoder);
  virtual ~AppleMP3Reader() MOZ_OVERRIDE;

  virtual nsresult Init(MediaDecoderReader* aCloneDonor) MOZ_OVERRIDE;

  nsresult PushDataToDemuxer();

  virtual bool DecodeAudioData() MOZ_OVERRIDE;
  virtual bool DecodeVideoFrame(bool &aKeyframeSkip,
                                int64_t aTimeThreshold) MOZ_OVERRIDE;

  virtual bool HasAudio() MOZ_OVERRIDE;
  virtual bool HasVideo() MOZ_OVERRIDE;

  virtual nsresult ReadMetadata(VideoInfo* aInfo,
                                MetadataTags** aTags) MOZ_OVERRIDE;

  virtual nsresult Seek(int64_t aTime,
                        int64_t aStartTime,
                        int64_t aEndTime,
                        int64_t aCurrentTime) MOZ_OVERRIDE;

  virtual nsresult GetBuffered(dom::TimeRanges* aBuffered,
                               int64_t aStartTime) MOZ_OVERRIDE;

  void AudioSampleCallback(UInt32 aNumBytes,
                           UInt32 aNumPackets,
                           const void *aData,
                           AudioStreamPacketDescription *aPackets);

  void AudioMetadataCallback(AudioFileStreamID aFileStream,
                             AudioFileStreamPropertyID aPropertyID,
                             UInt32 *aFlags);

private:
  void SetupDecoder();
  nsresult ReadAndNotify(uint32_t *aNumBytes, char *aData);

  static OSStatus PassthroughInputDataCallback(AudioConverterRef aAudioConverter,
                                               UInt32 *aNumDataPackets,
                                               AudioBufferList *aData,
                                               AudioStreamPacketDescription **aPacketDesc,
                                               void *aUserData);

  
  bool mStreamReady;

  
  
  UInt32 mAudioFramesPerCompressedPacket;
  
  UInt64 mCurrentAudioFrame;
  UInt32 mAudioChannels;
  UInt32 mAudioSampleRate;

  uint64_t mDuration;

  AudioFileStreamID mAudioFileStream;
  AudioConverterRef mAudioConverter;

  MP3FrameParser mMP3FrameParser;
};

} 

#endif 
