





#include "AxisPhysicsModel.h"

namespace mozilla {
namespace layers {














const double AxisPhysicsModel::kFixedTimestep = 1.0 / 120.0; 









AxisPhysicsModel::AxisPhysicsModel(double aInitialPosition,
                                   double aInitialVelocity)
  : mProgress(1.0)
  , mPrevState(aInitialPosition, aInitialVelocity)
  , mNextState(aInitialPosition, aInitialVelocity)
{

}

AxisPhysicsModel::~AxisPhysicsModel()
{

}

double
AxisPhysicsModel::GetVelocity()
{
  return LinearInterpolate(mPrevState.v, mNextState.v, mProgress);
}

double
AxisPhysicsModel::GetPosition()
{
  return LinearInterpolate(mPrevState.p, mNextState.p, mProgress);
}

void
AxisPhysicsModel::SetVelocity(double aVelocity)
{
  mNextState.v = aVelocity;
  mNextState.p = GetPosition();
  mProgress = 1.0;
}

void
AxisPhysicsModel::SetPosition(double aPosition)
{
  mNextState.v = GetVelocity();
  mNextState.p = aPosition;
  mProgress = 1.0;
}

void
AxisPhysicsModel::Simulate(const TimeDuration& aDeltaTime)
{
  for(mProgress += aDeltaTime.ToSeconds() / kFixedTimestep;
      mProgress > 1.0; mProgress -= 1.0) {
    Integrate(kFixedTimestep);
  }
}

void
AxisPhysicsModel::Integrate(double aDeltaTime)
{
  mPrevState = mNextState;

  
  
  Derivative a = Evaluate( mNextState, 0.0, Derivative() );
  Derivative b = Evaluate( mNextState, aDeltaTime * 0.5, a );
  Derivative c = Evaluate( mNextState, aDeltaTime * 0.5, b );
  Derivative d = Evaluate( mNextState, aDeltaTime, c );

  double dpdt = 1.0 / 6.0 * (a.dp + 2.0 * (b.dp + c.dp) + d.dp);
  double dvdt = 1.0 / 6.0 * (a.dv + 2.0 * (b.dv + c.dv) + d.dv);

  mNextState.p += dpdt * aDeltaTime;
  mNextState.v += dvdt * aDeltaTime;
}

AxisPhysicsModel::Derivative
AxisPhysicsModel::Evaluate(const State &aInitState, double aDeltaTime,
                           const Derivative &aDerivative)
{
  State state( aInitState.p + aDerivative.dp*aDeltaTime, aInitState.v + aDerivative.dv*aDeltaTime );

  return Derivative( state.v, Acceleration(state) );
}

double
AxisPhysicsModel::LinearInterpolate(double aV1, double aV2, double aBlend)
{
  return aV1 * (1.0 - aBlend) + aV2 * aBlend;
}

}
}
