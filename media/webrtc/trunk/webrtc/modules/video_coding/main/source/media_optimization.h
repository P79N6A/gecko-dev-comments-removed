









#ifndef WEBRTC_MODULES_VIDEO_CODING_MEDIA_OPTIMIZATION_H_
#define WEBRTC_MODULES_VIDEO_CODING_MEDIA_OPTIMIZATION_H_

#include "module_common_types.h"
#include "video_coding.h"
#include "trace.h"
#include "media_opt_util.h"
#include "qm_select.h"

#include <list>

namespace webrtc {

class Clock;
class FrameDropper;
class VCMContentMetricsProcessing;

namespace media_optimization {

enum { kBitrateMaxFrameSamples = 60 };
enum { kBitrateAverageWinMs    = 1000 };

struct VCMEncodedFrameSample {
  VCMEncodedFrameSample(int size_bytes, uint32_t timestamp,
                        int64_t time_complete_ms)
      : size_bytes(size_bytes),
        timestamp(timestamp),
        time_complete_ms(time_complete_ms) {}

  uint32_t size_bytes;
  uint32_t timestamp;
  int64_t time_complete_ms;
};

class VCMMediaOptimization
{
public:
    VCMMediaOptimization(int32_t id, Clock* clock);
    ~VCMMediaOptimization(void);
    


    int32_t Reset();
    







    uint32_t SetTargetRates(uint32_t target_bitrate,
                                  uint8_t &fractionLost,
                                  uint32_t roundTripTimeMs);

    


    int32_t SetEncodingData(VideoCodecType sendCodecType,
                                  int32_t maxBitRate,
                                  uint32_t frameRate,
                                  uint32_t bitRate,
                                  uint16_t width,
                                  uint16_t height,
                                  int numTemporalLayers);
    


    void EnableProtectionMethod(bool enable, VCMProtectionMethodEnum method);
    


    bool IsProtectionMethodEnabled(VCMProtectionMethodEnum method);
    


    void SetMtu(int32_t mtu);
    


    uint32_t InputFrameRate();

    


    uint32_t SentFrameRate();
    


    uint32_t SentBitRate();
    


    int32_t MaxBitRate();
    


    int32_t UpdateWithEncodedData(int encodedLength,
                                        uint32_t timestamp,
                                        FrameType encodedFrameType);
    



    int32_t RegisterProtectionCallback(VCMProtectionCallback*
                                             protectionCallback);
    


    int32_t RegisterVideoQMCallback(VCMQMSettingsCallback* videoQMSettings);
    void EnableFrameDropper(bool enable);

    bool DropFrame();

      


    int32_t SentFrameCount(VCMFrameCount &frameCount) const;

    


    void UpdateIncomingFrameRate();

    


    void UpdateContentData(const VideoContentMetrics* contentMetrics);

    


    int32_t SelectQuality();

private:
    typedef std::list<VCMEncodedFrameSample> FrameSampleList;

    


    int UpdateProtectionCallback(VCMProtectionMethod *selected_method,
                                 uint32_t* total_video_rate_bps,
                                 uint32_t* nack_overhead_rate_bps,
                                 uint32_t* fec_overhead_rate_bps);

    void PurgeOldFrameSamples(int64_t now_ms);
    void UpdateSentBitrate(int64_t nowMs);
    void UpdateSentFramerate();

    



    bool QMUpdate(VCMResolutionScale* qm);
    



    bool CheckStatusForQMchange();

    void ProcessIncomingFrameRate(int64_t now);

    enum { kFrameCountHistorySize = 90};
    enum { kFrameHistoryWinMs = 2000};

    int32_t                     _id;
    Clock*                            _clock;
    int32_t                     _maxBitRate;
    VideoCodecType                    _sendCodecType;
    uint16_t                    _codecWidth;
    uint16_t                    _codecHeight;
    float                             _userFrameRate;

    FrameDropper*                     _frameDropper;
    VCMLossProtectionLogic*           _lossProtLogic;
    uint8_t                     _fractionLost;


    uint32_t                    _sendStatistics[4];
    uint32_t                    _sendStatisticsZeroEncode;
    int32_t                     _maxPayloadSize;
    uint32_t                    _targetBitRate;

    float                             _incomingFrameRate;
    int64_t                     _incomingFrameTimes[kFrameCountHistorySize];

    bool                              _enableQm;

    VCMProtectionCallback*            _videoProtectionCallback;
    VCMQMSettingsCallback*            _videoQMSettingsCallback;

    std::list<VCMEncodedFrameSample>  _encodedFrameSamples;
    uint32_t                          _avgSentBitRateBps;
    uint32_t                          _avgSentFramerate;

    uint32_t                    _keyFrameCnt;
    uint32_t                    _deltaFrameCnt;

    VCMContentMetricsProcessing*      _content;
    VCMQmResolution*                  _qmResolution;

    int64_t                     _lastQMUpdateTime;
    int64_t                     _lastChangeTime; 
    int                               _numLayers;


}; 

}  
}  

#endif 
