





#ifndef AudioNode_h_
#define AudioNode_h_

#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/AudioNodeBinding.h"
#include "nsCycleCollectionParticipant.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "AudioContext.h"
#include "MediaStreamGraph.h"
#include "WebAudioUtils.h"
#include "mozilla/MemoryReporting.h"
#include "nsWeakReference.h"
#include "SelfRef.h"

namespace mozilla {

namespace dom {

class AudioContext;
class AudioBufferSourceNode;
class AudioParam;
class AudioParamTimeline;
struct ThreeDPoint;


























class AudioNode : public DOMEventTargetHelper,
                  public nsSupportsWeakReference
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
                                           DOMEventTargetHelper)

  virtual AudioBufferSourceNode* AsAudioBufferSourceNode()
  {
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

  uint32_t Id() const { return mId; }

  bool PassThrough() const;
  void SetPassThrough(bool aPassThrough);

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

  struct InputNode final
  {
    ~InputNode()
    {
      if (mStreamPort) {
        mStreamPort->Destroy();
      }
    }

    size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
    {
      size_t amount = 0;
      if (mStreamPort) {
        amount += mStreamPort->SizeOfIncludingThis(aMallocSizeOf);
      }

      return amount;
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

  
  
  
  void MarkActive() { Context()->RegisterActiveNode(this); }
  
  
  
  
  
  void MarkInactive() { Context()->UnregisterActiveNode(this); }

  virtual size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const;
  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const;

  virtual const char* NodeType() const = 0;

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
  const uint32_t mId;
  
  
  bool mPassThrough;
#ifdef DEBUG
  
  
  bool mDemiseNotified;
#endif
};

}
}

#endif
