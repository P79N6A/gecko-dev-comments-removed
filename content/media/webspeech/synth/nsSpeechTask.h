





#pragma once

#include "MediaStreamGraph.h"
#include "SpeechSynthesisUtterance.h"
#include "nsISpeechService.h"

namespace mozilla {
namespace dom {

class SpeechSynthesisUtterance;
class SpeechSynthesis;
class SynthStreamListener;

class nsSpeechTask : public nsISpeechTask
{
  friend class SynthStreamListener;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsSpeechTask, nsISpeechTask)

  NS_DECL_NSISPEECHTASK

  nsSpeechTask(SpeechSynthesisUtterance* aUtterance);
  nsSpeechTask(float aVolume, const nsAString& aText);

  virtual ~nsSpeechTask();

  virtual void Pause();

  virtual void Resume();

  virtual void Cancel();

  float GetCurrentTime();

  uint32_t GetCurrentCharOffset();

  void SetSpeechSynthesis(SpeechSynthesis* aSpeechSynthesis);

  void SetIndirectAudio(bool aIndirectAudio) { mIndirectAudio = aIndirectAudio; }

protected:
  virtual nsresult DispatchStartImpl();

  virtual nsresult DispatchEndImpl(float aElapsedTime, uint32_t aCharIndex);

  virtual nsresult DispatchPauseImpl(float aElapsedTime, uint32_t aCharIndex);

  virtual nsresult DispatchResumeImpl(float aElapsedTime, uint32_t aCharIndex);

  virtual nsresult DispatchErrorImpl(float aElapsedTime, uint32_t aCharIndex);

  virtual nsresult DispatchBoundaryImpl(const nsAString& aName,
                                        float aElapsedTime,
                                        uint32_t aCharIndex);

  virtual nsresult DispatchMarkImpl(const nsAString& aName,
                                    float aElapsedTime, uint32_t aCharIndex);

  nsRefPtr<SpeechSynthesisUtterance> mUtterance;

  float mVolume;

  nsString mText;

private:
  void End();

  nsRefPtr<SourceMediaStream> mStream;

  nsCOMPtr<nsISpeechTaskCallback> mCallback;

  uint32_t mChannels;

  nsRefPtr<SpeechSynthesis> mSpeechSynthesis;

  bool mIndirectAudio;
};

} 
} 
