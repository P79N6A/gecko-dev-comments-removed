



























#include "ZeroPole.h"

#include <cmath>
#include <float.h>

namespace WebCore {

void ZeroPole::process(const float *source, float *destination, int framesToProcess)
{
    float zero = m_zero;
    float pole = m_pole;

    
    const float k1 = 1 / (1 - zero);
    const float k2 = 1 - pole;
    
    
    float lastX = m_lastX;
    float lastY = m_lastY;

    for (int i = 0; i < framesToProcess; ++i) {
        float input = source[i];

        
        float output1 = k1 * (input - zero * lastX);
        lastX = input;

        
        float output2 = k2 * output1 + pole * lastY;
        lastY = output2;

        destination[i] = output2;
    }
    
    
    
    if (lastX == 0.0f && lastY != 0.0f && fabsf(lastY) < FLT_MIN) {
        
        lastY = 0.0;
        
        for (int i = framesToProcess; i-- && fabsf(destination[i]) < FLT_MIN; ) {
            destination[i] = 0.0f;
        }
    }

    m_lastX = lastX;
    m_lastY = lastY;
}

} 
