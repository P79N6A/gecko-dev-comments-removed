













#if !defined(nsDASHDecoder_h_)
#define nsDASHDecoder_h_

#include "nsTArray.h"
#include "nsIURI.h"
#include "nsITimer.h"
#include "nsThreadUtils.h"
#include "nsBuiltinDecoder.h"
#include "nsDASHReader.h"

class nsDASHRepDecoder;

namespace mozilla {
namespace net {
class IMPDManager;
class nsDASHMPDParser;
class Representation;
}
}

class nsDASHDecoder : public nsBuiltinDecoder
{
public:
  typedef class mozilla::net::IMPDManager IMPDManager;
  typedef class mozilla::net::nsDASHMPDParser nsDASHMPDParser;
  typedef class mozilla::net::Representation Representation;

  
  static const uint32_t DASH_MAX_MPD_SIZE = 50*1024*1024;

  nsDASHDecoder();
  ~nsDASHDecoder();

  
  nsMediaDecoder* Clone() { return nullptr; }

  
  
  nsDecoderStateMachine* CreateStateMachine();

  
  
  nsresult Load(MediaResource* aResource,
                nsIStreamListener** aListener,
                nsMediaDecoder* aCloneDonor);

  
  
  void NotifyDownloadEnded(nsresult aStatus);

  
  
  
  void NotifyDownloadEnded(nsDASHRepDecoder* aRepDecoder,
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
                                        nsMediaDecoder* aAudioDecoder);
  MediaResource* CreateVideoSubResource(nsIURI* aUrl,
                                        nsMediaDecoder* aVideoDecoder);

  
  
  nsresult CreateSubChannel(nsIURI* aUrl, nsIChannel** aChannel);

  
  
  nsresult LoadRepresentations();

  
  bool mNotifiedLoadAborted;

  
  nsAutoArrayPtr<char>         mBuffer;
  
  uint32_t                     mBufferLength;
  
  nsCOMPtr<nsIThread>          mMPDReaderThread;
  
  nsCOMPtr<nsIPrincipal>       mPrincipal;

  
  nsAutoPtr<IMPDManager>       mMPDManager;

  
  
  nsDASHReader* mDASHReader;

  
  nsRefPtr<nsDASHRepDecoder> mAudioRepDecoder;
  
  nsTArray<nsRefPtr<nsDASHRepDecoder> > mAudioRepDecoders;

  
  nsRefPtr<nsDASHRepDecoder> mVideoRepDecoder;
  
  nsTArray<nsRefPtr<nsDASHRepDecoder> > mVideoRepDecoders;
};

#endif
