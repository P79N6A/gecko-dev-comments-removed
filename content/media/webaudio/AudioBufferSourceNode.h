





#ifndef AudioBufferSourceNode_h_
#define AudioBufferSourceNode_h_

#include "AudioSourceNode.h"
#include "AudioBuffer.h"

namespace mozilla {
namespace dom {

class AudioBufferSourceNode : public AudioSourceNode
{
public:
  explicit AudioBufferSourceNode(AudioContext* aContext);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(AudioBufferSourceNode, AudioSourceNode)

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope);

  void Start(double) {  }
  void Stop(double) {  }

  AudioBuffer* GetBuffer() const
  {
    return mBuffer;
  }
  void SetBuffer(AudioBuffer* aBuffer);

private:
  nsRefPtr<AudioBuffer> mBuffer;
};

}
}

#endif

