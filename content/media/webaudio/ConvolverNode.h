





#ifndef ConvolverNode_h_
#define ConvolverNode_h_

#include "AudioNode.h"
#include "AudioBuffer.h"
#include "PlayingRefChangeHandler.h"

namespace mozilla {
namespace dom {

class ConvolverNode : public AudioNode
{
public:
  explicit ConvolverNode(AudioContext* aContext);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ConvolverNode, AudioNode);

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  AudioBuffer* GetBuffer(JSContext* aCx) const
  {
    return mBuffer;
  }

  void SetBuffer(JSContext* aCx, AudioBuffer* aBufferi, ErrorResult& aRv);

  bool Normalize() const
  {
    return mNormalize;
  }

  void SetNormalize(bool aNormal);

  virtual void NotifyInputConnected() MOZ_OVERRIDE
  {
    mMediaStreamGraphUpdateIndexAtLastInputConnection =
      mStream->Graph()->GetCurrentGraphUpdateIndex();
  }
  bool AcceptPlayingRefRelease(int64_t aLastGraphUpdateIndexProcessed) const
  {
    
    
    
    return aLastGraphUpdateIndexProcessed >= mMediaStreamGraphUpdateIndexAtLastInputConnection;
  }

private:
  friend class PlayingRefChangeHandler<ConvolverNode>;

  int64_t mMediaStreamGraphUpdateIndexAtLastInputConnection;
  nsRefPtr<AudioBuffer> mBuffer;
  SelfReference<ConvolverNode> mPlayingRef;
  bool mNormalize;
};


} 
} 

#endif

