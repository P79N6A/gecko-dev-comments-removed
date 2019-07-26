









#include "test_util.h"
#include "test_macros.h"
#include "rtp_dump.h"
#include <cmath>

using namespace webrtc;


#define PI  3.14159265
double
NormalDist(double mean, double stdDev)
{
    
    
    double uniform1 = (std::rand() + 1.0) / (RAND_MAX + 1.0);
    double uniform2 = (std::rand() + 1.0) / (RAND_MAX + 1.0);
    return (mean + stdDev * sqrt(-2 * log(uniform1)) * cos(2 * PI * uniform2));
}

RTPVideoCodecTypes
ConvertCodecType(const char* plname)
{
    if (strncmp(plname,"VP8" , 3) == 0)
    {
        return kRTPVideoVP8;
    }
    else if (strncmp(plname,"I420" , 5) == 0)
    {
        return kRTPVideoI420;
    }
    else
    {
        return kRTPVideoNoVideo; 
    }
}

