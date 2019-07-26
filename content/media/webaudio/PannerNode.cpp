





#include "PannerNode.h"
#include "AudioNodeEngine.h"
#include "AudioNodeStream.h"
#include "AudioListener.h"
#include "AudioBufferSourceNode.h"
#include "blink/HRTFPanner.h"
#include "blink/HRTFDatabaseLoader.h"

using WebCore::HRTFDatabaseLoader;
using WebCore::HRTFPanner;

namespace mozilla {
namespace dom {

using namespace std;

NS_IMPL_CYCLE_COLLECTION_CLASS(PannerNode)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(PannerNode)
  if (tmp->Context()) {
    tmp->Context()->UnregisterPannerNode(tmp);
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_END_INHERITED(AudioNode)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(PannerNode, AudioNode)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(PannerNode)
NS_INTERFACE_MAP_END_INHERITING(AudioNode)

NS_IMPL_ADDREF_INHERITED(PannerNode, AudioNode)
NS_IMPL_RELEASE_INHERITED(PannerNode, AudioNode)

class PannerNodeEngine : public AudioNodeEngine
{
public:
  explicit PannerNodeEngine(AudioNode* aNode)
    : AudioNodeEngine(aNode)
    
    , mPanningModelFunction(&PannerNodeEngine::HRTFPanningFunction)
    , mDistanceModelFunction(&PannerNodeEngine::InverseGainFunction)
    , mPosition()
    , mOrientation(1., 0., 0.)
    , mVelocity()
    , mRefDistance(1.)
    , mMaxDistance(10000.)
    , mRolloffFactor(1.)
    , mConeInnerAngle(360.)
    , mConeOuterAngle(360.)
    , mConeOuterGain(0.)
    
    
    , mListenerDopplerFactor(0.)
    , mListenerSpeedOfSound(0.)
  {
    
    TemporaryRef<HRTFDatabaseLoader> loader =
      HRTFDatabaseLoader::createAndLoadAsynchronouslyIfNecessary(aNode->Context()->SampleRate());
    mHRTFPanner = new HRTFPanner(aNode->Context()->SampleRate(), loader);
  }

  virtual void SetInt32Parameter(uint32_t aIndex, int32_t aParam) MOZ_OVERRIDE
  {
    switch (aIndex) {
    case PannerNode::PANNING_MODEL:
      switch (PanningModelType(aParam)) {
        case PanningModelType::Equalpower:
          mPanningModelFunction = &PannerNodeEngine::EqualPowerPanningFunction;
          break;
        case PanningModelType::HRTF:
          mPanningModelFunction = &PannerNodeEngine::HRTFPanningFunction;
          break;
        default:
          NS_NOTREACHED("We should never see the alternate names here");
          break;
      }
      break;
    case PannerNode::DISTANCE_MODEL:
      switch (DistanceModelType(aParam)) {
        case DistanceModelType::Inverse:
          mDistanceModelFunction = &PannerNodeEngine::InverseGainFunction;
          break;
        case DistanceModelType::Linear:
          mDistanceModelFunction = &PannerNodeEngine::LinearGainFunction;
          break;
        case DistanceModelType::Exponential:
          mDistanceModelFunction = &PannerNodeEngine::ExponentialGainFunction;
          break;
        default:
          NS_NOTREACHED("We should never see the alternate names here");
          break;
      }
      break;
    default:
      NS_ERROR("Bad PannerNodeEngine Int32Parameter");
    }
  }
  virtual void SetThreeDPointParameter(uint32_t aIndex, const ThreeDPoint& aParam) MOZ_OVERRIDE
  {
    switch (aIndex) {
    case PannerNode::LISTENER_POSITION: mListenerPosition = aParam; break;
    case PannerNode::LISTENER_FRONT_VECTOR: mListenerFrontVector = aParam; break;
    case PannerNode::LISTENER_RIGHT_VECTOR: mListenerRightVector = aParam; break;
    case PannerNode::LISTENER_VELOCITY: mListenerVelocity = aParam; break;
    case PannerNode::POSITION: mPosition = aParam; break;
    case PannerNode::ORIENTATION: mOrientation = aParam; break;
    case PannerNode::VELOCITY: mVelocity = aParam; break;
    default:
      NS_ERROR("Bad PannerNodeEngine ThreeDPointParameter");
    }
  }
  virtual void SetDoubleParameter(uint32_t aIndex, double aParam) MOZ_OVERRIDE
  {
    switch (aIndex) {
    case PannerNode::LISTENER_DOPPLER_FACTOR: mListenerDopplerFactor = aParam; break;
    case PannerNode::LISTENER_SPEED_OF_SOUND: mListenerSpeedOfSound = aParam; break;
    case PannerNode::REF_DISTANCE: mRefDistance = aParam; break;
    case PannerNode::MAX_DISTANCE: mMaxDistance = aParam; break;
    case PannerNode::ROLLOFF_FACTOR: mRolloffFactor = aParam; break;
    case PannerNode::CONE_INNER_ANGLE: mConeInnerAngle = aParam; break;
    case PannerNode::CONE_OUTER_ANGLE: mConeOuterAngle = aParam; break;
    case PannerNode::CONE_OUTER_GAIN: mConeOuterGain = aParam; break;
    default:
      NS_ERROR("Bad PannerNodeEngine DoubleParameter");
    }
  }

  virtual void ProduceAudioBlock(AudioNodeStream* aStream,
                                 const AudioChunk& aInput,
                                 AudioChunk* aOutput,
                                 bool *aFinished) MOZ_OVERRIDE
  {
    (this->*mPanningModelFunction)(aInput, aOutput);
  }

  void ComputeAzimuthAndElevation(float& aAzimuth, float& aElevation);
  void DistanceAndConeGain(AudioChunk* aChunk, float aGain);
  float ComputeConeGain();
  
  float ComputeDistanceGain();

  void GainMonoToStereo(const AudioChunk& aInput, AudioChunk* aOutput,
                        float aGainL, float aGainR);
  void GainStereoToStereo(const AudioChunk& aInput, AudioChunk* aOutput,
                          float aGainL, float aGainR, double aAzimuth);

  void EqualPowerPanningFunction(const AudioChunk& aInput, AudioChunk* aOutput);
  void HRTFPanningFunction(const AudioChunk& aInput, AudioChunk* aOutput);

  float LinearGainFunction(float aDistance);
  float InverseGainFunction(float aDistance);
  float ExponentialGainFunction(float aDistance);

  nsAutoPtr<HRTFPanner> mHRTFPanner;
  typedef void (PannerNodeEngine::*PanningModelFunction)(const AudioChunk& aInput, AudioChunk* aOutput);
  PanningModelFunction mPanningModelFunction;
  typedef float (PannerNodeEngine::*DistanceModelFunction)(float aDistance);
  DistanceModelFunction mDistanceModelFunction;
  ThreeDPoint mPosition;
  ThreeDPoint mOrientation;
  ThreeDPoint mVelocity;
  double mRefDistance;
  double mMaxDistance;
  double mRolloffFactor;
  double mConeInnerAngle;
  double mConeOuterAngle;
  double mConeOuterGain;
  ThreeDPoint mListenerPosition;
  ThreeDPoint mListenerFrontVector;
  ThreeDPoint mListenerRightVector;
  ThreeDPoint mListenerVelocity;
  double mListenerDopplerFactor;
  double mListenerSpeedOfSound;
};

PannerNode::PannerNode(AudioContext* aContext)
  : AudioNode(aContext,
              2,
              ChannelCountMode::Clamped_max,
              ChannelInterpretation::Speakers)
  
  , mPanningModel(PanningModelType::HRTF)
  , mDistanceModel(DistanceModelType::Inverse)
  , mPosition()
  , mOrientation(1., 0., 0.)
  , mVelocity()
  , mRefDistance(1.)
  , mMaxDistance(10000.)
  , mRolloffFactor(1.)
  , mConeInnerAngle(360.)
  , mConeOuterAngle(360.)
  , mConeOuterGain(0.)
{
  mStream = aContext->Graph()->CreateAudioNodeStream(new PannerNodeEngine(this),
                                                     MediaStreamGraph::INTERNAL_STREAM);
  
  Context()->Listener()->RegisterPannerNode(this);
}

PannerNode::~PannerNode()
{
  if (Context()) {
    Context()->UnregisterPannerNode(this);
  }
}

JSObject*
PannerNode::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return PannerNodeBinding::Wrap(aCx, aScope, this);
}

void PannerNode::DestroyMediaStream()
{
  if (Context()) {
    Context()->UnregisterPannerNode(this);
  }
  AudioNode::DestroyMediaStream();
}


float
PannerNodeEngine::LinearGainFunction(float aDistance)
{
  return 1 - mRolloffFactor * (aDistance - mRefDistance) / (mMaxDistance - mRefDistance);
}

float
PannerNodeEngine::InverseGainFunction(float aDistance)
{
  return mRefDistance / (mRefDistance + mRolloffFactor * (aDistance - mRefDistance));
}

float
PannerNodeEngine::ExponentialGainFunction(float aDistance)
{
  return pow(aDistance / mRefDistance, -mRolloffFactor);
}

void
PannerNodeEngine::HRTFPanningFunction(const AudioChunk& aInput,
                                      AudioChunk* aOutput)
{
  int numChannels = aInput.mChannelData.Length();

  
  AllocateAudioBlock(2, aOutput);

  float azimuth, elevation;
  ComputeAzimuthAndElevation(azimuth, elevation);

  AudioChunk input = aInput;
  
  if (!input.IsNull()) {
    float gain = ComputeConeGain() * ComputeDistanceGain() * aInput.mVolume;
    if (gain != 1.0f) {
      AllocateAudioBlock(numChannels, &input);
      for (int i = 0; i < numChannels; ++i) {
        const float* src = static_cast<const float*>(aInput.mChannelData[i]);
        float* dest =
          static_cast<float*>(const_cast<void*>(input.mChannelData[i]));
        AudioBlockCopyChannelWithScale(src, gain, dest);
      }
    }
  }

  mHRTFPanner->pan(azimuth, elevation, &input, aOutput, WEBAUDIO_BLOCK_SIZE);
}

void
PannerNodeEngine::EqualPowerPanningFunction(const AudioChunk& aInput,
                                            AudioChunk* aOutput)
{
  if (aInput.IsNull()) {
    *aOutput = aInput;
    return;
  }

  float azimuth, elevation, gainL, gainR, normalizedAzimuth, distanceGain, coneGain;
  int inputChannels = aInput.mChannelData.Length();

  
  
  if (mListenerPosition == mPosition &&
      mConeInnerAngle == 360 &&
      mConeOuterAngle == 360) {
    *aOutput = aInput;
    return;
  }

  
  AllocateAudioBlock(2, aOutput);

  ComputeAzimuthAndElevation(azimuth, elevation);
  coneGain = ComputeConeGain();

  
  
  azimuth = min(180.f, max(-180.f, azimuth));

  
  if (azimuth < -90.f) {
    azimuth = -180.f - azimuth;
  } else if (azimuth > 90) {
    azimuth = 180.f - azimuth;
  }

  
  if (inputChannels == 1) {
    normalizedAzimuth = (azimuth + 90.f) / 180.f;
  } else {
    if (azimuth <= 0) {
      normalizedAzimuth = (azimuth + 90.f) / 90.f;
    } else {
      normalizedAzimuth = azimuth / 90.f;
    }
  }

  distanceGain = ComputeDistanceGain();

  
  gainL = cos(0.5 * M_PI * normalizedAzimuth) * aInput.mVolume;
  gainR = sin(0.5 * M_PI * normalizedAzimuth) * aInput.mVolume;

  
  if (inputChannels == 1) {
    GainMonoToStereo(aInput, aOutput, gainL, gainR);
  } else {
    GainStereoToStereo(aInput, aOutput, gainL, gainR, azimuth);
  }

  DistanceAndConeGain(aOutput, distanceGain * coneGain);
}

void
PannerNodeEngine::GainMonoToStereo(const AudioChunk& aInput, AudioChunk* aOutput,
                                   float aGainL, float aGainR)
{
  float* outputL = static_cast<float*>(const_cast<void*>(aOutput->mChannelData[0]));
  float* outputR = static_cast<float*>(const_cast<void*>(aOutput->mChannelData[1]));
  const float* input = static_cast<float*>(const_cast<void*>(aInput.mChannelData[0]));

  AudioBlockPanMonoToStereo(input, aGainL, aGainR, outputL, outputR);
}

void
PannerNodeEngine::GainStereoToStereo(const AudioChunk& aInput, AudioChunk* aOutput,
                                     float aGainL, float aGainR, double aAzimuth)
{
  float* outputL = static_cast<float*>(const_cast<void*>(aOutput->mChannelData[0]));
  float* outputR = static_cast<float*>(const_cast<void*>(aOutput->mChannelData[1]));
  const float* inputL = static_cast<float*>(const_cast<void*>(aInput.mChannelData[0]));
  const float* inputR = static_cast<float*>(const_cast<void*>(aInput.mChannelData[1]));

  AudioBlockPanStereoToStereo(inputL, inputR, aGainL, aGainR, aAzimuth <= 0, outputL, outputR);
}

void
PannerNodeEngine::DistanceAndConeGain(AudioChunk* aChunk, float aGain)
{
  float* samples = static_cast<float*>(const_cast<void*>(*aChunk->mChannelData.Elements()));
  uint32_t channelCount = aChunk->mChannelData.Length();

  AudioBufferInPlaceScale(samples, channelCount, aGain);
}


void
PannerNodeEngine::ComputeAzimuthAndElevation(float& aAzimuth, float& aElevation)
{
  ThreeDPoint sourceListener = mPosition - mListenerPosition;

  if (sourceListener.IsZero()) {
    aAzimuth = 0.0;
    aElevation = 0.0;
    return;
  }

  sourceListener.Normalize();

  
  const ThreeDPoint& listenerFront = mListenerFrontVector;
  const ThreeDPoint& listenerRight = mListenerRightVector;
  ThreeDPoint up = listenerRight.CrossProduct(listenerFront);

  double upProjection = sourceListener.DotProduct(up);
  aElevation = 90 - 180 * acos(upProjection) / M_PI;

  if (aElevation > 90) {
    aElevation = 180 - aElevation;
  } else if (aElevation < -90) {
    aElevation = -180 - aElevation;
  }

  ThreeDPoint projectedSource = sourceListener - up * upProjection;
  if (projectedSource.IsZero()) {
    
    aAzimuth = 0.0;
    return;
  }
  projectedSource.Normalize();

  
  double projection = projectedSource.DotProduct(listenerRight);
  aAzimuth = 180 * acos(projection) / M_PI;

  
  double frontBack = projectedSource.DotProduct(listenerFront);
  if (frontBack < 0) {
    aAzimuth = 360 - aAzimuth;
  }
  
  
  if ((aAzimuth >= 0) && (aAzimuth <= 270)) {
    aAzimuth = 90 - aAzimuth;
  } else {
    aAzimuth = 450 - aAzimuth;
  }
}


float
PannerNodeEngine::ComputeConeGain()
{
  
  if (mOrientation.IsZero() || ((mConeInnerAngle == 360) && (mConeOuterAngle == 360))) {
    return 1;
  }

  
  ThreeDPoint sourceToListener = mListenerPosition - mPosition;
  sourceToListener.Normalize();

  
  double dotProduct = sourceToListener.DotProduct(mOrientation);
  double angle = 180 * acos(dotProduct) / M_PI;
  double absAngle = fabs(angle);

  
  double absInnerAngle = fabs(mConeInnerAngle) / 2;
  double absOuterAngle = fabs(mConeOuterAngle) / 2;
  double gain = 1;

  if (absAngle <= absInnerAngle) {
    
    gain = 1;
  } else if (absAngle >= absOuterAngle) {
    
    gain = mConeOuterGain;
  } else {
    
    
    double x = (absAngle - absInnerAngle) / (absOuterAngle - absInnerAngle);
    gain = (1 - x) + mConeOuterGain * x;
  }

  return gain;
}

float
PannerNodeEngine::ComputeDistanceGain()
{
  ThreeDPoint distanceVec = mPosition - mListenerPosition;
  float distance = sqrt(distanceVec.DotProduct(distanceVec));
  return (this->*mDistanceModelFunction)(distance);
}

float
PannerNode::ComputeDopplerShift()
{
  double dopplerShift = 1.0; 

  AudioListener* listener = Context()->Listener();

  if (listener->DopplerFactor() > 0) {
    
    if (!mVelocity.IsZero() || !listener->Velocity().IsZero()) {
      
      ThreeDPoint sourceToListener = mPosition - listener->Velocity();

      double sourceListenerMagnitude = sourceToListener.Magnitude();

      double listenerProjection = sourceToListener.DotProduct(listener->Velocity()) / sourceListenerMagnitude;
      double sourceProjection = sourceToListener.DotProduct(mVelocity) / sourceListenerMagnitude;

      listenerProjection = -listenerProjection;
      sourceProjection = -sourceProjection;

      double scaledSpeedOfSound = listener->DopplerFactor() / listener->DopplerFactor();
      listenerProjection = min(listenerProjection, scaledSpeedOfSound);
      sourceProjection = min(sourceProjection, scaledSpeedOfSound);

      dopplerShift = ((listener->SpeedOfSound() - listener->DopplerFactor() * listenerProjection) / (listener->SpeedOfSound() - listener->DopplerFactor() * sourceProjection));

      WebAudioUtils::FixNaN(dopplerShift); 

      
      dopplerShift = min(dopplerShift, 16.);
      dopplerShift = max(dopplerShift, 0.125);
    }
  }

  return dopplerShift;
}

void
PannerNode::FindConnectedSources()
{
  mSources.Clear();
  std::set<AudioNode*> cycleSet;
  FindConnectedSources(this, mSources, cycleSet);
}

void
PannerNode::FindConnectedSources(AudioNode* aNode,
                                 nsTArray<AudioBufferSourceNode*>& aSources,
                                 std::set<AudioNode*>& aNodesSeen)
{
  if (!aNode) {
    return;
  }

  const nsTArray<InputNode>& inputNodes = aNode->InputNodes();

  for(unsigned i = 0; i < inputNodes.Length(); i++) {
    
    if (aNodesSeen.find(inputNodes[i].mInputNode) != aNodesSeen.end()) {
      return;
    }
    aNodesSeen.insert(inputNodes[i].mInputNode);
    
    FindConnectedSources(inputNodes[i].mInputNode, aSources, aNodesSeen);

    
    AudioBufferSourceNode* node = inputNodes[i].mInputNode->AsAudioBufferSourceNode();
    if (node) {
      aSources.AppendElement(node);
    }
  }
}

void
PannerNode::SendDopplerToSourcesIfNeeded()
{
  
  
  if (!(Context()->Listener()->Velocity().IsZero() && mVelocity.IsZero())) {
    for(uint32_t i = 0; i < mSources.Length(); i++) {
      mSources[i]->SendDopplerShiftToStream(ComputeDopplerShift());
    }
  }
}


}
}

