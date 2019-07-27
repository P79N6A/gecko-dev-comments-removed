





#ifndef AudioParam_h_
#define AudioParam_h_

#include "AudioParamTimeline.h"
#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "nsAutoPtr.h"
#include "AudioNode.h"
#include "mozilla/dom/TypedArray.h"
#include "WebAudioUtils.h"
#include "js/TypeDecls.h"

namespace mozilla {

namespace dom {

class AudioParam final : public nsWrapperCache,
                         public AudioParamTimeline
{
  virtual ~AudioParam();

public:
  typedef void (*CallbackType)(AudioNode*);

  AudioParam(AudioNode* aNode,
             CallbackType aCallback,
             float aDefaultValue,
             const char* aName);

  NS_IMETHOD_(MozExternalRefCountType) AddRef(void);
  NS_IMETHOD_(MozExternalRefCountType) Release(void);
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(AudioParam)

  AudioContext* GetParentObject() const
  {
    return mNode->Context();
  }

  double DOMTimeToStreamTime(double aTime) const
  {
    return mNode->Context()->DOMTimeToStreamTime(aTime);
  }

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  
  
  void SetValueCurveAtTime(const Float32Array& aValues, double aStartTime, double aDuration, ErrorResult& aRv)
  {
    if (!WebAudioUtils::IsTimeValid(aStartTime)) {
      aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
      return;
    }
    aValues.ComputeLengthAndData();
    AudioParamTimeline::SetValueCurveAtTime(aValues.Data(), aValues.Length(),
                                            DOMTimeToStreamTime(aStartTime), aDuration, aRv);
    mCallback(mNode);
  }

  
  
  void SetValue(float aValue)
  {
    
    if (HasSimpleValue() &&
        WebAudioUtils::FuzzyEqual(GetValue(), aValue)) {
      return;
    }
    AudioParamTimeline::SetValue(aValue);
    mCallback(mNode);
  }
  void SetValueAtTime(float aValue, double aStartTime, ErrorResult& aRv)
  {
    if (!WebAudioUtils::IsTimeValid(aStartTime)) {
      aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
      return;
    }
    AudioParamTimeline::SetValueAtTime(aValue, DOMTimeToStreamTime(aStartTime), aRv);
    mCallback(mNode);
  }
  void LinearRampToValueAtTime(float aValue, double aEndTime, ErrorResult& aRv)
  {
    if (!WebAudioUtils::IsTimeValid(aEndTime)) {
      aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
      return;
    }
    AudioParamTimeline::LinearRampToValueAtTime(aValue, DOMTimeToStreamTime(aEndTime), aRv);
    mCallback(mNode);
  }
  void ExponentialRampToValueAtTime(float aValue, double aEndTime, ErrorResult& aRv)
  {
    if (!WebAudioUtils::IsTimeValid(aEndTime)) {
      aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
      return;
    }
    AudioParamTimeline::ExponentialRampToValueAtTime(aValue, DOMTimeToStreamTime(aEndTime), aRv);
    mCallback(mNode);
  }
  void SetTargetAtTime(float aTarget, double aStartTime, double aTimeConstant, ErrorResult& aRv)
  {
    if (!WebAudioUtils::IsTimeValid(aStartTime) ||
        !WebAudioUtils::IsTimeValid(aTimeConstant)) {
      aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
      return;
    }
    AudioParamTimeline::SetTargetAtTime(aTarget, DOMTimeToStreamTime(aStartTime), aTimeConstant, aRv);
    mCallback(mNode);
  }
  void CancelScheduledValues(double aStartTime, ErrorResult& aRv)
  {
    if (!WebAudioUtils::IsTimeValid(aStartTime)) {
      aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
      return;
    }
    AudioParamTimeline::CancelScheduledValues(DOMTimeToStreamTime(aStartTime));
    mCallback(mNode);
  }

  uint32_t ParentNodeId()
  {
    return mNode->Id();
  }

  void GetName(nsAString& aName)
  {
    aName.AssignASCII(mName);
  }

  float DefaultValue() const
  {
    return mDefaultValue;
  }

  AudioNode* Node() const
  {
    return mNode;
  }

  const nsTArray<AudioNode::InputNode>& InputNodes() const
  {
    return mInputNodes;
  }

  void RemoveInputNode(uint32_t aIndex)
  {
    mInputNodes.RemoveElementAt(aIndex);
  }

  AudioNode::InputNode* AppendInputNode()
  {
    return mInputNodes.AppendElement();
  }

  void DisconnectFromGraphAndDestroyStream();

  
  MediaStream* Stream();

  virtual size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const override
  {
    size_t amount = AudioParamTimeline::SizeOfExcludingThis(aMallocSizeOf);
    
    

    
    amount += mInputNodes.SizeOfExcludingThis(aMallocSizeOf);

    if (mNodeStreamPort) {
      amount += mNodeStreamPort->SizeOfIncludingThis(aMallocSizeOf);
    }

    return amount;
  }

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const override
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

protected:
  nsCycleCollectingAutoRefCnt mRefCnt;
  NS_DECL_OWNINGTHREAD

private:
  nsRefPtr<AudioNode> mNode;
  
  
  nsTArray<AudioNode::InputNode> mInputNodes;
  CallbackType mCallback;
  const float mDefaultValue;
  const char* mName;
  
  nsRefPtr<MediaInputPort> mNodeStreamPort;
};

}
}

#endif

