













#if !defined(DASHRepDecoder_h_)
#define DASHRepDecoder_h_

#include "Representation.h"
#include "ImageLayers.h"
#include "DASHDecoder.h"
#include "WebMDecoder.h"
#include "WebMReader.h"
#include "MediaDecoder.h"

namespace mozilla {

class DASHDecoder;
class DASHRepReader;

class DASHRepDecoder : public MediaDecoder
{
public:
  typedef mozilla::net::Representation Representation;
  typedef mozilla::net::SegmentBase SegmentBase;
  typedef mozilla::layers::ImageContainer ImageContainer;

  
  DASHRepDecoder(DASHDecoder* aMainDecoder) :
    mMainDecoder(aMainDecoder),
    mMPDRepresentation(nullptr),
    mMetadataChunkCount(0),
    mCurrentByteRange(),
    mSubsegmentIdx(-1),
    mReader(nullptr)
  {
    MOZ_COUNT_CTOR(DASHRepDecoder);
  }

  ~DASHRepDecoder()
  {
    MOZ_COUNT_DTOR(DASHRepDecoder);
  }

  
  virtual MediaDecoder* Clone() { return nullptr; }

  
  
  nsresult SetStateMachine(MediaDecoderStateMachine* aSM);

private:
  
  
  MediaDecoderStateMachine* CreateStateMachine();

public:
  
  
  void SetResource(MediaResource* aResource);

  
  
  void SetMPDRepresentation(Representation const * aRep);

  
  nsresult Load(MediaResource* aResource = nullptr,
                nsIStreamListener** aListener = nullptr,
                MediaDecoder* aCloneDonor = nullptr);

  
  
  void LoadNextByteRange();

  
  void SetReader(WebMReader* aReader);

  
  
  void NetworkError();

  
  
  
  virtual void SetDuration(double aDuration);

  
  void SetInfinite(bool aInfinite);

  
  void SetMediaSeekable(bool aSeekable);

  
  
  
  
  void Progress(bool aTimer);

  
  
  void NotifyDataArrived(const char* aBuffer,
                         uint32_t aLength,
                         int64_t aOffset);

  
  
  void NotifyBytesDownloaded();

  
  
  void NotifyDownloadEnded(nsresult aStatus);

  
  
  
  
  void NotifySuspendedStatusChanged();

  
  nsresult GetByteRangeForSeek(int64_t const aOffset,
                               MediaByteRange& aByteRange);

  
  uint32_t GetNumDataByteRanges() {
    return mByteRanges.Length();
  }

  
  void PrepareForSwitch();

  
  bool OnStateMachineThread() const;

  
  bool OnDecodeThread() const;

  
  ReentrantMonitor& GetReentrantMonitor() MOZ_OVERRIDE;

  
  ImageContainer* GetImageContainer();

  
  
  void OnReadMetadataCompleted() MOZ_OVERRIDE;

  
  void Shutdown() {
    NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
    
    MediaDecoder::Shutdown();
    NS_ENSURE_TRUE(mShuttingDown, );
    
    mMainDecoder = nullptr;
  }

  
  
  void ReleaseStateMachine();

  
  void DecodeError();

private:
  
  
  nsresult PopulateByteRanges();

  
  nsRefPtr<DASHDecoder> mMainDecoder;
  
  Representation const * mMPDRepresentation;

  
  uint16_t        mMetadataChunkCount;

  
  nsTArray<MediaByteRange> mByteRanges;

  
  MediaByteRange  mInitByteRange;
  MediaByteRange  mIndexByteRange;

  
  MediaByteRange  mCurrentByteRange;
  
  int32_t         mSubsegmentIdx;

  
  
  DASHRepReader* mReader;
};

} 

#endif 
