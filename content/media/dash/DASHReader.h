














#if !defined(DASHReader_h_)
#define DASHReader_h_

#include "MediaDecoderReader.h"

namespace mozilla {

class DASHReader : public MediaDecoderReader
{
public:
  DASHReader(AbstractMediaDecoder* aDecoder) :
    MediaDecoderReader(aDecoder),
    mReadMetadataMonitor("media.dashreader.readmetadata"),
    mReadyToReadMetadata(false),
    mDecoderIsShuttingDown(false),
    mAudioReader(this),
    mVideoReader(this),
    mAudioReaders(this),
    mVideoReaders(this)
  {
    MOZ_COUNT_CTOR(DASHReader);
  }
  ~DASHReader()
  {
    MOZ_COUNT_DTOR(DASHReader);
  }

  
  
  void AddAudioReader(MediaDecoderReader* aAudioReader);
  void AddVideoReader(MediaDecoderReader* aVideoReader);

  
  
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

  
  
  bool HasAudio() {
    NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");
    return mAudioReader ? mAudioReader->HasAudio() : false;
  }
  bool HasVideo() {
    NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");
    return mVideoReader ? mVideoReader->HasVideo() : false;
  }

  
  
  MediaQueue<AudioData>& AudioQueue();
  MediaQueue<VideoData>& VideoQueue();

  
  nsresult Init(MediaDecoderReader* aCloneDonor);

  
  int64_t VideoQueueMemoryInUse();
  int64_t AudioQueueMemoryInUse();

  
  bool DecodeVideoFrame(bool &aKeyframeSkip, int64_t aTimeThreshold);
  bool DecodeAudioData();

  
  nsresult Seek(int64_t aTime,
                int64_t aStartTime,
                int64_t aEndTime,
                int64_t aCurrentTime);

  
  nsresult GetBuffered(nsTimeRanges* aBuffered, int64_t aStartTime);

  
  VideoData* FindStartTime(int64_t& aOutStartTime);

  
  bool IsSeekableInBufferedRanges();

private:
  
  
  
  
  
  class ReentrantMonitorConditionallyEnter
  {
  public:
    ReentrantMonitorConditionallyEnter(bool aEnter,
                                       ReentrantMonitor &aReentrantMonitor) :
      mReentrantMonitor(nullptr)
    {
      MOZ_COUNT_CTOR(DASHReader::ReentrantMonitorConditionallyEnter);
      if (aEnter) {
        mReentrantMonitor = &aReentrantMonitor;
        NS_ASSERTION(mReentrantMonitor, "null monitor");
        mReentrantMonitor->Enter();
      }
    }
    ~ReentrantMonitorConditionallyEnter(void)
    {
      if (mReentrantMonitor) {
        mReentrantMonitor->Exit();
      }
      MOZ_COUNT_DTOR(DASHReader::ReentrantMonitorConditionallyEnter);
    }
  private:
    
    ReentrantMonitorConditionallyEnter();
    ReentrantMonitorConditionallyEnter(const ReentrantMonitorConditionallyEnter&);
    ReentrantMonitorConditionallyEnter& operator =(const ReentrantMonitorConditionallyEnter&);
    static void* operator new(size_t) CPP_THROW_NEW;
    static void operator delete(void*);

    
    
    ReentrantMonitor* mReentrantMonitor;
  };

  
  
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

    
    
    MonitoredSubReader& operator=(MediaDecoderReader* rhs)
    {
      NS_ASSERTION(mReader->GetDecoder(), "Decoder is null!");
      mReader->GetDecoder()->GetReentrantMonitor().AssertCurrentThreadIn();
      mSubReader = rhs;
      return *this;
    }

    
    
    operator MediaDecoderReader*() const
    {
      NS_ASSERTION(mReader->GetDecoder(), "Decoder is null!");
      if (!mReader->GetDecoder()->OnDecodeThread()) {
        mReader->GetDecoder()->GetReentrantMonitor().AssertCurrentThreadIn();
      }
      return mSubReader;
    }

    
    
    MediaDecoderReader* operator->() const
    {
      return *this;
    }
  private:
    
    DASHReader* mReader;
    
    nsRefPtr<MediaDecoderReader> mSubReader;
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

    
    
    
    nsRefPtr<MediaDecoderReader>& operator[](uint32_t i)
    {
      NS_ASSERTION(mReader->GetDecoder(), "Decoder is null!");
      if (!mReader->GetDecoder()->OnDecodeThread()) {
        mReader->GetDecoder()->GetReentrantMonitor().AssertCurrentThreadIn();
      }
      return mSubReaderList[i];
    }

    
    
    void
    AppendElement(MediaDecoderReader* aReader)
    {
      NS_ASSERTION(mReader->GetDecoder(), "Decoder is null!");
      mReader->GetDecoder()->GetReentrantMonitor().AssertCurrentThreadIn();
      mSubReaderList.AppendElement(aReader);
    }
  private:
    
    DASHReader* mReader;
    
    nsTArray<nsRefPtr<MediaDecoderReader> > mSubReaderList;
  };

  
  
  
  
  MonitoredSubReaderList mAudioReaders;
  MonitoredSubReaderList mVideoReaders;
};

} 

#endif
