



























#include "ZeroPole.h"

#include "DenormalDisabler.h"

namespace WebCore {

void ZeroPole::process(const float *source, float *destination, unsigned framesToProcess)
{
    float zero = m_zero;
    float pole = m_pole;

    
    const float k1 = 1 / (1 - zero);
    const float k2 = 1 - pole;
    
    
    float lastX = m_lastX;
    float lastY = m_lastY;

    while (framesToProcess--) {
        float input = *source++;

        
        float output1 = k1 * (input - zero * lastX);
        lastX = input;

        
        float output2 = k2 * output1 + pole * lastY;
        lastY = output2;

        *destination++ = output2;
    }
    
    
    
    m_lastX = DenormalDisabler::flushDenormalFloatToZero(lastX);
    m_lastY = DenormalDisabler::flushDenormalFloatToZero(lastY);
}

} 
