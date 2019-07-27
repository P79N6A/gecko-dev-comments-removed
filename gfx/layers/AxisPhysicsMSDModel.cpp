





#include "AxisPhysicsMSDModel.h"
#include <math.h>                       

namespace mozilla {
namespace layers {


















AxisPhysicsMSDModel::AxisPhysicsMSDModel(double aInitialPosition,
                                         double aInitialDestination,
                                         double aInitialVelocity,
                                         double aSpringConstant,
                                         double aDampingRatio)
  : AxisPhysicsModel(aInitialPosition, aInitialVelocity)
  , mDestination(aInitialDestination)
  , mSpringConstant(aSpringConstant)
  , mSpringConstantSqrtXTwo(sqrt(mSpringConstant) * 2.0)
  , mDampingRatio(aDampingRatio)
{
}

AxisPhysicsMSDModel::~AxisPhysicsMSDModel()
{
}

double
AxisPhysicsMSDModel::Acceleration(const State &aState)
{
  

  
  double spring_force = (mDestination - aState.p) * mSpringConstant;
  double damp_force = -aState.v * mDampingRatio * mSpringConstantSqrtXTwo;

  return spring_force + damp_force;
}


double
AxisPhysicsMSDModel::GetDestination()
{
  return mDestination;
}

void
AxisPhysicsMSDModel::SetDestination(double aDestination)
{
  mDestination = aDestination;
}

bool
AxisPhysicsMSDModel::IsFinished()
{
  
  
  
  
  

  
  
  
  
  const double kFinishDistance = 30.0;

  
  
  
  
  
  
  const double kFinishVelocity = 60.0;

  return fabs(mDestination - GetPosition ()) < kFinishDistance
    && fabs(GetVelocity()) <= kFinishVelocity;
}

}
}
