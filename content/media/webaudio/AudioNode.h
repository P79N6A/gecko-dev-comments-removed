





#ifndef AudioNode_h_
#define AudioNode_h_

#include "nsWrapperCache.h"
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














class AudioNode : public nsISupports,
                  public nsWrapperCache,
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
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(AudioNode)

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

  void Connect(AudioNode& aDestination, uint32_t aOutput,
               uint32_t aInput, ErrorResult& aRv);

  void Disconnect(uint32_t aOutput, ErrorResult& aRv);

  
  
  
  virtual uint32_t NumberOfInputs() const { return 1; }
  virtual uint32_t NumberOfOutputs() const { return 1; }

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

private:
  
  void DisconnectFromGraph();

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
};

}
}

#endif
