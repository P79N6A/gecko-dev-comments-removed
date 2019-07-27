





#include "MediaTaskQueue.h"
#include "FFmpegRuntimeLinker.h"

#include "FFmpegAudioDecoder.h"
#include "mp4_demuxer/Adts.h"

#define MAX_CHANNELS 16

typedef mp4_demuxer::MP4Sample MP4Sample;

namespace mozilla
{

FFmpegAudioDecoder<LIBAV_VER>::FFmpegAudioDecoder(
  MediaTaskQueue* aTaskQueue, MediaDataDecoderCallback* aCallback,
  const mp4_demuxer::AudioDecoderConfig& aConfig)
  : FFmpegDataDecoder(aTaskQueue, GetCodecId(aConfig.mime_type))
  , mCallback(aCallback)
{
  MOZ_COUNT_CTOR(FFmpegAudioDecoder);
  mExtraData.append(aConfig.audio_specific_config.begin(),
                    aConfig.audio_specific_config.length());
}

nsresult
FFmpegAudioDecoder<LIBAV_VER>::Init()
{
  nsresult rv = FFmpegDataDecoder::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

static AudioDataValue*
CopyAndPackAudio(AVFrame* aFrame, uint32_t aNumChannels, uint32_t aNumAFrames)
{
  MOZ_ASSERT(aNumChannels <= MAX_CHANNELS);

  nsAutoArrayPtr<AudioDataValue> audio(
    new AudioDataValue[aNumChannels * aNumAFrames]);

  if (aFrame->format == AV_SAMPLE_FMT_FLT) {
    
    
    memcpy(audio, aFrame->data[0],
           aNumChannels * aNumAFrames * sizeof(AudioDataValue));
  } else if (aFrame->format == AV_SAMPLE_FMT_FLTP) {
    
    AudioDataValue* tmp = audio;
    AudioDataValue** data = reinterpret_cast<AudioDataValue**>(aFrame->data);
    for (uint32_t frame = 0; frame < aNumAFrames; frame++) {
      for (uint32_t channel = 0; channel < aNumChannels; channel++) {
        *tmp++ = data[channel][frame];
      }
    }
  } else if (aFrame->format == AV_SAMPLE_FMT_S16) {
    
    AudioDataValue* tmp = audio;
    int16_t* data = reinterpret_cast<int16_t**>(aFrame->data)[0];
    for (uint32_t frame = 0; frame < aNumAFrames; frame++) {
      for (uint32_t channel = 0; channel < aNumChannels; channel++) {
        *tmp++ = AudioSampleToFloat(*data++);
      }
    }
  } else if (aFrame->format == AV_SAMPLE_FMT_S16P) {
    
    
    AudioDataValue* tmp = audio;
    int16_t** data = reinterpret_cast<int16_t**>(aFrame->data);
    for (uint32_t frame = 0; frame < aNumAFrames; frame++) {
      for (uint32_t channel = 0; channel < aNumChannels; channel++) {
        *tmp++ = AudioSampleToFloat(data[channel][frame]);
      }
    }
  }

  return audio.forget();
}

void
FFmpegAudioDecoder<LIBAV_VER>::DecodePacket(MP4Sample* aSample)
{
  AVPacket packet;
  av_init_packet(&packet);

  aSample->Pad(FF_INPUT_BUFFER_PADDING_SIZE);
  packet.data = aSample->data;
  packet.size = aSample->size;

  if (!PrepareFrame()) {
    NS_WARNING("FFmpeg audio decoder failed to allocate frame.");
    mCallback->Error();
    return;
  }

  int64_t samplePosition = aSample->byte_offset;
  Microseconds pts = aSample->composition_timestamp;

  while (packet.size > 0) {
    int decoded;
    int bytesConsumed =
      avcodec_decode_audio4(mCodecContext, mFrame, &decoded, &packet);

    if (bytesConsumed < 0) {
      NS_WARNING("FFmpeg audio decoder error.");
      mCallback->Error();
      return;
    }

    if (decoded) {
      uint32_t numChannels = mCodecContext->channels;
      uint32_t samplingRate = mCodecContext->sample_rate;

      nsAutoArrayPtr<AudioDataValue> audio(
        CopyAndPackAudio(mFrame, numChannels, mFrame->nb_samples));

      CheckedInt<Microseconds> duration =
        FramesToUsecs(mFrame->nb_samples, samplingRate);
      if (!duration.isValid()) {
        NS_WARNING("Invalid count of accumulated audio samples");
        mCallback->Error();
        return;
      }

      AudioData* data = new AudioData(samplePosition,
                                      pts,
                                      duration.value(),
                                      mFrame->nb_samples,
                                      audio.forget(),
                                      numChannels,
                                      samplingRate);
      mCallback->Output(data);
      pts += duration.value();
    }
    packet.data += bytesConsumed;
    packet.size -= bytesConsumed;
    samplePosition += bytesConsumed;
  }

  if (mTaskQueue->IsEmpty()) {
    mCallback->InputExhausted();
  }
}

nsresult
FFmpegAudioDecoder<LIBAV_VER>::Input(MP4Sample* aSample)
{
  mTaskQueue->Dispatch(NS_NewRunnableMethodWithArg<nsAutoPtr<MP4Sample> >(
    this, &FFmpegAudioDecoder::DecodePacket, nsAutoPtr<MP4Sample>(aSample)));

  return NS_OK;
}

nsresult
FFmpegAudioDecoder<LIBAV_VER>::Drain()
{
  mTaskQueue->AwaitIdle();
  mCallback->DrainComplete();
  return Flush();
}

AVCodecID
FFmpegAudioDecoder<LIBAV_VER>::GetCodecId(const char* aMimeType)
{
  if (!strcmp(aMimeType, "audio/mpeg")) {
    return AV_CODEC_ID_MP3;
  }

  if (!strcmp(aMimeType, "audio/mp4a-latm")) {
    return AV_CODEC_ID_AAC;
  }

  return AV_CODEC_ID_NONE;
}

FFmpegAudioDecoder<LIBAV_VER>::~FFmpegAudioDecoder()
{
  MOZ_COUNT_DTOR(FFmpegAudioDecoder);
}

} 
