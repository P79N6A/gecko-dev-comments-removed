









#ifndef WEBRTC_MODULES_VIDEO_CODING_MEDIA_OPT_UTIL_H_
#define WEBRTC_MODULES_VIDEO_CODING_MEDIA_OPT_UTIL_H_

#include <math.h>
#include <stdlib.h>

#include "webrtc/modules/video_coding/main/source/internal_defines.h"
#include "webrtc/modules/video_coding/main/source/qm_select.h"
#include "webrtc/modules/video_coding/utility/include/exp_filter.h"
#include "webrtc/system_wrappers/interface/trace.h"
#include "webrtc/typedefs.h"

namespace webrtc {
namespace media_optimization {




enum { kLossPrHistorySize = 10 };


enum { kLossPrShortFilterWinMs = 1000 };


enum FilterPacketLossMode {
  kNoFilter,    
  kAvgFilter,   
  kMaxFilter    
                
};



enum HybridNackTH {
    kHighRttNackMs = 100,
    kLowRttNackMs = 20
};

struct VCMProtectionParameters
{
    VCMProtectionParameters() : rtt(0), lossPr(0.0f), bitRate(0.0f),
        packetsPerFrame(0.0f), packetsPerFrameKey(0.0f), frameRate(0.0f),
        keyFrameSize(0.0f), fecRateDelta(0), fecRateKey(0),
        residualPacketLossFec(0.0f), codecWidth(0), codecHeight(0),
        numLayers(1)
        {}

    int                 rtt;
    float               lossPr;
    float               bitRate;
    float               packetsPerFrame;
    float               packetsPerFrameKey;
    float               frameRate;
    float               keyFrameSize;
    uint8_t       fecRateDelta;
    uint8_t       fecRateKey;
    float               residualPacketLossFec;
    uint16_t      codecWidth;
    uint16_t      codecHeight;
    int                 numLayers;
};






enum VCMProtectionMethodEnum
{
    kNack,
    kFec,
    kNackFec,
    kNone
};

class VCMLossProbabilitySample
{
public:
    VCMLossProbabilitySample() : lossPr255(0), timeMs(-1) {};

    uint8_t     lossPr255;
    int64_t     timeMs;
};


class VCMProtectionMethod
{
public:
    VCMProtectionMethod();
    virtual ~VCMProtectionMethod();

    
    
    
    
    
    
    
    virtual bool UpdateParameters(const VCMProtectionParameters* parameters) = 0;

    
    
    
    enum VCMProtectionMethodEnum Type() const { return _type; }

    
    
    
    
    virtual float RequiredBitRate() { return _efficiency; }

    
    
    
    virtual uint8_t RequiredPacketLossER() { return _effectivePacketLoss; }

    
    
    
    virtual uint8_t RequiredProtectionFactorK() { return _protectionFactorK; }

    
    
    
    virtual uint8_t RequiredProtectionFactorD() { return _protectionFactorD; }

    
    
    
    virtual bool RequiredUepProtectionK() { return _useUepProtectionK; }

    
    
    
    virtual bool RequiredUepProtectionD() { return _useUepProtectionD; }

    virtual int MaxFramesFec() const { return 1; }

    
    void UpdateContentMetrics(const VideoContentMetrics* contentMetrics);

protected:

    uint8_t                        _effectivePacketLoss;
    uint8_t                        _protectionFactorK;
    uint8_t                        _protectionFactorD;
    
    float                                _residualPacketLossFec;
    float                                _scaleProtKey;
    int32_t                        _maxPayloadSize;

    VCMQmRobustness*                     _qmRobustness;
    bool                                 _useUepProtectionK;
    bool                                 _useUepProtectionD;
    float                                _corrFecCost;
    enum VCMProtectionMethodEnum         _type;
    float                                _efficiency;
};

class VCMNackMethod : public VCMProtectionMethod
{
public:
    VCMNackMethod();
    virtual ~VCMNackMethod();
    virtual bool UpdateParameters(const VCMProtectionParameters* parameters);
    
    bool EffectivePacketLoss(const VCMProtectionParameters* parameter);
};

class VCMFecMethod : public VCMProtectionMethod
{
public:
    VCMFecMethod();
    virtual ~VCMFecMethod();
    virtual bool UpdateParameters(const VCMProtectionParameters* parameters);
    
    bool EffectivePacketLoss(const VCMProtectionParameters* parameters);
    
    bool ProtectionFactor(const VCMProtectionParameters* parameters);
    
    uint8_t BoostCodeRateKey(uint8_t packetFrameDelta,
                                   uint8_t packetFrameKey) const;
    
