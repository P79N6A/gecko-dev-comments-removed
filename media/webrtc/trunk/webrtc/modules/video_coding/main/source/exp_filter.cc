









#include "exp_filter.h"

#include <math.h>

namespace webrtc {

void
VCMExpFilter::Reset(float alpha)
{
    _alpha = alpha;
    _filtered = -1.0;
}

float
VCMExpFilter::Apply(float exp, float sample)
{
    if (_filtered == -1.0)
    {
        
        _filtered = sample;
    }
    else if (exp == 1.0)
    {
        _filtered = _alpha * _filtered + (1 - _alpha) * sample;
    }
    else
    {
        float alpha = pow(_alpha, exp);
        _filtered = alpha * _filtered + (1 - alpha) * sample;
    }
    if (_max != -1 && _filtered > _max)
    {
        _filtered = _max;
    }
    return _filtered;
}

void
VCMExpFilter::UpdateBase(float alpha)
{
    _alpha = alpha;
}

float
VCMExpFilter::Value() const
{
    return _filtered;
}

}
