









#include "frame_dropper.h"
#include "internal_defines.h"
#include "trace.h"

namespace webrtc
{

VCMFrameDropper::VCMFrameDropper(WebRtc_Word32 vcmId)
:
_vcmId(vcmId),
_keyFrameSizeAvgKbits(0.9f),
_keyFrameRatio(0.99f),
_dropRatio(0.9f, 0.96f)
{
    Reset();
}

void
VCMFrameDropper::Reset()
{
    _keyFrameRatio.Reset(0.99f);
    _keyFrameRatio.Apply(1.0f, 1.0f/300.0f); 
    _keyFrameSizeAvgKbits.Reset(0.9f);
    _keyFrameCount = 0;
    _accumulator = 0.0f;
    _accumulatorMax = 150.0f; 
    _targetBitRate = 300.0f;
    _incoming_frame_rate = 30;
    _keyFrameSpreadFrames = 0.5f * _incoming_frame_rate;
    _dropNext = false;
    _dropRatio.Reset(0.9f);
    _dropRatio.Apply(0.0f, 0.0f); 
    _dropCount = 0;
    _windowSize = 0.5f;
    _wasBelowMax = true;
    _enabled = true;
    _fastMode = false; 
    
    _cap_buffer_size = 3.0f;
    
    _max_time_drops = 4.0f;
}

void
VCMFrameDropper::Enable(bool enable)
{
    _enabled = enable;
}

void
VCMFrameDropper::Fill(WebRtc_UWord32 frameSizeBytes, bool deltaFrame)
{
    if (!_enabled)
    {
        return;
    }
    float frameSizeKbits = 8.0f * static_cast<float>(frameSizeBytes) / 1000.0f;
    if (!deltaFrame && !_fastMode) 
    {
        _keyFrameSizeAvgKbits.Apply(1, frameSizeKbits);
        _keyFrameRatio.Apply(1.0, 1.0);
        if (frameSizeKbits > _keyFrameSizeAvgKbits.Value())
        {
            
            
            
            frameSizeKbits -= _keyFrameSizeAvgKbits.Value();
        }
        else
        {
            
            frameSizeKbits = 0;
        }
        if (_keyFrameRatio.Value() > 1e-5 && 1 / _keyFrameRatio.Value() < _keyFrameSpreadFrames)
        {
            
            
            
            
            _keyFrameCount = static_cast<WebRtc_Word32>(1 / _keyFrameRatio.Value() + 0.5);
        }
        else
        {
            
            _keyFrameCount = static_cast<WebRtc_Word32>(_keyFrameSpreadFrames + 0.5);
        }
    }
    else
    {
        
        _keyFrameRatio.Apply(1.0, 0.0);
    }
    
    _accumulator += frameSizeKbits;
    CapAccumulator();
}

void
VCMFrameDropper::Leak(WebRtc_UWord32 inputFrameRate)
{
    if (!_enabled)
    {
        return;
    }
    if (inputFrameRate < 1)
    {
        return;
    }
    if (_targetBitRate < 0.0f)
    {
        return;
    }
    _keyFrameSpreadFrames = 0.5f * inputFrameRate;
    
    
    
    float T = _targetBitRate / inputFrameRate;
    if (_keyFrameCount > 0)
    {
        
        if (_keyFrameRatio.Value() > 0 && 1 / _keyFrameRatio.Value() < _keyFrameSpreadFrames)
        {
            T -= _keyFrameSizeAvgKbits.Value() * _keyFrameRatio.Value();
        }
        else
        {
            T -= _keyFrameSizeAvgKbits.Value() / _keyFrameSpreadFrames;
        }
        _keyFrameCount--;
    }
    _accumulator -= T;
    UpdateRatio();
}

void
VCMFrameDropper::UpdateNack(WebRtc_UWord32 nackBytes)
{
    if (!_enabled)
    {
        return;
    }
    _accumulator += static_cast<float>(nackBytes) * 8.0f / 1000.0f;
}

void
VCMFrameDropper::FillBucket(float inKbits, float outKbits)
{
    _accumulator += (inKbits - outKbits);
}

void
VCMFrameDropper::UpdateRatio()
{
    if (_accumulator > 1.3f * _accumulatorMax)
    {
        
        _dropRatio.UpdateBase(0.8f);
    }
    else
    {
        
        _dropRatio.UpdateBase(0.9f);
    }
    if (_accumulator > _accumulatorMax)
    {
        
        
        
        if (_wasBelowMax)
        {
            _dropNext = true;
        }
        if (_fastMode)
        {
            
            _dropNext = true;
        }

        _dropRatio.Apply(1.0f, 1.0f);
        _dropRatio.UpdateBase(0.9f);
    }
    else
    {
        _dropRatio.Apply(1.0f, 0.0f);
    }
    if (_accumulator < 0.0f)
    {
        _accumulator = 0.0f;
    }
    _wasBelowMax = _accumulator < _accumulatorMax;
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding, VCMId(_vcmId),  "FrameDropper: dropRatio = %f accumulator = %f, accumulatorMax = %f", _dropRatio.Value(), _accumulator, _accumulatorMax);
}



