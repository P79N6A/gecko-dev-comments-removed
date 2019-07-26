





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

class AudioParam MOZ_FINAL : public nsWrapperCache,
                             public AudioParamTimeline
{
public:
  typedef void (*CallbackType)(AudioNode*);

  AudioParam(AudioNode* aNode,
             CallbackType aCallback,
             float aDefaultValue);
  virtual ~AudioParam();

  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(AudioParam)

  AudioContext* GetParentObject() const
  {
    return mNode->Context();
  }

  double DOMTimeToStreamTime(double aTime) const
  {
    return mNode->Context()->DOMTimeToStreamTime(aTime);
  }

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  
  
  void SetValueCurveAtTime(const Float32Array& aValues, double aStartTime, double aDuration, ErrorResult& aRv)
  {
    if (!WebAudioUtils::IsTimeValid(aStartTime)) {
      aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
      return;
    }
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
  void SetTargetValueAtTime(float aTarget, double aStartTime, double aTimeConstant, ErrorResult& aRv)
  {
    SetTargetAtTime(aTarget, aStartTime, aTimeConstant, aRv);
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

protected:
  nsCycleCollectingAutoRefCnt mRefCnt;
  NS_DECL_OWNINGTHREAD

private:
  nsRefPtr<AudioNode> mNode;
  
  
  nsTArray<AudioNode::InputNode> mInputNodes;
  CallbackType mCallback;
  const float mDefaultValue;
  
  nsRefPtr<MediaInputPort> mNodeStreamPort;
};

}
}

#endif

