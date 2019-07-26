









#ifndef WEBRTC_MODULES_VIDEO_CODING_MEDIA_OPTIMIZATION_H_
#define WEBRTC_MODULES_VIDEO_CODING_MEDIA_OPTIMIZATION_H_

#include "module_common_types.h"
#include "video_coding.h"
#include "trace.h"
#include "media_opt_util.h"
#include "qm_select.h"

namespace webrtc
{

enum { kBitrateMaxFrameSamples = 60 };
enum { kBitrateAverageWinMs    = 1000 };

class TickTimeBase;
class VCMContentMetricsProcessing;
class VCMFrameDropper;

struct VCMEncodedFrameSample
{
    VCMEncodedFrameSample() : _sizeBytes(-1), _timeCompleteMs(-1) {}

    WebRtc_Word64     _sizeBytes;
    WebRtc_Word64     _timeCompleteMs;
};

class VCMMediaOptimization
{
public:
    VCMMediaOptimization(WebRtc_Word32 id, TickTimeBase* clock);
    ~VCMMediaOptimization(void);
    


    WebRtc_Word32 Reset();
    








    WebRtc_UWord32 SetTargetRates(WebRtc_UWord32 bitRate,
                                  WebRtc_UWord8 &fractionLost,
                                  WebRtc_UWord32 roundTripTimeMs);

    


    WebRtc_Word32 SetEncodingData(VideoCodecType sendCodecType,
                                  WebRtc_Word32 maxBitRate,
                                  WebRtc_UWord32 frameRate,
                                  WebRtc_UWord32 bitRate,
                                  WebRtc_UWord16 width,
                                  WebRtc_UWord16 height,
                                  int numTemporalLayers);
    


    void EnableProtectionMethod(bool enable, VCMProtectionMethodEnum method);
    


    bool IsProtectionMethodEnabled(VCMProtectionMethodEnum method);
    


    void SetMtu(WebRtc_Word32 mtu);
    


    WebRtc_UWord32 InputFrameRate();

    


    float SentFrameRate();
    


    float SentBitRate();
    


    WebRtc_Word32 MaxBitRate();
    


    WebRtc_Word32 UpdateWithEncodedData(WebRtc_Word32 encodedLength,
                                        FrameType encodedFrameType);
    



    WebRtc_Word32 RegisterProtectionCallback(VCMProtectionCallback*
                                             protectionCallback);
    


    WebRtc_Word32 RegisterVideoQMCallback(VCMQMSettingsCallback* videoQMSettings);
    void EnableFrameDropper(bool enable);

    bool DropFrame();

      


    WebRtc_Word32 SentFrameCount(VCMFrameCount &frameCount) const;

    


    void UpdateIncomingFrameRate();

    


    void updateContentData(const VideoContentMetrics* contentMetrics);

    


    WebRtc_Word32 SelectQuality();

private:

    


    int UpdateProtectionCallback(VCMProtectionMethod *selected_method,
                                 uint32_t* total_video_rate_bps,
                                 uint32_t* nack_overhead_rate_bps,
                                 uint32_t* fec_overhead_rate_bps);

    void UpdateBitRateEstimate(WebRtc_Word64 encodedLength, WebRtc_Word64 nowMs);
    



    bool QMUpdate(VCMResolutionScale* qm);
    



    bool checkStatusForQMchange();

    void ProcessIncomingFrameRate(WebRtc_Word64 now);

    enum { kFrameCountHistorySize = 90};
    enum { kFrameHistoryWinMs = 2000};

    WebRtc_Word32                     _id;
    TickTimeBase*                     _clock;
    WebRtc_Word32                     _maxBitRate;
    VideoCodecType                    _sendCodecType;
    WebRtc_UWord16                    _codecWidth;
    WebRtc_UWord16                    _codecHeight;
    float                             _userFrameRate;

    VCMFrameDropper*                  _frameDropper;
    VCMLossProtectionLogic*           _lossProtLogic;
    WebRtc_UWord8                     _fractionLost;


    WebRtc_UWord32                    _sendStatistics[4];
    WebRtc_UWord32                    _sendStatisticsZeroEncode;
    WebRtc_Word32                     _maxPayloadSize;
    WebRtc_UWord32                    _targetBitRate;

    float                             _incomingFrameRate;
    WebRtc_Word64                     _incomingFrameTimes[kFrameCountHistorySize];

    bool                              _enableQm;

    VCMProtectionCallback*            _videoProtectionCallback;
    VCMQMSettingsCallback*            _videoQMSettingsCallback;

    VCMEncodedFrameSample             _encodedFrameSamples[kBitrateMaxFrameSamples];
    float                             _avgSentBitRateBps;

    WebRtc_UWord32                    _keyFrameCnt;
    WebRtc_UWord32                    _deltaFrameCnt;

    VCMContentMetricsProcessing*      _content;
    VCMQmResolution*                  _qmResolution;

    WebRtc_Word64                     _lastQMUpdateTime;
    WebRtc_Word64                     _lastChangeTime; 
    int                               _numLayers;


}; 

} 

#endif 
