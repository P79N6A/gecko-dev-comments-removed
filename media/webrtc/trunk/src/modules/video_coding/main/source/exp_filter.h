









#ifndef WEBRTC_MODULES_VIDEO_CODING_EXP_FILTER_H_
#define WEBRTC_MODULES_VIDEO_CODING_EXP_FILTER_H_

namespace webrtc
{





class VCMExpFilter
{
public:
    VCMExpFilter(float alpha, float max = -1.0) : _alpha(alpha), _filtered(-1.0), _max(max) {}

    
    
    
    
    void Reset(float alpha);

    
    
    
    
    
    float Apply(float exp, float sample);

    
    
    
    float Value() const;

    
    
    
    
    void UpdateBase(float alpha);

private:
    float          _alpha;     
    float          _filtered;  
    const float    _max;
}; 

} 

#endif 