bool
VCMFrameDropper::DropFrame()
{
    if (!_enabled)
    {
        return false;
    }
    if (_dropNext)
    {
        _dropNext = false;
        _dropCount = 0;
    }

    if (_dropRatio.Value() >= 0.5f) 
    {
        
        
        float denom = 1.0f - _dropRatio.Value();
        if (denom < 1e-5)
        {
            denom = (float)1e-5;
        }
        WebRtc_Word32 limit = static_cast<WebRtc_Word32>(1.0f / denom - 1.0f + 0.5f);
        
        
        int max_limit = static_cast<int>(_incoming_frame_rate *
                                         _max_time_drops);
        if (limit > max_limit) {
          limit = max_limit;
        }
        if (_dropCount < 0)
        {
            
            if (_dropRatio.Value() > 0.4f)
            {
                _dropCount = -_dropCount;
            }
            else
            {
                _dropCount = 0;
            }
        }
        if (_dropCount < limit)
        {
            
            _dropCount++;
            return true;
        }
        else
        {
            
            _dropCount = 0;
            return false;
        }
    }
    else if (_dropRatio.Value() > 0.0f && _dropRatio.Value() < 0.5f) 
    {
        
        
        
        float denom = _dropRatio.Value();
        if (denom < 1e-5)
        {
            denom = (float)1e-5;
        }
        WebRtc_Word32 limit = -static_cast<WebRtc_Word32>(1.0f / denom - 1.0f + 0.5f);
        if (_dropCount > 0)
        {
            
            
            if (_dropRatio.Value() < 0.6f)
            {
                _dropCount = -_dropCount;
            }
            else
            {
                _dropCount = 0;
            }
        }
        if (_dropCount > limit)
        {
            if (_dropCount == 0)
            {
                
                _dropCount--;
                return true;
            }
            else
            {
                
                _dropCount--;
                return false;
            }
        }
        else
        {
            _dropCount = 0;
            return false;
        }
    }
    _dropCount = 0;
    return false;

    
    
    
    
}

void
VCMFrameDropper::SetRates(float bitRate, float incoming_frame_rate)
{
    
    _accumulatorMax = bitRate * _windowSize; 
    if (_targetBitRate > 0.0f && bitRate < _targetBitRate && _accumulator > _accumulatorMax)
    {
        
        _accumulator = bitRate / _targetBitRate * _accumulator;
    }
    _targetBitRate = bitRate;
    CapAccumulator();
    _incoming_frame_rate = incoming_frame_rate;
}

float
VCMFrameDropper::ActualFrameRate(WebRtc_UWord32 inputFrameRate) const
{
    if (!_enabled)
    {
        return static_cast<float>(inputFrameRate);
    }
    return inputFrameRate * (1.0f - _dropRatio.Value());
}




void VCMFrameDropper::CapAccumulator() {
  float max_accumulator = _targetBitRate * _cap_buffer_size;
  if (_accumulator > max_accumulator) {
    _accumulator = max_accumulator;
  }
}

}