    uint8_t ConvertFECRate(uint8_t codeRate) const;
    
    float AvgRecoveryFEC(const VCMProtectionParameters* parameters) const;
    
    void UpdateProtectionFactorD(uint8_t protectionFactorD);
    
    void UpdateProtectionFactorK(uint8_t protectionFactorK);
    
    int BitsPerFrame(const VCMProtectionParameters* parameters);

protected:
    enum { kUpperLimitFramesFec = 6 };
    
    
    
    enum { kMaxBytesPerFrameForFec = 700 };
    
    enum { kMaxBytesPerFrameForFecLow = 400 };
    
    enum { kMaxBytesPerFrameForFecHigh = 1000 };
    
    enum { kMaxRttTurnOffFec = 200 };
};


class VCMNackFecMethod : public VCMFecMethod
{
public:
    VCMNackFecMethod(int lowRttNackThresholdMs,
                     int highRttNackThresholdMs);
    virtual ~VCMNackFecMethod();
    virtual bool UpdateParameters(const VCMProtectionParameters* parameters);
    
    bool EffectivePacketLoss(const VCMProtectionParameters* parameters);
    
    bool ProtectionFactor(const VCMProtectionParameters* parameters);
    
    int MaxFramesFec() const;
    
    bool BitRateTooLowForFec(const VCMProtectionParameters* parameters);
private:
    int ComputeMaxFramesFec(const VCMProtectionParameters* parameters);

    int _lowRttNackMs;
    int _highRttNackMs;
    int _maxFramesFec;
};

class VCMLossProtectionLogic
{
public:
    VCMLossProtectionLogic(int64_t nowMs);
    ~VCMLossProtectionLogic();

    
    
    
    
    
    
    bool SetMethod(VCMProtectionMethodEnum newMethodType);

    
    
    
    
    
    bool RemoveMethod(VCMProtectionMethodEnum method);

    
    float RequiredBitRate() const;

    
    
    
    
    void UpdateRtt(uint32_t rtt);

    
    
    
    
    
    void UpdateResidualPacketLoss(float _residualPacketLoss);

    
    
    
    
    
    void UpdateFilteredLossPr(uint8_t packetLossEnc);

    
    
    
    
    void UpdateBitRate(float bitRate);

    
    
    
    
    void UpdatePacketsPerFrame(float nPackets, int64_t nowMs);

   
    
    
    
    void UpdatePacketsPerFrameKey(float nPackets, int64_t nowMs);

    
    
    
    
    void UpdateKeyFrameSize(float keyFrameSize);

    
    
    
    
    void UpdateFrameRate(float frameRate) { _frameRate = frameRate; }

    
    
    
    
    
    void UpdateFrameSize(uint16_t width, uint16_t height);

    
    
    
    
    void UpdateNumLayers(int numLayers);

    
    
    
    
    
    
    
    void UpdateFECRates(uint8_t fecRateKey, uint8_t fecRateDelta)
                       { _fecRateKey = fecRateKey;
                         _fecRateDelta = fecRateDelta; }

    
    
    
    bool UpdateMethod();

    
    
    
    VCMProtectionMethod* SelectedMethod() const;

    
    VCMProtectionMethodEnum SelectedType() const;

    
    
    
    

    
    uint8_t FilteredLoss(int64_t nowMs, FilterPacketLossMode filter_mode,
                               uint8_t lossPr255);

    void Reset(int64_t nowMs);

    void Release();

private:
    
    void UpdateMaxLossHistory(uint8_t lossPr255, int64_t now);
    uint8_t MaxFilteredLossPr(int64_t nowMs) const;
    VCMProtectionMethod*      _selectedMethod;
    VCMProtectionParameters   _currentParameters;
    uint32_t            _rtt;
    float                     _lossPr;
    float                     _bitRate;
    float                     _frameRate;
    float                     _keyFrameSize;
    uint8_t             _fecRateKey;
    uint8_t             _fecRateDelta;
    int64_t             _lastPrUpdateT;
    int64_t             _lastPacketPerFrameUpdateT;
    int64_t             _lastPacketPerFrameUpdateTKey;
    VCMExpFilter              _lossPr255;
    VCMLossProbabilitySample  _lossPrHistory[kLossPrHistorySize];
    uint8_t             _shortMaxLossPr255;
    VCMExpFilter              _packetsPerFrame;
    VCMExpFilter              _packetsPerFrameKey;
    float                     _residualPacketLossFec;
    uint16_t            _codecWidth;
    uint16_t            _codecHeight;
    int                       _numLayers;
};

}  
}  

#endif
