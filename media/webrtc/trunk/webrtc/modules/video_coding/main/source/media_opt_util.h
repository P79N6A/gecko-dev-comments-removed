









#ifndef WEBRTC_MODULES_VIDEO_CODING_MEDIA_OPT_UTIL_H_
#define WEBRTC_MODULES_VIDEO_CODING_MEDIA_OPT_UTIL_H_

#include "typedefs.h"
#include "trace.h"
#include "exp_filter.h"
#include "internal_defines.h"
#include "qm_select.h"

#include <cmath>
#include <cstdlib>


namespace webrtc
{



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
    WebRtc_UWord8       fecRateDelta;
    WebRtc_UWord8       fecRateKey;
    float               residualPacketLossFec;
    WebRtc_UWord16      codecWidth;
    WebRtc_UWord16      codecHeight;
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

    WebRtc_UWord8     lossPr255;
    WebRtc_Word64     timeMs;
};


class VCMProtectionMethod
{
public:
    VCMProtectionMethod();
    virtual ~VCMProtectionMethod();

    
    
    
    
    
    
    
    virtual bool UpdateParameters(const VCMProtectionParameters* parameters) = 0;

    
    
    
    enum VCMProtectionMethodEnum Type() const { return _type; }

    
    
    
    
    virtual float RequiredBitRate() { return _efficiency; }

    
    
    
    virtual WebRtc_UWord8 RequiredPacketLossER() { return _effectivePacketLoss; }

    
    
    
    virtual WebRtc_UWord8 RequiredProtectionFactorK() { return _protectionFactorK; }

    
    
    
    virtual WebRtc_UWord8 RequiredProtectionFactorD() { return _protectionFactorD; }

    
    
    
    virtual bool RequiredUepProtectionK() { return _useUepProtectionK; }

    
    
    
    virtual bool RequiredUepProtectionD() { return _useUepProtectionD; }

    virtual int MaxFramesFec() const { return 1; }

    
    void UpdateContentMetrics(const VideoContentMetrics* contentMetrics);

protected:

    WebRtc_UWord8                        _effectivePacketLoss;
    WebRtc_UWord8                        _protectionFactorK;
    WebRtc_UWord8                        _protectionFactorD;
    
    float                                _residualPacketLossFec;
    float                                _scaleProtKey;
    WebRtc_Word32                        _maxPayloadSize;

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
    
    WebRtc_UWord8 BoostCodeRateKey(WebRtc_UWord8 packetFrameDelta,
                                   WebRtc_UWord8 packetFrameKey) const;
    
    WebRtc_UWord8 ConvertFECRate(WebRtc_UWord8 codeRate) const;
    
    float AvgRecoveryFEC(const VCMProtectionParameters* parameters) const;
    
    void UpdateProtectionFactorD(WebRtc_UWord8 protectionFactorD);
    
    void UpdateProtectionFactorK(WebRtc_UWord8 protectionFactorK);
    
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

    
    
    
    
    
    
    bool SetMethod(enum VCMProtectionMethodEnum newMethodType);

    
    
    
    
    
    bool RemoveMethod(enum VCMProtectionMethodEnum method);

    
    float RequiredBitRate() const;

    
    
    
    
    void UpdateRtt(WebRtc_UWord32 rtt);

    
    
    
    
    
    void UpdateResidualPacketLoss(float _residualPacketLoss);

    
    
    
    
    
    void UpdateFilteredLossPr(WebRtc_UWord8 packetLossEnc);

    
    
    
    
    void UpdateBitRate(float bitRate);

    
    
    
    
    void UpdatePacketsPerFrame(float nPackets, int64_t nowMs);

   
    
    
    
    void UpdatePacketsPerFrameKey(float nPackets, int64_t nowMs);

    
    
    
    
    void UpdateKeyFrameSize(float keyFrameSize);

    
    
    
    
    void UpdateFrameRate(float frameRate) { _frameRate = frameRate; }

    
    
    
    
    
    void UpdateFrameSize(WebRtc_UWord16 width, WebRtc_UWord16 height);

    
    
    
    
    void UpdateNumLayers(int numLayers);

    
    
    
    
    
    
    
    void UpdateFECRates(WebRtc_UWord8 fecRateKey, WebRtc_UWord8 fecRateDelta)
                       { _fecRateKey = fecRateKey;
                         _fecRateDelta = fecRateDelta; }

    
    
    
    bool UpdateMethod();

    
    
    
    VCMProtectionMethod* SelectedMethod() const;

    
    VCMProtectionMethodEnum SelectedType() const;

    
    
    
    

    
    WebRtc_UWord8 FilteredLoss(int64_t nowMs, FilterPacketLossMode filter_mode,
                               WebRtc_UWord8 lossPr255);

    void Reset(int64_t nowMs);

    void Release();

private:
    
    void UpdateMaxLossHistory(WebRtc_UWord8 lossPr255, WebRtc_Word64 now);
    WebRtc_UWord8 MaxFilteredLossPr(WebRtc_Word64 nowMs) const;
    VCMProtectionMethod*      _selectedMethod;
    VCMProtectionParameters   _currentParameters;
    WebRtc_UWord32            _rtt;
    float                     _lossPr;
    float                     _bitRate;
    float                     _frameRate;
    float                     _keyFrameSize;
    WebRtc_UWord8             _fecRateKey;
    WebRtc_UWord8             _fecRateDelta;
    WebRtc_Word64             _lastPrUpdateT;
    WebRtc_Word64             _lastPacketPerFrameUpdateT;
    WebRtc_Word64             _lastPacketPerFrameUpdateTKey;
    VCMExpFilter              _lossPr255;
    VCMLossProbabilitySample  _lossPrHistory[kLossPrHistorySize];
    WebRtc_UWord8             _shortMaxLossPr255;
    VCMExpFilter              _packetsPerFrame;
    VCMExpFilter              _packetsPerFrameKey;
    float                     _residualPacketLossFec;
    WebRtc_UWord16            _codecWidth;
    WebRtc_UWord16            _codecHeight;
    int                       _numLayers;
};

} 

#endif
