









#include "webrtc/modules/video_coding/main/source/media_opt_util.h"

#include <algorithm>
#include <float.h>
#include <limits.h>
#include <math.h>

#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/video_coding/codecs/vp8/include/vp8_common_types.h"
#include "webrtc/modules/video_coding/main/interface/video_coding_defines.h"
#include "webrtc/modules/video_coding/main/source/er_tables_xor.h"
#include "webrtc/modules/video_coding/main/source/fec_tables_xor.h"
#include "webrtc/modules/video_coding/main/source/nack_fec_tables.h"

namespace webrtc {
namespace media_optimization {

VCMProtectionMethod::VCMProtectionMethod():
_effectivePacketLoss(0),
_protectionFactorK(0),
_protectionFactorD(0),
_residualPacketLossFec(0.0f),
_scaleProtKey(2.0f),
_maxPayloadSize(1460),
_qmRobustness(new VCMQmRobustness()),
_useUepProtectionK(false),
_useUepProtectionD(true),
_corrFecCost(1.0),
_type(kNone),
_efficiency(0)
{
    
}

VCMProtectionMethod::~VCMProtectionMethod()
{
    delete _qmRobustness;
}
void
VCMProtectionMethod::UpdateContentMetrics(const
                                          VideoContentMetrics* contentMetrics)
{
    _qmRobustness->UpdateContent(contentMetrics);
}

VCMNackFecMethod::VCMNackFecMethod(int lowRttNackThresholdMs,
                                   int highRttNackThresholdMs)
    : VCMFecMethod(),
      _lowRttNackMs(lowRttNackThresholdMs),
      _highRttNackMs(highRttNackThresholdMs),
      _maxFramesFec(1) {
  assert(lowRttNackThresholdMs >= -1 && highRttNackThresholdMs >= -1);
  assert(highRttNackThresholdMs == -1 ||
         lowRttNackThresholdMs <= highRttNackThresholdMs);
  assert(lowRttNackThresholdMs > -1 || highRttNackThresholdMs == -1);
  _type = kNackFec;
}

VCMNackFecMethod::~VCMNackFecMethod()
{
    
}
bool
VCMNackFecMethod::ProtectionFactor(const VCMProtectionParameters* parameters)
{
    
    
    
    
    
    
    
    

    
    

    
    VCMFecMethod::ProtectionFactor(parameters);
    if (_lowRttNackMs == -1 || parameters->rtt < _lowRttNackMs)
    {
        _protectionFactorD = 0;
        VCMFecMethod::UpdateProtectionFactorD(_protectionFactorD);
    }

    
    
    else if (_highRttNackMs == -1 || parameters->rtt < _highRttNackMs)
    {
        
        
        float adjustRtt = 1.0f;

        
        
        _protectionFactorD = static_cast<uint8_t>
                            (adjustRtt *
                             static_cast<float>(_protectionFactorD));
        
        VCMFecMethod::UpdateProtectionFactorD(_protectionFactorD);
    }

    return true;
}

int VCMNackFecMethod::ComputeMaxFramesFec(
    const VCMProtectionParameters* parameters) {
  if (parameters->numLayers > 2) {
    
    
    
    return 1;
  }
  
  
  
  
  float base_layer_framerate = parameters->frameRate /
      static_cast<float>(1 << (parameters->numLayers - 1));
  int max_frames_fec = std::max(static_cast<int>(
      2.0f * base_layer_framerate * parameters->rtt /
      1000.0f + 0.5f), 1);
  
  
  if (max_frames_fec > kUpperLimitFramesFec) {
    max_frames_fec = kUpperLimitFramesFec;
  }
  return max_frames_fec;
}

int VCMNackFecMethod::MaxFramesFec() const {
  return _maxFramesFec;
}

bool VCMNackFecMethod::BitRateTooLowForFec(
    const VCMProtectionParameters* parameters) {
  
  
  
  
  
  int estimate_bytes_per_frame = 1000 * BitsPerFrame(parameters) / 8;
  int max_bytes_per_frame = kMaxBytesPerFrameForFec;
  int num_pixels = parameters->codecWidth * parameters->codecHeight;
  if (num_pixels <= 352 * 288) {
    max_bytes_per_frame = kMaxBytesPerFrameForFecLow;
  } else if (num_pixels > 640 * 480) {
    max_bytes_per_frame = kMaxBytesPerFrameForFecHigh;
  }
  
  
  if (estimate_bytes_per_frame < max_bytes_per_frame &&
      parameters->numLayers < 3 &&
      parameters->rtt < kMaxRttTurnOffFec) {
    return true;
  }
  return false;
}

bool
VCMNackFecMethod::EffectivePacketLoss(const VCMProtectionParameters* parameters)
{
    
    
    VCMFecMethod::EffectivePacketLoss(parameters);
    return true;
}

bool
VCMNackFecMethod::UpdateParameters(const VCMProtectionParameters* parameters)
{
    ProtectionFactor(parameters);
    EffectivePacketLoss(parameters);
    _maxFramesFec = ComputeMaxFramesFec(parameters);
    if (BitRateTooLowForFec(parameters)) {
      _protectionFactorK = 0;
      _protectionFactorD = 0;
    }

    

    
    float fecRate = static_cast<float> (_protectionFactorD) / 255.0f;
    _efficiency = parameters->bitRate * fecRate * _corrFecCost;

    
    if (_highRttNackMs == -1 || parameters->rtt < _highRttNackMs)
    {
        
        _efficiency += parameters->bitRate * _residualPacketLossFec /
                       (1.0f + _residualPacketLossFec);
    }

    
    
    
    
    
    _protectionFactorK = VCMFecMethod::ConvertFECRate(_protectionFactorK);
    _protectionFactorD = VCMFecMethod::ConvertFECRate(_protectionFactorD);

    return true;
}

VCMNackMethod::VCMNackMethod():
VCMProtectionMethod()
{
    _type = kNack;
}

VCMNackMethod::~VCMNackMethod()
{
    
}

bool
VCMNackMethod::EffectivePacketLoss(const VCMProtectionParameters* parameter)
{
    
    _effectivePacketLoss = 0;
    return true;
}

bool
VCMNackMethod::UpdateParameters(const VCMProtectionParameters* parameters)
{
    
    EffectivePacketLoss(parameters);

    
    _efficiency = parameters->bitRate * parameters->lossPr /
                  (1.0f + parameters->lossPr);
    return true;
}

VCMFecMethod::VCMFecMethod():
VCMProtectionMethod()
{
    _type = kFec;
}
VCMFecMethod::~VCMFecMethod()
{
    
}

uint8_t
VCMFecMethod::BoostCodeRateKey(uint8_t packetFrameDelta,
                               uint8_t packetFrameKey) const
{
    uint8_t boostRateKey = 2;
    
    uint8_t ratio = 1;

    if (packetFrameDelta > 0)
    {
        ratio = (int8_t) (packetFrameKey / packetFrameDelta);
    }
    ratio = VCM_MAX(boostRateKey, ratio);

    return ratio;
}

uint8_t
VCMFecMethod::ConvertFECRate(uint8_t codeRateRTP) const
{
    return static_cast<uint8_t> (VCM_MIN(255,(0.5 + 255.0 * codeRateRTP /
                                      (float)(255 - codeRateRTP))));
}


void
VCMFecMethod::UpdateProtectionFactorD(uint8_t protectionFactorD)
{
    _protectionFactorD = protectionFactorD;
}


void
VCMFecMethod::UpdateProtectionFactorK(uint8_t protectionFactorK)
{
    _protectionFactorK = protectionFactorK;
}




float
VCMFecMethod::AvgRecoveryFEC(const VCMProtectionParameters* parameters) const
{
    
    
    const uint16_t bitRatePerFrame = static_cast<uint16_t>
                        (parameters->bitRate / (parameters->frameRate));

    
    const uint8_t avgTotPackets = 1 + static_cast<uint8_t>
                        (static_cast<float> (bitRatePerFrame * 1000.0) /
                         static_cast<float> (8.0 * _maxPayloadSize) + 0.5);

    const float protectionFactor = static_cast<float>(_protectionFactorD) /
                                                      255.0;

    
    
    uint8_t fecPacketsPerFrame = static_cast<uint8_t>
                                      (protectionFactor * avgTotPackets);

    uint8_t sourcePacketsPerFrame = avgTotPackets - fecPacketsPerFrame;

    if ( (fecPacketsPerFrame == 0) || (sourcePacketsPerFrame == 0) )
    {
        
        return 0.0;
    }

    
    if (sourcePacketsPerFrame > kMaxNumPackets)
    {
        sourcePacketsPerFrame = kMaxNumPackets;
    }

    
    if (fecPacketsPerFrame > kMaxNumPackets)
    {
        fecPacketsPerFrame = kMaxNumPackets;
    }

    
    uint16_t codeIndexTable[kMaxNumPackets * kMaxNumPackets];
    uint16_t k = 0;
    for (uint8_t i = 1; i <= kMaxNumPackets; i++)
    {
        for (uint8_t j = 1; j <= i; j++)
        {
            codeIndexTable[(j - 1) * kMaxNumPackets + i - 1] = k;
            k += 1;
        }
    }

    uint8_t lossRate = static_cast<uint8_t> (255.0 *
                             parameters->lossPr + 0.5f);

    
    if (lossRate >= kPacketLossMax)
    {
        lossRate = kPacketLossMax - 1;
    }

    const uint16_t codeIndex = (fecPacketsPerFrame - 1) * kMaxNumPackets +
                                     (sourcePacketsPerFrame - 1);

    const uint16_t indexTable = codeIndexTable[codeIndex] * kPacketLossMax +
                                      lossRate;

    
    assert(indexTable < kSizeAvgFECRecoveryXOR);
    float avgFecRecov = static_cast<float>(kAvgFECRecoveryXOR[indexTable]);

    return avgFecRecov;
}

bool
VCMFecMethod::ProtectionFactor(const VCMProtectionParameters* parameters)
{
    

    
    uint8_t packetLoss = (uint8_t) (255 * parameters->lossPr);
    if (packetLoss == 0)
    {
        _protectionFactorK = 0;
        _protectionFactorD = 0;
         return true;
    }

    
    

    
    uint8_t firstPartitionProt = (uint8_t) (255 * 0.20);

    
    
    uint8_t minProtLevelFec = 85;

    
    
    uint8_t lossThr = 0;
    uint8_t packetNumThr = 1;

    
    const uint8_t ratePar1 = 5;
    const uint8_t ratePar2 = 49;

    
    float spatialSizeToRef = static_cast<float>
                           (parameters->codecWidth * parameters->codecHeight) /
                           (static_cast<float>(704 * 576));
    
    
    
    const float resolnFac = 1.0 / powf(spatialSizeToRef, 0.3f);

    const int bitRatePerFrame = BitsPerFrame(parameters);


    
    const uint8_t avgTotPackets = 1 + (uint8_t)
                                        ((float) bitRatePerFrame * 1000.0
                                       / (float) (8.0 * _maxPayloadSize) + 0.5);

    
    uint8_t codeRateDelta = 0;
    uint8_t codeRateKey = 0;

    
    
    
    const uint16_t effRateFecTable = static_cast<uint16_t>
                                           (resolnFac * bitRatePerFrame);
    uint8_t rateIndexTable =
        (uint8_t) VCM_MAX(VCM_MIN((effRateFecTable - ratePar1) /
                                         ratePar1, ratePar2), 0);

    
    
    if (packetLoss >= kPacketLossMax)
    {
        packetLoss = kPacketLossMax - 1;
    }
    uint16_t indexTable = rateIndexTable * kPacketLossMax + packetLoss;

    
    assert(indexTable < kSizeCodeRateXORTable);

    
    codeRateDelta = kCodeRateXORTable[indexTable];

    if (packetLoss > lossThr && avgTotPackets > packetNumThr)
    {
        
        if (codeRateDelta < firstPartitionProt)
        {
            codeRateDelta = firstPartitionProt;
        }
    }

    
    if (codeRateDelta >= kPacketLossMax)
    {
        codeRateDelta = kPacketLossMax - 1;
    }

    float adjustFec = 1.0f;
    
    
    if (parameters->numLayers == 1)
    {
        adjustFec = _qmRobustness->AdjustFecFactor(codeRateDelta,
                                                   parameters->bitRate,
                                                   parameters->frameRate,
                                                   parameters->rtt,
                                                   packetLoss);
    }

    codeRateDelta = static_cast<uint8_t>(codeRateDelta * adjustFec);

    
    
    
    
    const uint8_t packetFrameDelta = (uint8_t)
                                           (0.5 + parameters->packetsPerFrame);
    const uint8_t packetFrameKey = (uint8_t)
                                         (0.5 + parameters->packetsPerFrameKey);
    const uint8_t boostKey = BoostCodeRateKey(packetFrameDelta,
                                                    packetFrameKey);

    rateIndexTable = (uint8_t) VCM_MAX(VCM_MIN(
                      1 + (boostKey * effRateFecTable - ratePar1) /
                      ratePar1,ratePar2),0);
    uint16_t indexTableKey = rateIndexTable * kPacketLossMax + packetLoss;

    indexTableKey = VCM_MIN(indexTableKey, kSizeCodeRateXORTable);

    
    assert(indexTableKey < kSizeCodeRateXORTable);

    
    codeRateKey = kCodeRateXORTable[indexTableKey];

    
    int boostKeyProt = _scaleProtKey * codeRateDelta;
    if (boostKeyProt >= kPacketLossMax)
    {
        boostKeyProt = kPacketLossMax - 1;
    }

    
    
    codeRateKey = static_cast<uint8_t> (VCM_MAX(packetLoss,
            VCM_MAX(boostKeyProt, codeRateKey)));

    
    if (codeRateKey >= kPacketLossMax)
    {
        codeRateKey = kPacketLossMax - 1;
    }

    _protectionFactorK = codeRateKey;
    _protectionFactorD = codeRateDelta;

    
    
    
    
    
    
    
    

    float numPacketsFl = 1.0f + ((float) bitRatePerFrame * 1000.0
                                / (float) (8.0 * _maxPayloadSize) + 0.5);

    const float estNumFecGen = 0.5f + static_cast<float> (_protectionFactorD *
                                                         numPacketsFl / 255.0f);


    
    
    _corrFecCost = 1.0f;
    if (estNumFecGen < 1.1f && _protectionFactorD < minProtLevelFec)
    {
        _corrFecCost = 0.5f;
    }
    if (estNumFecGen < 0.9f && _protectionFactorD < minProtLevelFec)
    {
        _corrFecCost = 0.0f;
    }

     
    _useUepProtectionK = _qmRobustness->SetUepProtection(codeRateKey,
                                                         parameters->bitRate,
                                                         packetLoss,
                                                         0);

    _useUepProtectionD = _qmRobustness->SetUepProtection(codeRateDelta,
                                                         parameters->bitRate,
                                                         packetLoss,
                                                         1);

    
    return true;
}

int VCMFecMethod::BitsPerFrame(const VCMProtectionParameters* parameters) {
  
  
  const float bitRateRatio =
    kVp8LayerRateAlloction[parameters->numLayers - 1][0];
  float frameRateRatio = powf(1 / 2.0, parameters->numLayers - 1);
  float bitRate = parameters->bitRate * bitRateRatio;
  float frameRate = parameters->frameRate * frameRateRatio;

  
  float adjustmentFactor = 1;

  
  return static_cast<int>(adjustmentFactor * bitRate / frameRate);
}

bool
VCMFecMethod::EffectivePacketLoss(const VCMProtectionParameters* parameters)
{
    
    
    
    

    
    uint8_t packetLoss = (uint8_t) (255 * parameters->lossPr);

    float avgFecRecov = AvgRecoveryFEC(parameters);

    
    _residualPacketLossFec = (float) (packetLoss - avgFecRecov) / 255.0f;

    
    _effectivePacketLoss = 0;

    return true;
}

bool
VCMFecMethod::UpdateParameters(const VCMProtectionParameters* parameters)
{
    
    ProtectionFactor(parameters);

    
    EffectivePacketLoss(parameters);

    
    
    float fecRate = static_cast<float> (_protectionFactorD) / 255.0f;
    if (fecRate >= 0.0f)
    {
        
        
        
        

        
        
        _efficiency = parameters->bitRate * fecRate * _corrFecCost;
    }
    else
    {
        _efficiency = 0.0f;
    }

    
    
    
    
    
    _protectionFactorK = ConvertFECRate(_protectionFactorK);
    _protectionFactorD = ConvertFECRate(_protectionFactorD);

    return true;
}
VCMLossProtectionLogic::VCMLossProtectionLogic(int64_t nowMs):
_selectedMethod(NULL),
_currentParameters(),
_rtt(0),
_lossPr(0.0f),
_bitRate(0.0f),
_frameRate(0.0f),
_keyFrameSize(0.0f),
_fecRateKey(0),
_fecRateDelta(0),
_lastPrUpdateT(0),
_lossPr255(0.9999f),
_lossPrHistory(),
_shortMaxLossPr255(0),
_packetsPerFrame(0.9999f),
_packetsPerFrameKey(0.9999f),
_residualPacketLossFec(0),
_codecWidth(0),
_codecHeight(0),
_numLayers(1)
{
    Reset(nowMs);
}

VCMLossProtectionLogic::~VCMLossProtectionLogic()
{
    Release();
}

bool
VCMLossProtectionLogic::SetMethod(enum VCMProtectionMethodEnum newMethodType)
{
    if (_selectedMethod != NULL)
    {
        if (_selectedMethod->Type() == newMethodType)
        {
            
            return false;
        }
        
        delete _selectedMethod;
    }
    VCMProtectionMethod *newMethod = NULL;
    switch (newMethodType)
    {
        case kNack:
        {
            newMethod = new VCMNackMethod();
            break;
        }
        case kFec:
        {
            newMethod  = new VCMFecMethod();
            break;
        }
        case kNackFec:
        {
            
            newMethod =  new VCMNackFecMethod(kLowRttNackMs, -1);
            break;
        }
        default:
        {
          return false;
          break;
        }

    }
    _selectedMethod = newMethod;
    return true;
}
bool
VCMLossProtectionLogic::RemoveMethod(enum VCMProtectionMethodEnum method)
{
    if (_selectedMethod == NULL)
    {
        return false;
    }
    else if (_selectedMethod->Type() == method)
    {
        delete _selectedMethod;
        _selectedMethod = NULL;
    }
    return true;
}

float
VCMLossProtectionLogic::RequiredBitRate() const
{
    float RequiredBitRate = 0.0f;
    if (_selectedMethod != NULL)
    {
        RequiredBitRate = _selectedMethod->RequiredBitRate();
    }
    return RequiredBitRate;
}

void
VCMLossProtectionLogic::UpdateRtt(uint32_t rtt)
{
    _rtt = rtt;
}

void
VCMLossProtectionLogic::UpdateResidualPacketLoss(float residualPacketLoss)
{
    _residualPacketLossFec = residualPacketLoss;
}

void
VCMLossProtectionLogic::UpdateMaxLossHistory(uint8_t lossPr255,
                                             int64_t now)
{
    if (_lossPrHistory[0].timeMs >= 0 &&
        now - _lossPrHistory[0].timeMs < kLossPrShortFilterWinMs)
    {
        if (lossPr255 > _shortMaxLossPr255)
        {
            _shortMaxLossPr255 = lossPr255;
        }
    }
    else
    {
        
        if (_lossPrHistory[0].timeMs == -1)
        {
            
            _shortMaxLossPr255 = lossPr255;
        }
        else
        {
            
            for (int32_t i = (kLossPrHistorySize - 2); i >= 0; i--)
            {
                _lossPrHistory[i + 1].lossPr255 = _lossPrHistory[i].lossPr255;
                _lossPrHistory[i + 1].timeMs = _lossPrHistory[i].timeMs;
            }
        }
        if (_shortMaxLossPr255 == 0)
        {
            _shortMaxLossPr255 = lossPr255;
        }

        _lossPrHistory[0].lossPr255 = _shortMaxLossPr255;
        _lossPrHistory[0].timeMs = now;
        _shortMaxLossPr255 = 0;
    }
}

uint8_t
VCMLossProtectionLogic::MaxFilteredLossPr(int64_t nowMs) const
{
    uint8_t maxFound = _shortMaxLossPr255;
    if (_lossPrHistory[0].timeMs == -1)
    {
        return maxFound;
    }
    for (int32_t i = 0; i < kLossPrHistorySize; i++)
    {
        if (_lossPrHistory[i].timeMs == -1)
        {
            break;
        }
        if (nowMs - _lossPrHistory[i].timeMs >
            kLossPrHistorySize * kLossPrShortFilterWinMs)
        {
            
            break;
        }
        if (_lossPrHistory[i].lossPr255 > maxFound)
        {
            
            maxFound = _lossPrHistory[i].lossPr255;
        }
    }
    return maxFound;
}

uint8_t VCMLossProtectionLogic::FilteredLoss(
    int64_t nowMs,
    FilterPacketLossMode filter_mode,
    uint8_t lossPr255) {

  
  UpdateMaxLossHistory(lossPr255, nowMs);

  
  _lossPr255.Apply(static_cast<float> (nowMs - _lastPrUpdateT),
                   static_cast<float> (lossPr255));
  _lastPrUpdateT = nowMs;

  
  uint8_t filtered_loss = lossPr255;

  switch (filter_mode) {
    case kNoFilter:
      break;
    case kAvgFilter:
      filtered_loss = static_cast<uint8_t> (_lossPr255.Value() + 0.5);
      break;
    case kMaxFilter:
      filtered_loss = MaxFilteredLossPr(nowMs);
      break;
  }

  return filtered_loss;
}

void
VCMLossProtectionLogic::UpdateFilteredLossPr(uint8_t packetLossEnc)
{
    _lossPr = (float) packetLossEnc / (float) 255.0;
}

void
VCMLossProtectionLogic::UpdateBitRate(float bitRate)
{
    _bitRate = bitRate;
}

void
VCMLossProtectionLogic::UpdatePacketsPerFrame(float nPackets, int64_t nowMs)
{
    _packetsPerFrame.Apply(static_cast<float>(nowMs - _lastPacketPerFrameUpdateT),
                           nPackets);
    _lastPacketPerFrameUpdateT = nowMs;
}

void
VCMLossProtectionLogic::UpdatePacketsPerFrameKey(float nPackets, int64_t nowMs)
{
    _packetsPerFrameKey.Apply(static_cast<float>(nowMs -
                              _lastPacketPerFrameUpdateTKey), nPackets);
    _lastPacketPerFrameUpdateTKey = nowMs;
}

void
VCMLossProtectionLogic::UpdateKeyFrameSize(float keyFrameSize)
{
    _keyFrameSize = keyFrameSize;
}

void
VCMLossProtectionLogic::UpdateFrameSize(uint16_t width,
                                        uint16_t height)
{
    _codecWidth = width;
    _codecHeight = height;
}

void VCMLossProtectionLogic::UpdateNumLayers(int numLayers) {
  _numLayers = (numLayers == 0) ? 1 : numLayers;
}

bool
VCMLossProtectionLogic::UpdateMethod()
{
    if (_selectedMethod == NULL)
    {
        return false;
    }
    _currentParameters.rtt = _rtt;
    _currentParameters.lossPr = _lossPr;
    _currentParameters.bitRate = _bitRate;
    _currentParameters.frameRate = _frameRate; 
    _currentParameters.keyFrameSize = _keyFrameSize;
    _currentParameters.fecRateDelta = _fecRateDelta;
    _currentParameters.fecRateKey = _fecRateKey;
    _currentParameters.packetsPerFrame = _packetsPerFrame.Value();
    _currentParameters.packetsPerFrameKey = _packetsPerFrameKey.Value();
    _currentParameters.residualPacketLossFec = _residualPacketLossFec;
    _currentParameters.codecWidth = _codecWidth;
    _currentParameters.codecHeight = _codecHeight;
    _currentParameters.numLayers = _numLayers;
    return _selectedMethod->UpdateParameters(&_currentParameters);
}

VCMProtectionMethod*
VCMLossProtectionLogic::SelectedMethod() const
{
    return _selectedMethod;
}

VCMProtectionMethodEnum
VCMLossProtectionLogic::SelectedType() const
{
    return _selectedMethod->Type();
}

void
VCMLossProtectionLogic::Reset(int64_t nowMs)
{
    _lastPrUpdateT = nowMs;
    _lastPacketPerFrameUpdateT = nowMs;
    _lastPacketPerFrameUpdateTKey = nowMs;
    _lossPr255.Reset(0.9999f);
    _packetsPerFrame.Reset(0.9999f);
    _fecRateDelta = _fecRateKey = 0;
    for (int32_t i = 0; i < kLossPrHistorySize; i++)
    {
        _lossPrHistory[i].lossPr255 = 0;
        _lossPrHistory[i].timeMs = -1;
    }
    _shortMaxLossPr255 = 0;
    Release();
}

void
VCMLossProtectionLogic::Release()
{
    delete _selectedMethod;
    _selectedMethod = NULL;
}

}  
}  
