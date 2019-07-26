





#ifndef AudioNode_h_
#define AudioNode_h_

#include "nsDOMEventTargetHelper.h"
#include "mozilla/dom/AudioNodeBinding.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"
#include "EnableWebAudioCheck.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "AudioContext.h"
#include "AudioParamTimeline.h"
#include "MediaStreamGraph.h"
#include "WebAudioUtils.h"

struct JSContext;

namespace mozilla {

class ErrorResult;

namespace dom {

class AudioParam;
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

template<class T>
class SelfCountedReference {
public:
  SelfCountedReference() : mRefCnt(0) {}
  ~SelfCountedReference()
  {
    NS_ASSERTION(mRefCnt == 0, "Forgot to drop the self reference?");
  }

  void Take(T* t)
  {
    if (mRefCnt++ == 0) {
      t->AddRef();
    }
  }
  void Drop(T* t)
  {
    if (mRefCnt > 0) {
      --mRefCnt;
      if (mRefCnt == 0) {
        t->Release();
      }
    }
  }
  void ForceDrop(T* t)
  {
    if (mRefCnt > 0) {
      mRefCnt = 0;
      t->Release();
    }
  }

  operator bool() const { return mRefCnt > 0; }

private:
  nsrefcnt mRefCnt;
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
  void SetChannelCountModeValue(ChannelCountMode aMode)
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

  void RemoveOutputParam(AudioParam* aParam);

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
