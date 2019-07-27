





#ifndef mozilla_layers_AxisPhysicsMSDModel_h
#define mozilla_layers_AxisPhysicsMSDModel_h

#include "AxisPhysicsModel.h"

namespace mozilla {
namespace layers {





class AxisPhysicsMSDModel : public AxisPhysicsModel {
public:
  AxisPhysicsMSDModel(double aInitialPosition, double aInitialDestination,
                      double aInitialVelocity, double aSpringConstant,
                      double aDampingRatio);

  ~AxisPhysicsMSDModel();

  


  double GetDestination();

  


  void SetDestination(double aDestination);

  



  bool IsFinished();

protected:
  virtual double Acceleration(const State &aState);

private:

  



  double mDestination;

  


  double mSpringConstant;

  



  double mSpringConstantSqrtXTwo;

  












  double mDampingRatio;

};


}
}

#endif
