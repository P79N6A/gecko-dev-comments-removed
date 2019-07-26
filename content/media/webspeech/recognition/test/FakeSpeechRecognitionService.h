





#pragma once

#include "nsCOMPtr.h"
#include "nsIObserver.h"


namespace mozilla {
  class AudioSegment;

  namespace dom {
    class SpeechRecognition;
    class SpeechRecognitionResultList;
  }
}

#include "nsISpeechRecognitionService.h"

#define NS_FAKE_SPEECH_RECOGNITION_SERVICE_CID \
  {0x48c345e7, 0x9929, 0x4f9a, {0xa5, 0x63, 0xf4, 0x78, 0x22, 0x2d, 0xab, 0xcd}};

namespace mozilla {

class FakeSpeechRecognitionService : public nsISpeechRecognitionService,
                                     public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISPEECHRECOGNITIONSERVICE
  NS_DECL_NSIOBSERVER

  FakeSpeechRecognitionService();

private:
  virtual ~FakeSpeechRecognitionService();

  WeakPtr<dom::SpeechRecognition> mRecognition;
  dom::SpeechRecognitionResultList* BuildMockResultList();
};

} 
