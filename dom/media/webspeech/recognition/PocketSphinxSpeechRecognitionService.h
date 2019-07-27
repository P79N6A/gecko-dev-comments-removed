





#ifndef mozilla_dom_PocketSphinxRecognitionService_h
#define mozilla_dom_PocketSphinxRecognitionService_h

#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsIObserver.h"
#include "nsISpeechRecognitionService.h"
#include "speex/speex_resampler.h"

extern "C" {
#include <pocketsphinx/pocketsphinx.h>
#include <sphinxbase/sphinx_config.h>
}

#define NS_POCKETSPHINX_SPEECH_RECOGNITION_SERVICE_CID                         \
  {                                                                            \
    0x0ff5ce56, 0x5b09, 0x4db8, {                                              \
      0xad, 0xc6, 0x82, 0x66, 0xaf, 0x95, 0xf8, 0x64                           \
    }                                                                          \
  };

namespace mozilla {




class PocketSphinxSpeechRecognitionService : public nsISpeechRecognitionService,
                                             public nsIObserver
{
public:
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSISPEECHRECOGNITIONSERVICE

  
  NS_DECL_NSIOBSERVER

  



  PocketSphinxSpeechRecognitionService();

private:
  


  virtual ~PocketSphinxSpeechRecognitionService();

  
  WeakPtr<dom::SpeechRecognition> mRecognition;

  


  dom::SpeechRecognitionResultList* BuildMockResultList();

  
  SpeexResamplerState* mSpeexState;

  
  ps_decoder_t* mPSHandle;

  
  cmd_ln_t* mPSConfig;

  
  bool ISDecoderCreated;

  
  bool ISGrammarCompiled;

  
  nsTArray<int16_t> mAudioVector;
};

} 

#endif
