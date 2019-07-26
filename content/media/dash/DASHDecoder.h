













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
                           MediaByteRange &aRange);

  
  
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

  
  nsRefPtr<DASHRepDecoder> mAudioRepDecoder;
  
  nsTArray<nsRefPtr<DASHRepDecoder> > mAudioRepDecoders;

  
  nsRefPtr<DASHRepDecoder> mVideoRepDecoder;
  
  nsTArray<nsRefPtr<DASHRepDecoder> > mVideoRepDecoders;
};

} 

#endif
