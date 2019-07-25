









#include <stdio.h>
#include <ctime>
#include "JitterEstimateTest.h"

using namespace webrtc;

JitterEstimateTest::JitterEstimateTest(unsigned int frameRate) :
_frameRate(frameRate),
_capacity(2000),
_rate(500),
_jitter(5, 0),
_keyFrameRate(1.0),
_deltaFrameSize(10000, 1e6),
_counter(0),
_lossrate(0.0)
{
    
    _seed = static_cast<unsigned>(std::time(0));
    std::srand(_seed);
    _prevTimestamp = static_cast<unsigned int>((std::rand() + 1.0)/(RAND_MAX + 1.0)*(pow((float) 2, (long) sizeof(unsigned int)*8)-1));
    _prevWallClock = VCMTickTime::MillisecondTimestamp();
}

FrameSample
JitterEstimateTest::GenerateFrameSample()
{
    double increment = 1.0/_frameRate;
    unsigned int frameSize = static_cast<unsigned int>(_deltaFrameSize.RandValue());
    bool keyFrame = false;
    bool resent = false;
    _prevTimestamp += static_cast<unsigned int>(90000*increment + 0.5);
    double deltaFrameRate = _frameRate - _keyFrameRate;
    double ratio = deltaFrameRate/static_cast<double>(_keyFrameRate);
    if (ratio < 1.0)
    {
        ratio = 1.0/ratio;
        if (_counter >= ratio)
            _counter = 0;
        else
        {
            _counter++;
            frameSize += static_cast<unsigned int>(3*_deltaFrameSize.GetAverage());
            keyFrame = true;
        }
    }
    else
    {
        if (_counter >= ratio)
        {
            frameSize += static_cast<unsigned int>(3*_deltaFrameSize.GetAverage());
            _counter = 0;
            keyFrame = true;
        }
        else
            _counter++;
    }
    WebRtc_Word64 jitter =  static_cast<WebRtc_Word64>(_jitter.RandValue() + 1.0/_capacity * frameSize + 0.5);
    _prevWallClock += static_cast<WebRtc_Word64>(1000*increment + 0.5);
    double rndValue = RandUniform();
    resent = (rndValue < _lossrate);
    
    return FrameSample(_prevTimestamp, _prevWallClock + jitter, frameSize, keyFrame, resent);
}

void
JitterEstimateTest::SetCapacity(unsigned int c)
{
    _capacity = c;
}

void
JitterEstimateTest::SetRate(unsigned int r)
{
    _rate = r;
}

void
JitterEstimateTest::SetJitter(double m, double v)
{
    _jitter.SetParams(m, v);
}

void
JitterEstimateTest::SetFrameSizeStats(double m, double v)
{
    _deltaFrameSize.SetParams(m, v);
}

void
JitterEstimateTest::SetKeyFrameRate(int rate)
{
    _keyFrameRate = rate;
}

void
JitterEstimateTest::SetLossRate(double rate)
{
    _lossrate = rate;
}
