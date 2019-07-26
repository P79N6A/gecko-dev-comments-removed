





#ifndef PannerNode_h_
#define PannerNode_h_

#include "AudioNode.h"
#include "AudioParam.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/TypedEnum.h"
#include "mozilla/dom/PannerNodeBinding.h"
#include "ThreeDPoint.h"
#include "mozilla/WeakPtr.h"

namespace mozilla {
namespace dom {

class AudioContext;

class PannerNode : public AudioNode,
                   public SupportsWeakPtr<PannerNode>
{
public:
  explicit PannerNode(AudioContext* aContext);
  virtual ~PannerNode();

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope);

  virtual bool SupportsMediaStreams() const MOZ_OVERRIDE
  {
    return true;
  }

  PanningModelType PanningModel() const
  {
    return mPanningModel;
  }
  void SetPanningModel(PanningModelType aPanningModel)
  {
    mPanningModel = aPanningModel;
    SendInt32ParameterToStream(PANNING_MODEL, int32_t(mPanningModel));
  }

  DistanceModelType DistanceModel() const
  {
    return mDistanceModel;
  }
  void SetDistanceModel(DistanceModelType aDistanceModel)
  {
    mDistanceModel = aDistanceModel;
    SendInt32ParameterToStream(DISTANCE_MODEL, int32_t(mDistanceModel));
  }

  void SetPosition(double aX, double aY, double aZ)
  {
    mPosition.x = aX;
    mPosition.y = aY;
    mPosition.z = aZ;
    SendThreeDPointParameterToStream(POSITION, mPosition);
  }

  void SetOrientation(double aX, double aY, double aZ)
  {
    mOrientation.x = aX;
    mOrientation.y = aY;
    mOrientation.z = aZ;
    SendThreeDPointParameterToStream(ORIENTATION, mOrientation);
  }

  void SetVelocity(double aX, double aY, double aZ)
  {
    mVelocity.x = aX;
    mVelocity.y = aY;
    mVelocity.z = aZ;
    SendThreeDPointParameterToStream(VELOCITY, mVelocity);
  }

  double RefDistance() const
  {
    return mRefDistance;
  }
  void SetRefDistance(double aRefDistance)
  {
    mRefDistance = aRefDistance;
    SendDoubleParameterToStream(REF_DISTANCE, mRefDistance);
  }

  double MaxDistance() const
  {
    return mMaxDistance;
  }
  void SetMaxDistance(double aMaxDistance)
  {
    mMaxDistance = aMaxDistance;
    SendDoubleParameterToStream(MAX_DISTANCE, mMaxDistance);
  }

  double RolloffFactor() const
  {
    return mRolloffFactor;
  }
  void SetRolloffFactor(double aRolloffFactor)
  {
    mRolloffFactor = aRolloffFactor;
    SendDoubleParameterToStream(ROLLOFF_FACTOR, mRolloffFactor);
  }

  double ConeInnerAngle() const
  {
    return mConeInnerAngle;
  }
  void SetConeInnerAngle(double aConeInnerAngle)
  {
    mConeInnerAngle = aConeInnerAngle;
    SendDoubleParameterToStream(CONE_INNER_ANGLE, mConeInnerAngle);
  }

  double ConeOuterAngle() const
  {
    return mConeOuterAngle;
  }
  void SetConeOuterAngle(double aConeOuterAngle)
  {
    mConeOuterAngle = aConeOuterAngle;
    SendDoubleParameterToStream(CONE_OUTER_ANGLE, mConeOuterAngle);
  }

  double ConeOuterGain() const
  {
    return mConeOuterGain;
  }
  void SetConeOuterGain(double aConeOuterGain)
  {
    mConeOuterGain = aConeOuterGain;
    SendDoubleParameterToStream(CONE_OUTER_GAIN, mConeOuterGain);
  }

private:
  friend class AudioListener;
  friend class PannerNodeEngine;
  enum EngineParameters {
    LISTENER_POSITION,
    LISTENER_ORIENTATION,
    LISTENER_UPVECTOR,
    LISTENER_VELOCITY,
    LISTENER_DOPPLER_FACTOR,
    LISTENER_SPEED_OF_SOUND,
    PANNING_MODEL,
    DISTANCE_MODEL,
    POSITION,
    ORIENTATION,
    VELOCITY,
    REF_DISTANCE,
    MAX_DISTANCE,
    ROLLOFF_FACTOR,
    CONE_INNER_ANGLE,
    CONE_OUTER_ANGLE,
    CONE_OUTER_GAIN
  };

private:
  PanningModelType mPanningModel;
  DistanceModelType mDistanceModel;
  ThreeDPoint mPosition;
  ThreeDPoint mOrientation;
  ThreeDPoint mVelocity;
  double mRefDistance;
  double mMaxDistance;
  double mRolloffFactor;
  double mConeInnerAngle;
  double mConeOuterAngle;
  double mConeOuterGain;
};

}
}

#endif

