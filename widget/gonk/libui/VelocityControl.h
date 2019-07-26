















#ifndef _ANDROIDFW_VELOCITY_CONTROL_H
#define _ANDROIDFW_VELOCITY_CONTROL_H

#include "Input.h"
#include "VelocityTracker.h"
#include <utils/Timers.h>

namespace android {




struct VelocityControlParameters {
    
    
    
    
    
    
    
    float scale;

    
    
    
    
    
    
    float lowThreshold;

    
    
    
    
    
    
    
    float highThreshold;

    
    
    
    
    
    
    float acceleration;

    VelocityControlParameters() :
            scale(1.0f), lowThreshold(0.0f), highThreshold(0.0f), acceleration(1.0f) {
    }

    VelocityControlParameters(float scale, float lowThreshold,
            float highThreshold, float acceleration) :
            scale(scale), lowThreshold(lowThreshold),
            highThreshold(highThreshold), acceleration(acceleration) {
    }
};




class VelocityControl {
public:
    VelocityControl();

    
    void setParameters(const VelocityControlParameters& parameters);

    

    void reset();

    

    void move(nsecs_t eventTime, float* deltaX, float* deltaY);

private:
    
    
    static const nsecs_t STOP_TIME = 500 * 1000000; 

    VelocityControlParameters mParameters;

    nsecs_t mLastMovementTime;
    VelocityTracker::Position mRawPosition;
    VelocityTracker mVelocityTracker;
};

} 

#endif 
