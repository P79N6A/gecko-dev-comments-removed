




#include "AudioNodeEngine.h"
#include "AudioNodeExternalInputStream.h"
#include "AudioChannelFormat.h"
#include "mozilla/dom/MediaStreamAudioSourceNode.h"

using namespace mozilla::dom;

namespace mozilla {

AudioNodeExternalInputStream::AudioNodeExternalInputStream(AudioNodeEngine* aEngine, TrackRate aSampleRate)
  : AudioNodeStream(aEngine, MediaStreamGraph::INTERNAL_STREAM, aSampleRate)
  , mCurrentOutputPosition(0)
{
  MOZ_COUNT_CTOR(AudioNodeExternalInputStream);
}

AudioNodeExternalInputStream::~AudioNodeExternalInputStream()
{
  MOZ_COUNT_DTOR(AudioNodeExternalInputStream);
}

AudioNodeExternalInputStream::TrackMapEntry::~TrackMapEntry()
{
  if (mResampler) {
    speex_resampler_destroy(mResampler);
  }
}

size_t
AudioNodeExternalInputStream::GetTrackMapEntry(const StreamBuffer::Track& aTrack,
                                               GraphTime aFrom)
{
  AudioSegment* segment = aTrack.Get<AudioSegment>();

  
  for (size_t i = 0; i < mTrackMap.Length(); ++i) {
    TrackMapEntry* map = &mTrackMap[i];
    if (map->mTrackID == aTrack.GetID()) {
      return i;
    }
  }

  
  AudioSegment::ChunkIterator ci(*segment);
  while (!ci.IsEnded() && ci->IsNull()) {
    ci.Next();
  }
  if (ci.IsEnded()) {
    
    return nsTArray<TrackMapEntry>::NoIndex;
  }

  
  
  SpeexResamplerState* resampler = nullptr;
  size_t channelCount = std::min((*ci).mChannelData.Length(),
                                   WebAudioUtils::MaxChannelCount);
  if (aTrack.GetRate() != mSampleRate) {
    resampler = speex_resampler_init(channelCount,
      aTrack.GetRate(), mSampleRate, SPEEX_RESAMPLER_QUALITY_DEFAULT, nullptr);
    speex_resampler_skip_zeros(resampler);
  }

  TrackMapEntry* map = mTrackMap.AppendElement();
  map->mEndOfConsumedInputTicks = 0;
  map->mEndOfLastInputIntervalInInputStream = -1;
  map->mEndOfLastInputIntervalInOutputStream = -1;
  map->mSamplesPassedToResampler =
    TimeToTicksRoundUp(aTrack.GetRate(), GraphTimeToStreamTime(aFrom));
  map->mResampler = resampler;
  map->mResamplerChannelCount = channelCount;
  map->mTrackID = aTrack.GetID();
  return mTrackMap.Length() - 1;
}

static const uint32_t SPEEX_RESAMPLER_PROCESS_MAX_OUTPUT = 1000;

template <typename T> static void
ResampleChannelBuffer(SpeexResamplerState* aResampler, uint32_t aChannel,
                      const T* aInput, uint32_t aInputDuration,
                      nsTArray<float>* aOutput)
{
  if (!aResampler) {
    float* out = aOutput->AppendElements(aInputDuration);
    for (uint32_t i = 0; i < aInputDuration; ++i) {
      out[i] = AudioSampleToFloat(aInput[i]);
    }
    return;
  }

  uint32_t processed = 0;
  while (processed < aInputDuration) {
    uint32_t prevLength = aOutput->Length();
    float* output = aOutput->AppendElements(SPEEX_RESAMPLER_PROCESS_MAX_OUTPUT);
    uint32_t in = aInputDuration - processed;
    uint32_t out = aOutput->Length() - prevLength;
    WebAudioUtils::SpeexResamplerProcess(aResampler, aChannel,
                                         aInput + processed, &in,
                                         output, &out);
    processed += in;
    aOutput->SetLength(prevLength + out);
  }
}

void
AudioNodeExternalInputStream::TrackMapEntry::ResampleChannels(const nsTArray<const void*>& aBuffers,
                                                              uint32_t aInputDuration,
                                                              AudioSampleFormat aFormat,
                                                              float aVolume)
{
  NS_ASSERTION(aBuffers.Length() == mResamplerChannelCount,
               "Channel count must be correct here");

  nsAutoTArray<nsTArray<float>,2> resampledBuffers;
  resampledBuffers.SetLength(aBuffers.Length());
  nsTArray<float> samplesAdjustedForVolume;
  nsAutoTArray<const float*,2> bufferPtrs;
  bufferPtrs.SetLength(aBuffers.Length());

  for (uint32_t i = 0; i < aBuffers.Length(); ++i) {
    AudioSampleFormat format = aFormat;
    const void* buffer = aBuffers[i];

    if (aVolume != 1.0f) {
      format = AUDIO_FORMAT_FLOAT32;
      samplesAdjustedForVolume.SetLength(aInputDuration);
      switch (aFormat) {
      case AUDIO_FORMAT_FLOAT32:
        ConvertAudioSamplesWithScale(static_cast<const float*>(buffer),
                                     samplesAdjustedForVolume.Elements(),
                                     aInputDuration, aVolume);
        break;
      case AUDIO_FORMAT_S16:
        ConvertAudioSamplesWithScale(static_cast<const int16_t*>(buffer),
                                     samplesAdjustedForVolume.Elements(),
                                     aInputDuration, aVolume);
        break;
      default:
        MOZ_ASSERT(false);
        return;
      }
      buffer = samplesAdjustedForVolume.Elements();
    }

    switch (format) {
    case AUDIO_FORMAT_FLOAT32:
      ResampleChannelBuffer(mResampler, i,
                            static_cast<const float*>(buffer),
                            aInputDuration, &resampledBuffers[i]);
      break;
    case AUDIO_FORMAT_S16:
      ResampleChannelBuffer(mResampler, i,
                            static_cast<const int16_t*>(buffer),
                            aInputDuration, &resampledBuffers[i]);
      break;
    default:
      MOZ_ASSERT(false);
      return;
    }
    bufferPtrs[i] = resampledBuffers[i].Elements();
    NS_ASSERTION(i == 0 ||
                 resampledBuffers[i].Length() == resampledBuffers[0].Length(),
                 "Resampler made different decisions for different channels!");
  }

  uint32_t length = resampledBuffers[0].Length();
  nsRefPtr<ThreadSharedObject> buf = new SharedChannelArrayBuffer<float>(&resampledBuffers);
  mResampledData.AppendFrames(buf.forget(), bufferPtrs, length);
}

void
AudioNodeExternalInputStream::TrackMapEntry::ResampleInputData(AudioSegment* aSegment)
{
  AudioSegment::ChunkIterator ci(*aSegment);
  while (!ci.IsEnded()) {
    const AudioChunk& chunk = *ci;
    nsAutoTArray<const void*,2> channels;
    if (chunk.GetDuration() > UINT32_MAX) {
      
      NS_ERROR("Chunk duration out of bounds");
      return;
    }
    uint32_t duration = uint32_t(chunk.GetDuration());

    if (chunk.IsNull()) {
      nsAutoTArray<AudioDataValue,1024> silence;
      silence.SetLength(duration);
      PodZero(silence.Elements(), silence.Length());
      channels.SetLength(mResamplerChannelCount);
      for (uint32_t i = 0; i < channels.Length(); ++i) {
        channels[i] = silence.Elements();
      }
      ResampleChannels(channels, duration, AUDIO_OUTPUT_FORMAT, 0.0f);
    } else if (chunk.mChannelData.Length() == mResamplerChannelCount) {
      
      
      channels.AppendElements(chunk.mChannelData);
      ResampleChannels(channels, duration, chunk.mBufferFormat, chunk.mVolume);
    } else {
      
      
      uint32_t upChannels = GetAudioChannelsSuperset(chunk.mChannelData.Length(), mResamplerChannelCount);
      nsTArray<float> buffer;
      if (chunk.mBufferFormat == AUDIO_FORMAT_FLOAT32) {
        channels.AppendElements(chunk.mChannelData);
      } else {
        NS_ASSERTION(chunk.mBufferFormat == AUDIO_FORMAT_S16, "Unknown format");
        if (duration > UINT32_MAX/chunk.mChannelData.Length()) {
          NS_ERROR("Chunk duration out of bounds");
          return;
        }
        buffer.SetLength(chunk.mChannelData.Length()*duration);
        for (uint32_t i = 0; i < chunk.mChannelData.Length(); ++i) {
          const int16_t* samples = static_cast<const int16_t*>(chunk.mChannelData[i]);
          float* converted = &buffer[i*duration];
          for (uint32_t j = 0; j < duration; ++j) {
            converted[j] = AudioSampleToFloat(samples[j]);
          }
          channels.AppendElement(converted);
        }
      }
      nsTArray<float> zeroes;
      if (channels.Length() < upChannels) {
        zeroes.SetLength(duration);
        PodZero(zeroes.Elements(), zeroes.Length());
        AudioChannelsUpMix(&channels, upChannels, zeroes.Elements());
      }
      if (channels.Length() == mResamplerChannelCount) {
        ResampleChannels(channels, duration, AUDIO_FORMAT_FLOAT32, chunk.mVolume);
      } else {
        nsTArray<float> output;
        if (duration > UINT32_MAX/mResamplerChannelCount) {
          NS_ERROR("Chunk duration out of bounds");
          return;
        }
        output.SetLength(duration*mResamplerChannelCount);
        nsAutoTArray<float*,2> outputPtrs;
        nsAutoTArray<const void*,2> outputPtrsConst;
        for (uint32_t i = 0; i < mResamplerChannelCount; ++i) {
          outputPtrs.AppendElement(output.Elements() + i*duration);
          outputPtrsConst.AppendElement(outputPtrs[i]);
        }
        AudioChannelsDownMix(channels, outputPtrs.Elements(), outputPtrs.Length(), duration);
        ResampleChannels(outputPtrsConst, duration, AUDIO_FORMAT_FLOAT32, chunk.mVolume);
      }
    }
    ci.Next();
  }
}






static void
CopyChunkToBlock(const AudioChunk& aInput, AudioChunk *aBlock, uint32_t aOffsetInBlock)
{
  uint32_t d = aInput.GetDuration();
  for (uint32_t i = 0; i < aBlock->mChannelData.Length(); ++i) {
    float* out = static_cast<float*>(const_cast<void*>(aBlock->mChannelData[i])) +
      aOffsetInBlock;
    if (aInput.IsNull()) {
      PodZero(out, d);
    } else {
      const float* in = static_cast<const float*>(aInput.mChannelData[i]);
      ConvertAudioSamplesWithScale(in, out, d, aInput.mVolume);
    }
  }
}






static void
ConvertSegmentToAudioBlock(AudioSegment* aSegment, AudioChunk* aBlock)
{
  NS_ASSERTION(aSegment->GetDuration() == WEBAUDIO_BLOCK_SIZE, "Bad segment duration");

  {
    AudioSegment::ChunkIterator ci(*aSegment);
    NS_ASSERTION(!ci.IsEnded(), "Segment must have at least one chunk");
    AudioChunk& firstChunk = *ci;
    ci.Next();
    if (ci.IsEnded()) {
      *aBlock = firstChunk;
      return;
    }

    while (ci->IsNull() && !ci.IsEnded()) {
      ci.Next();
    }
    if (ci.IsEnded()) {
      
      aBlock->SetNull(WEBAUDIO_BLOCK_SIZE);
      return;
    }

    AllocateAudioBlock(ci->mChannelData.Length(), aBlock);
  }

  AudioSegment::ChunkIterator ci(*aSegment);
  uint32_t duration = 0;
  while (!ci.IsEnded()) {
    CopyChunkToBlock(*ci, aBlock, duration);
    duration += ci->GetDuration();
    ci.Next();
  }
}

void
AudioNodeExternalInputStream::ProcessInput(GraphTime aFrom, GraphTime aTo,
                                           uint32_t aFlags)
{
  
  mLastChunks.SetLength(1);

  
  
  if (!IsEnabled() || mInputs.IsEmpty() || mPassThrough) {
    mLastChunks[0].SetNull(WEBAUDIO_BLOCK_SIZE);
    AdvanceOutputSegment();
    return;
  }

  MOZ_ASSERT(mInputs.Length() == 1);

  MediaStream* source = mInputs[0]->GetSource();
  nsAutoTArray<AudioSegment,1> audioSegments;
  nsAutoTArray<bool,1> trackMapEntriesUsed;
  uint32_t inputChannels = 0;
  for (StreamBuffer::TrackIter tracks(source->mBuffer, MediaSegment::AUDIO);
       !tracks.IsEnded(); tracks.Next()) {
    const StreamBuffer::Track& inputTrack = *tracks;
    
    size_t trackMapIndex = GetTrackMapEntry(inputTrack, aFrom);
    
    
    
    if (trackMapIndex == nsTArray<TrackMapEntry>::NoIndex) {
      continue;
    }

    while (trackMapEntriesUsed.Length() <= trackMapIndex) {
      trackMapEntriesUsed.AppendElement(false);
    }
    trackMapEntriesUsed[trackMapIndex] = true;

    TrackMapEntry* trackMap = &mTrackMap[trackMapIndex];
    AudioSegment segment;
    GraphTime next;
    TrackRate inputTrackRate = inputTrack.GetRate();
    for (GraphTime t = aFrom; t < aTo; t = next) {
      MediaInputPort::InputInterval interval = mInputs[0]->GetNextInputInterval(t);
      interval.mEnd = std::min(interval.mEnd, aTo);
      if (interval.mStart >= interval.mEnd)
        break;
      next = interval.mEnd;

      
      StreamTime outputEnd = GraphTimeToStreamTime(interval.mEnd);
      TrackTicks startTicks = trackMap->mSamplesPassedToResampler + segment.GetDuration();
      StreamTime outputStart = GraphTimeToStreamTime(interval.mStart);
      NS_ASSERTION(startTicks == TimeToTicksRoundUp(inputTrackRate, outputStart),
                   "Samples missing");
      TrackTicks endTicks = TimeToTicksRoundUp(inputTrackRate, outputEnd);
      TrackTicks ticks = endTicks - startTicks;

      if (interval.mInputIsBlocked) {
        segment.AppendNullData(ticks);
      } else {
        
        StreamTime inputStart = source->GraphTimeToStreamTime(interval.mStart);
        StreamTime inputEnd = source->GraphTimeToStreamTime(interval.mEnd);
        TrackTicks inputTrackEndPoint =
            inputTrack.IsEnded() ? inputTrack.GetEnd() : TRACK_TICKS_MAX;

        if (trackMap->mEndOfLastInputIntervalInInputStream != inputStart ||
            trackMap->mEndOfLastInputIntervalInOutputStream != outputStart) {
          
          trackMap->mEndOfConsumedInputTicks = TimeToTicksRoundDown(inputTrackRate, inputStart) - 1;
        }
        TrackTicks inputStartTicks = trackMap->mEndOfConsumedInputTicks;
        TrackTicks inputEndTicks = inputStartTicks + ticks;
        trackMap->mEndOfConsumedInputTicks = inputEndTicks;
        trackMap->mEndOfLastInputIntervalInInputStream = inputEnd;
        trackMap->mEndOfLastInputIntervalInOutputStream = outputEnd;

        if (inputStartTicks < 0) {
          
          segment.AppendNullData(-inputStartTicks);
          inputStartTicks = 0;
        }
        if (inputEndTicks > inputStartTicks) {
          segment.AppendSlice(*inputTrack.GetSegment(),
                              std::min(inputTrackEndPoint, inputStartTicks),
                              std::min(inputTrackEndPoint, inputEndTicks));
        }
        
        segment.AppendNullData(ticks - segment.GetDuration());
      }
    }

    trackMap->mSamplesPassedToResampler += segment.GetDuration();
    trackMap->ResampleInputData(&segment);

    if (trackMap->mResampledData.GetDuration() < mCurrentOutputPosition + WEBAUDIO_BLOCK_SIZE) {
      
      trackMap->mResampledData.InsertNullDataAtStart(
        mCurrentOutputPosition + WEBAUDIO_BLOCK_SIZE - trackMap->mResampledData.GetDuration());
    }
    audioSegments.AppendElement()->AppendSlice(trackMap->mResampledData,
      mCurrentOutputPosition, mCurrentOutputPosition + WEBAUDIO_BLOCK_SIZE);
    trackMap->mResampledData.ForgetUpTo(mCurrentOutputPosition + WEBAUDIO_BLOCK_SIZE);
    inputChannels = GetAudioChannelsSuperset(inputChannels, trackMap->mResamplerChannelCount);
  }

  for (int32_t i = mTrackMap.Length() - 1; i >= 0; --i) {
    if (i >= int32_t(trackMapEntriesUsed.Length()) || !trackMapEntriesUsed[i]) {
      mTrackMap.RemoveElementAt(i);
    }
  }

  uint32_t accumulateIndex = 0;
  if (inputChannels) {
    nsAutoTArray<float,GUESS_AUDIO_CHANNELS*WEBAUDIO_BLOCK_SIZE> downmixBuffer;
    for (uint32_t i = 0; i < audioSegments.Length(); ++i) {
      AudioChunk tmpChunk;
      ConvertSegmentToAudioBlock(&audioSegments[i], &tmpChunk);
      if (!tmpChunk.IsNull()) {
        if (accumulateIndex == 0) {
          AllocateAudioBlock(inputChannels, &mLastChunks[0]);
        }
        AccumulateInputChunk(accumulateIndex, tmpChunk, &mLastChunks[0], &downmixBuffer);
        accumulateIndex++;
      }
    }
  }
  if (accumulateIndex == 0) {
    mLastChunks[0].SetNull(WEBAUDIO_BLOCK_SIZE);
  }
  mCurrentOutputPosition += WEBAUDIO_BLOCK_SIZE;

  
  AdvanceOutputSegment();
}

bool
AudioNodeExternalInputStream::IsEnabled()
{
  return ((MediaStreamAudioSourceNodeEngine*)Engine())->IsEnabled();
}

}
