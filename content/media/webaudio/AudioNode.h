





#ifndef AudioNode_h_
#define AudioNode_h_

#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"
#include "EnableWebAudioCheck.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "AudioContext.h"
#include "AudioParamTimeline.h"
#include "MediaStreamGraph.h"

struct JSContext;

namespace mozilla {

class ErrorResult;

namespace dom {

struct ThreeDPoint;




















class AudioNode : public nsISupports,
                  public EnableWebAudioCheck
{
public:
  explicit AudioNode(AudioContext* aContext);
  virtual ~AudioNode();

  
  
  
  virtual void DestroyMediaStream()
  {
    if (mStream) {
      mStream->Destroy();
      mStream = nullptr;
    }
  }

  
  
  virtual bool SupportsMediaStreams() const
  {
    return false;
  }

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(AudioNode)

  void JSBindingFinalized()
  {
    NS_ASSERTION(!mJSBindingFinalized, "JS binding already finalized");
    mJSBindingFinalized = true;
    UpdateOutputEnded();
  }

  AudioContext* GetParentObject() const
  {
    return mContext;
  }

  AudioContext* Context() const
  {
    return mContext;
  }

  void Connect(AudioNode& aDestination, uint32_t aOutput,
               uint32_t aInput, ErrorResult& aRv);

  void Disconnect(uint32_t aOutput, ErrorResult& aRv);

  
  
  
  virtual uint32_t NumberOfInputs() const { return 1; }
  virtual uint32_t NumberOfOutputs() const { return 1; }

  
  void UpdateOutputEnded();
  bool IsOutputEnded() const { return mOutputEnded; }

  struct InputNode {
    ~InputNode()
    {
      if (mStreamPort) {
        mStreamPort->Destroy();
      }
    }

    
    
    nsRefPtr<AudioNode> mInputNode;
    nsRefPtr<MediaInputPort> mStreamPort;
    
    uint32_t mInputPort;
    
    uint32_t mOutputPort;
  };

  MediaStream* Stream() { return mStream; }

  
  
  void SetProduceOwnOutput(bool aCanProduceOwnOutput)
  {
    mCanProduceOwnOutput = aCanProduceOwnOutput;
    if (!aCanProduceOwnOutput) {
      UpdateOutputEnded();
    }
  }

protected:
  static void Callback(AudioNode* aNode) {  }

  
  void SendDoubleParameterToStream(uint32_t aIndex, double aValue);
  void SendInt32ParameterToStream(uint32_t aIndex, int32_t aValue);
  void SendThreeDPointParameterToStream(uint32_t aIndex, const ThreeDPoint& aValue);
  static void SendTimelineParameterToStream(AudioNode* aNode, uint32_t aIndex,
                                            const AudioParamTimeline& aValue);

private:
  nsRefPtr<AudioContext> mContext;

protected:
  
  
  nsRefPtr<MediaStream> mStream;

private:
  
  
  nsTArray<InputNode> mInputNodes;
  
  
  
  
  nsTArray<nsRefPtr<AudioNode> > mOutputNodes;
  
  
  bool mJSBindingFinalized;
  
  
  bool mCanProduceOwnOutput;
  
  
  bool mOutputEnded;
};

}
}

#endif
