





#ifndef ConvolverNode_h_
#define ConvolverNode_h_

#include "AudioNode.h"
#include "AudioBuffer.h"

namespace mozilla {
namespace dom {

class ConvolverNode final : public AudioNode
{
public:
  explicit ConvolverNode(AudioContext* aContext);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ConvolverNode, AudioNode);

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

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

  virtual void SetChannelCount(uint32_t aChannelCount, ErrorResult& aRv) override
  {
    if (aChannelCount > 2) {
      aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
      return;
    }
    AudioNode::SetChannelCount(aChannelCount, aRv);
  }
  virtual void SetChannelCountModeValue(ChannelCountMode aMode, ErrorResult& aRv) override
  {
    if (aMode == ChannelCountMode::Max) {
      aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
      return;
    }
    AudioNode::SetChannelCountModeValue(aMode, aRv);
  }

  virtual const char* NodeType() const override
  {
    return "ConvolverNode";
  }

  virtual size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const override;
  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const override;

protected:
  virtual ~ConvolverNode();

private:
  nsRefPtr<AudioBuffer> mBuffer;
  bool mNormalize;
};


} 
} 

#endif

