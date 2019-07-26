













#if !defined(nsDASHRepDecoder_h_)
#define nsDASHRepDecoder_h_

#include "Representation.h"
#include "ImageLayers.h"
#include "nsDASHDecoder.h"
#include "nsWebMDecoder.h"
#include "nsWebMReader.h"
#include "nsBuiltinDecoder.h"

class nsDASHDecoder;

class nsDASHRepDecoder : public nsBuiltinDecoder
{
public:
  typedef mozilla::net::Representation Representation;
  typedef mozilla::net::SegmentBase SegmentBase;
  typedef mozilla::layers::ImageContainer ImageContainer;

  
  nsDASHRepDecoder(nsDASHDecoder* aMainDecoder) :
    mMainDecoder(aMainDecoder),
    mMPDRepresentation(nullptr),
    mMetadataChunkCount(0),
    mCurrentByteRange(),
    mSubsegmentIdx(0),
    mReader(nullptr)
  {
    MOZ_COUNT_CTOR(nsDASHRepDecoder);
  }

  ~nsDASHRepDecoder()
  {
    MOZ_COUNT_DTOR(nsDASHRepDecoder);
  }

  
  virtual nsMediaDecoder* Clone() { return nullptr; }

  
  
  nsresult SetStateMachine(nsDecoderStateMachine* aSM);

private:
  
  
  nsDecoderStateMachine* CreateStateMachine();

public:
  
  
  void SetResource(MediaResource* aResource);

  
  
  void SetMPDRepresentation(Representation const * aRep);

  
  nsresult Load(MediaResource* aResource = nullptr,
                nsIStreamListener** aListener = nullptr,
                nsMediaDecoder* aCloneDonor = nullptr);

  
  
  void LoadNextByteRange();

  
  void SetReader(nsWebMReader* aReader);

  
  
  void NetworkError();

  
  
  
  virtual void SetDuration(double aDuration);

  
  void SetInfinite(bool aInfinite);

  
  void SetSeekable(bool aSeekable);

  
  
  
  
  void Progress(bool aTimer);

  
  
  void NotifyDataArrived(const char* aBuffer,
                         uint32_t aLength,
                         int64_t aOffset);

  
  
  void NotifyBytesDownloaded();

  
  
  void NotifyDownloadEnded(nsresult aStatus);

  
  
  
  
  void NotifySuspendedStatusChanged();

  
  nsresult GetByteRangeForSeek(int64_t const aOffset,
                               MediaByteRange& aByteRange);

  
  bool OnStateMachineThread() const;

  
  bool OnDecodeThread() const;

  
  ReentrantMonitor& GetReentrantMonitor();

  
  
  nsDecoderStateMachine::State GetDecodeState();

  
  ImageContainer* GetImageContainer();

  
  
  void OnReadMetadataCompleted();

  
  void Shutdown() {
    NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
    
    nsBuiltinDecoder::Shutdown();
    NS_ENSURE_TRUE(mShuttingDown, );
    
    mMainDecoder = nullptr;
  }

  
  
  void ReleaseStateMachine();

  
  void DecodeError();

private:
  
  nsRefPtr<nsDASHDecoder> mMainDecoder;
  
  Representation const * mMPDRepresentation;

  
  uint16_t        mMetadataChunkCount;

  
  nsTArray<MediaByteRange> mByteRanges;

  
  MediaByteRange  mInitByteRange;
  MediaByteRange  mIndexByteRange;

  
  MediaByteRange  mCurrentByteRange;
  
  uint64_t        mSubsegmentIdx;

  
  
  nsBuiltinDecoderReader*   mReader;
};

#endif 
