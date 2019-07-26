













#if !defined(DASHDecoder_h_)
#define DASHDecoder_h_

#include "nsTArray.h"
#include "nsIURI.h"
#include "nsITimer.h"
#include "nsThreadUtils.h"
#include "MediaDecoder.h"
#include "DASHReader.h"

namespace mozilla {
namespace net {
class IMPDManager;
class nsDASHMPDParser;
class Representation;
}

class DASHRepDecoder;

class DASHDecoder : public MediaDecoder
{
public:
  typedef class mozilla::net::IMPDManager IMPDManager;
  typedef class mozilla::net::nsDASHMPDParser nsDASHMPDParser;
  typedef class mozilla::net::Representation Representation;

  
  static const uint32_t DASH_MAX_MPD_SIZE = 50*1024*1024;

  DASHDecoder();
  ~DASHDecoder();

  
  MediaDecoder* Clone() { return nullptr; }

  
  
  MediaDecoderStateMachine* CreateStateMachine();

  
  
  nsresult Load(MediaResource* aResource,
                nsIStreamListener** aListener,
                MediaDecoder* aCloneDonor);

  
  
  void NotifyDownloadEnded(nsresult aStatus);

  
  
  
  void NotifyDownloadEnded(DASHRepDecoder* aRepDecoder,
                           nsresult aStatus,
                           int32_t const aSubsegmentIdx);

  
  
  void OnReadMetadataCompleted() MOZ_OVERRIDE { }

  
  
  
  void OnReadMetadataCompleted(DASHRepDecoder* aRepDecoder);

  
  
  
  
  bool IsDecoderAllowedToDownloadData(DASHRepDecoder* aRepDecoder);

  
  
  
  
  
  bool IsDecoderAllowedToDownloadSubsegment(DASHRepDecoder* aRepDecoder,
                                            int32_t const aSubsegmentIdx);

  
  
  
  
  nsresult PossiblySwitchDecoder(DASHRepDecoder* aRepDecoder);

  
  
  
  
  void SetSubsegmentIndex(DASHRepDecoder* aRepDecoder,
                          uint32_t aSubsegmentIdx)
  {
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
    if (aRepDecoder == AudioRepDecoder()) {
      mAudioSubsegmentIdx = aSubsegmentIdx;
    } else if (aRepDecoder == VideoRepDecoder()) {
      mVideoSubsegmentIdx = aSubsegmentIdx;
    }
  }
private:
  
  
  
  
  void IncrementSubsegmentIndex(DASHRepDecoder* aRepDecoder)
  {
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
    if (aRepDecoder == AudioRepDecoder()) {
      mAudioSubsegmentIdx++;
    } else if (aRepDecoder == VideoRepDecoder()) {
      mVideoSubsegmentIdx++;
    }
  }
public:
  
  
  
  int32_t GetSubsegmentIndex(DASHRepDecoder* aRepDecoder)
  {
    ReentrantMonitorConditionallyEnter mon(!OnDecodeThread(),
                                           GetReentrantMonitor());
    if (aRepDecoder == AudioRepDecoder()) {
      return mAudioSubsegmentIdx;
    } else if (aRepDecoder == VideoRepDecoder()) {
      return mVideoSubsegmentIdx;
    }
    return (-1);
  }

  
  
  int32_t GetRepIdxForVideoSubsegmentLoad(int32_t aSubsegmentIdx)
  {
    NS_ASSERTION(0 < aSubsegmentIdx, "Subsegment index should not be negative.");
    ReentrantMonitorConditionallyEnter mon(!OnDecodeThread(),
                                           GetReentrantMonitor());
    if ((uint32_t)aSubsegmentIdx < mVideoSubsegmentLoads.Length()) {
      return mVideoSubsegmentLoads[aSubsegmentIdx];
    } else {
      
      return 0;
    }
  }

  
  
  void ReleaseStateMachine();

  
  
  void Shutdown();

  
  
  void LoadAborted();

  
  
  
  void DecodeError();

private:
  
  
  void ReadMPDBuffer();

  
  
  void OnReadMPDBufferCompleted();

  
  
  nsresult ParseMPDBuffer();

  
  
  nsresult CreateRepDecoders();

  
  
  nsresult CreateAudioRepDecoder(nsIURI* aUrl, Representation const * aRep);
  nsresult CreateVideoRepDecoder(nsIURI* aUrl, Representation const * aRep);

  
  
  
  
  
  DASHRepDecoder* AudioRepDecoder() {
    ReentrantMonitorConditionallyEnter mon(!OnDecodeThread(),
                                           GetReentrantMonitor());
    NS_ENSURE_TRUE((uint32_t)mAudioRepDecoderIdx < mAudioRepDecoders.Length(),
                   nullptr);
    if (mAudioRepDecoderIdx < 0) {
      return nullptr;
    } else {
      return mAudioRepDecoders[mAudioRepDecoderIdx];
    }
  }

  
  
  
  DASHRepDecoder* VideoRepDecoder() {
    ReentrantMonitorConditionallyEnter mon(!OnDecodeThread(),
                                           GetReentrantMonitor());
    NS_ENSURE_TRUE((uint32_t)mVideoRepDecoderIdx < mVideoRepDecoders.Length(),
                   nullptr);
    if (mVideoRepDecoderIdx < 0) {
      return nullptr;
    } else {
      return mVideoRepDecoders[mVideoRepDecoderIdx];
    }
  }

  
  
  MediaResource* CreateAudioSubResource(nsIURI* aUrl,
                                        MediaDecoder* aAudioDecoder);
  MediaResource* CreateVideoSubResource(nsIURI* aUrl,
                                        MediaDecoder* aVideoDecoder);

  
  
  nsresult CreateSubChannel(nsIURI* aUrl, nsIChannel** aChannel);

  
  
  nsresult LoadRepresentations();

  
  bool mNotifiedLoadAborted;

  
  nsAutoArrayPtr<char>         mBuffer;
  
  uint32_t                     mBufferLength;
  
  nsCOMPtr<nsIThread>          mMPDReaderThread;
  
  nsCOMPtr<nsIPrincipal>       mPrincipal;

  
  nsAutoPtr<IMPDManager>       mMPDManager;

  
  
  DASHReader* mDASHReader;

  
  
  

  
  int32_t mVideoAdaptSetIdx;

  
  int32_t mAudioRepDecoderIdx;
  int32_t mVideoRepDecoderIdx;

  
  
  nsTArray<nsRefPtr<DASHRepDecoder> > mAudioRepDecoders;
  nsTArray<nsRefPtr<DASHRepDecoder> > mVideoRepDecoders;

  
  int32_t mAudioSubsegmentIdx;
  int32_t mVideoSubsegmentIdx;

  
  
  
  
  
  uint32_t mAudioMetadataReadCount;
  uint32_t mVideoMetadataReadCount;

  
  
  nsTArray<int32_t> mVideoSubsegmentLoads;
};

} 

#endif
