





#ifndef mozilla_layers_AxisPhysicsModel_h
#define mozilla_layers_AxisPhysicsModel_h

#include "AxisPhysicsModel.h"
#include <sys/types.h>                  
#include "mozilla/TimeStamp.h"          

namespace mozilla {
namespace layers {











class AxisPhysicsModel {
public:
  AxisPhysicsModel(double aInitialPosition, double aInitialVelocity);
  ~AxisPhysicsModel();

  



  void Simulate(const TimeDuration& aDeltaTime);

  


  double GetVelocity();

  


  void SetVelocity(double aVelocity);

  


  double GetPosition();

  


  void SetPosition(double aPosition);

protected:

  struct State
  {
    State(double ap, double av) : p(ap), v(av) {};
    double p; 
    double v; 
  };

  struct Derivative
  {
    Derivative() : dp(0.0), dv(0.0) {};
    Derivative(double aDp, double aDv) : dp(aDp), dv(aDv) {};
    double dp; 
    double dv; 
  };

  




  virtual double Acceleration(const State &aState) = 0;

private:

  


  static const double kFixedTimestep;

  



  double mProgress;

  



  State mPrevState;

  



  State mNextState;

  



  void Integrate(double aDeltaTime);

  



  Derivative Evaluate(const State &aInitState, double aDeltaTime,
                      const Derivative &aDerivative);

  


  static double LinearInterpolate(double aV1, double aV2, double aBlend);

};


}
}

#endif
