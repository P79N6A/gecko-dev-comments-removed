














#if !defined(DASHReader_h_)
#define DASHReader_h_

#include "VideoUtils.h"
#include "MediaDecoderReader.h"
#include "DASHRepReader.h"

namespace mozilla {

class DASHRepReader;

class DASHReader : public MediaDecoderReader
{
public:
  DASHReader(AbstractMediaDecoder* aDecoder);
  ~DASHReader();

  
  
  void AddAudioReader(DASHRepReader* aAudioReader);
  void AddVideoReader(DASHRepReader* aVideoReader);

  
  
  nsresult ReadMetadata(VideoInfo* aInfo,
                        MetadataTags** aTags);

  
  
  
  nsresult WaitForMetadata() {
    NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");
    ReentrantMonitorAutoEnter mon(mReadMetadataMonitor);
    while (true) {
      
      if (mDecoderIsShuttingDown) {
        return NS_ERROR_ABORT;
      } else if (mReadyToReadMetadata) {
        break;
      }
      mon.Wait();
    }
    return NS_OK;
  }

  
  
  void ReadyToReadMetadata() {
    NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
    ReentrantMonitorAutoEnter mon(mReadMetadataMonitor);
    mReadyToReadMetadata = true;
    mon.NotifyAll();
  }

  
  
  void NotifyDecoderShuttingDown() {
    NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
    ReentrantMonitorAutoEnter metadataMon(mReadMetadataMonitor);
    mDecoderIsShuttingDown = true;
    
    metadataMon.NotifyAll();
  }

  
  
  bool HasAudio();
  bool HasVideo();

  
  
  MediaQueue<AudioData>& AudioQueue() MOZ_OVERRIDE;
  MediaQueue<VideoData>& VideoQueue() MOZ_OVERRIDE;

  
  nsresult Init(MediaDecoderReader* aCloneDonor);

  
  int64_t VideoQueueMemoryInUse();
  int64_t AudioQueueMemoryInUse();

  
  
  
  void PrepareToDecode() MOZ_OVERRIDE;

  
  bool DecodeVideoFrame(bool &aKeyframeSkip, int64_t aTimeThreshold);
  bool DecodeAudioData();

  
  nsresult Seek(int64_t aTime,
                int64_t aStartTime,
                int64_t aEndTime,
                int64_t aCurrentTime);

  
  nsresult GetBuffered(nsTimeRanges* aBuffered, int64_t aStartTime);

  
  VideoData* FindStartTime(int64_t& aOutStartTime);

  
  
  
  
  
  void RequestVideoReaderSwitch(uint32_t aFromReaderIdx,
                                uint32_t aToReaderIdx,
                                uint32_t aSubsegmentIdx);

private:
  
  
  
  void PossiblySwitchVideoReaders();

  
  
  ReentrantMonitor mReadMetadataMonitor;
  bool mReadyToReadMetadata;
  bool mDecoderIsShuttingDown;

  
  
  
  
  class MonitoredSubReader
  {
  public:
    
    
    MonitoredSubReader(DASHReader* aReader) :
      mReader(aReader),
      mSubReader(nullptr)
    {
      MOZ_COUNT_CTOR(DASHReader::MonitoredSubReader);
      NS_ASSERTION(mReader, "Reader is null!");
    }
    
    ~MonitoredSubReader()
    {
      MOZ_COUNT_DTOR(DASHReader::MonitoredSubReader);
    }

    
    
    MonitoredSubReader& operator=(DASHRepReader* rhs)
    {
      NS_ASSERTION(mReader->GetDecoder(), "Decoder is null!");
      mReader->GetDecoder()->GetReentrantMonitor().AssertCurrentThreadIn();
      mSubReader = rhs;
      return *this;
    }

    
    
    operator DASHRepReader*() const
    {
      NS_ASSERTION(mReader->GetDecoder(), "Decoder is null!");
      if (!mReader->GetDecoder()->OnDecodeThread()) {
        mReader->GetDecoder()->GetReentrantMonitor().AssertCurrentThreadIn();
      }
      return mSubReader;
    }

    
    
    DASHRepReader* operator->() const
    {
      return *this;
    }
  private:
    
    DASHReader* mReader;
    
    nsRefPtr<DASHRepReader> mSubReader;
  };

  
  
  
  
  
  MonitoredSubReader mAudioReader;
  MonitoredSubReader mVideoReader;

  
  
  
  
  
  
  class MonitoredSubReaderList
  {
  public:
    
    
    MonitoredSubReaderList(DASHReader* aReader) :
      mReader(aReader)
    {
      MOZ_COUNT_CTOR(DASHReader::MonitoredSubReaderList);
      NS_ASSERTION(mReader, "Reader is null!");
    }
    
    
    ~MonitoredSubReaderList()
    {
      MOZ_COUNT_DTOR(DASHReader::MonitoredSubReaderList);
    }

    
    
    uint32_t Length() const
    {
      NS_ASSERTION(mReader->GetDecoder(), "Decoder is null!");
      if (!mReader->GetDecoder()->OnDecodeThread()) {
        mReader->GetDecoder()->GetReentrantMonitor().AssertCurrentThreadIn();
      }
      return mSubReaderList.Length();
    }

    
    
    
    nsRefPtr<DASHRepReader>& operator[](uint32_t i)
    {
      NS_ASSERTION(mReader->GetDecoder(), "Decoder is null!");
      if (!mReader->GetDecoder()->OnDecodeThread()) {
        mReader->GetDecoder()->GetReentrantMonitor().AssertCurrentThreadIn();
      }
      return mSubReaderList[i];
    }

    
    
    void
    AppendElement(DASHRepReader* aReader)
    {
      NS_ASSERTION(mReader->GetDecoder(), "Decoder is null!");
      mReader->GetDecoder()->GetReentrantMonitor().AssertCurrentThreadIn();
      mSubReaderList.AppendElement(aReader);
    }
  private:
    
    DASHReader* mReader;
    
    nsTArray<nsRefPtr<DASHRepReader> > mSubReaderList;
  };

  
  
  
  
  MonitoredSubReaderList mAudioReaders;
  MonitoredSubReaderList mVideoReaders;

  
  
  bool mSwitchVideoReaders;

  
  
  nsTArray<uint32_t> mSwitchToVideoSubsegmentIndexes;

  
  
  int32_t mSwitchCount;
};

} 

#endif
