





#ifndef AudioNode_h_
#define AudioNode_h_

#include "nsDOMEventTargetHelper.h"
#include "mozilla/dom/AudioNodeBinding.h"
#include "nsCycleCollectionParticipant.h"
#include "EnableWebAudioCheck.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "AudioContext.h"
#include "MediaStreamGraph.h"
#include "WebAudioUtils.h"

namespace mozilla {

namespace dom {

class AudioContext;
class AudioBufferSourceNode;
class AudioParam;
class AudioParamTimeline;
struct ThreeDPoint;

template<class T>
class SelfReference {
public:
  SelfReference() : mHeld(false) {}
  ~SelfReference()
  {
    NS_ASSERTION(!mHeld, "Forgot to drop the self reference?");
  }

  void Take(T* t)
  {
    if (!mHeld) {
      mHeld = true;
      t->AddRef();
    }
  }
  void Drop(T* t)
  {
    if (mHeld) {
      mHeld = false;
      t->Release();
    }
  }

  operator bool() const { return mHeld; }

private:
  bool mHeld;
};


























class AudioNode : public nsDOMEventTargetHelper,
                  public EnableWebAudioCheck
{
protected:
  
  virtual ~AudioNode();

public:
  AudioNode(AudioContext* aContext,
            uint32_t aChannelCount,
            ChannelCountMode aChannelCountMode,
            ChannelInterpretation aChannelInterpretation);

  
  virtual void DestroyMediaStream();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(AudioNode,
                                           nsDOMEventTargetHelper)

  virtual AudioBufferSourceNode* AsAudioBufferSourceNode() {
    return nullptr;
  }

  virtual const DelayNode* AsDelayNode() const {
    return nullptr;
  }

  AudioContext* GetParentObject() const
  {
    return mContext;
  }

  AudioContext* Context() const
  {
    return mContext;
  }

  virtual void Connect(AudioNode& aDestination, uint32_t aOutput,
                       uint32_t aInput, ErrorResult& aRv);

  virtual void Connect(AudioParam& aDestination, uint32_t aOutput,
                       ErrorResult& aRv);

  virtual void Disconnect(uint32_t aOutput, ErrorResult& aRv);

  
  
  
  virtual uint16_t NumberOfInputs() const { return 1; }
  virtual uint16_t NumberOfOutputs() const { return 1; }

  uint32_t ChannelCount() const { return mChannelCount; }
  virtual void SetChannelCount(uint32_t aChannelCount, ErrorResult& aRv)
  {
    if (aChannelCount == 0 ||
        aChannelCount > WebAudioUtils::MaxChannelCount) {
      aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
      return;
    }
    mChannelCount = aChannelCount;
    SendChannelMixingParametersToStream();
  }
  ChannelCountMode ChannelCountModeValue() const
  {
    return mChannelCountMode;
  }
  virtual void SetChannelCountModeValue(ChannelCountMode aMode, ErrorResult& aRv)
  {
    mChannelCountMode = aMode;
    SendChannelMixingParametersToStream();
  }
  ChannelInterpretation ChannelInterpretationValue() const
  {
    return mChannelInterpretation;
  }
  void SetChannelInterpretationValue(ChannelInterpretation aMode)
  {
    mChannelInterpretation = aMode;
    SendChannelMixingParametersToStream();
  }

  struct InputNode {
    ~InputNode()
    {
      if (mStreamPort) {
        mStreamPort->Destroy();
      }
    }

    
    AudioNode* mInputNode;
    nsRefPtr<MediaInputPort> mStreamPort;
    
    
    uint32_t mInputPort;
    
    uint32_t mOutputPort;
  };

  MediaStream* Stream() { return mStream; }

  const nsTArray<InputNode>& InputNodes() const
  {
    return mInputNodes;
  }
  const nsTArray<nsRefPtr<AudioNode> >& OutputNodes() const
  {
    return mOutputNodes;
  }
  const nsTArray<nsRefPtr<AudioParam> >& OutputParams() const
  {
    return mOutputParams;
  }

  void RemoveOutputParam(AudioParam* aParam);

  virtual void NotifyInputConnected() {}

  
  
  
  void MarkActive() { Context()->RegisterActiveNode(this); }
  
  
  
  
  
  void MarkInactive() { Context()->UnregisterActiveNode(this); }

private:
  friend class AudioBufferSourceNode;
  
  void DisconnectFromGraph();

protected:
  static void Callback(AudioNode* aNode) {  }

  
  void SendDoubleParameterToStream(uint32_t aIndex, double aValue);
  void SendInt32ParameterToStream(uint32_t aIndex, int32_t aValue);
  void SendThreeDPointParameterToStream(uint32_t aIndex, const ThreeDPoint& aValue);
  void SendChannelMixingParametersToStream();
  static void SendTimelineParameterToStream(AudioNode* aNode, uint32_t aIndex,
                                            const AudioParamTimeline& aValue);

private:
  nsRefPtr<AudioContext> mContext;

protected:
  
  
  nsRefPtr<MediaStream> mStream;

private:
  
  
  nsTArray<InputNode> mInputNodes;
  
  
  
  
  nsTArray<nsRefPtr<AudioNode> > mOutputNodes;
  
  
  
  
  
  nsTArray<nsRefPtr<AudioParam> > mOutputParams;
  uint32_t mChannelCount;
  ChannelCountMode mChannelCountMode;
  ChannelInterpretation mChannelInterpretation;
};

}
}

#endif
