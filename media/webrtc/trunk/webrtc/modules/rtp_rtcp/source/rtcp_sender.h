









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTCP_SENDER_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTCP_SENDER_H_

#include <map>
#include <sstream>
#include <string>

#include "typedefs.h"
#include "rtcp_utility.h"
#include "rtp_utility.h"
#include "rtp_rtcp_defines.h"
#include "scoped_ptr.h"
#include "tmmbr_help.h"
#include "modules/remote_bitrate_estimator/include/bwe_defines.h"
#include "modules/remote_bitrate_estimator/include/remote_bitrate_estimator.h"

namespace webrtc {

class ModuleRtpRtcpImpl; 

class NACKStringBuilder
{
public:
    NACKStringBuilder();
    void PushNACK(uint16_t nack);
    std::string GetResult();

private:
    std::ostringstream _stream;
    int _count;
    uint16_t _prevNack;
    bool _consecutive;
};

class RTCPSender
{
public:
    RTCPSender(const int32_t id, const bool audio,
               Clock* clock, ModuleRtpRtcpImpl* owner);
    virtual ~RTCPSender();

    void ChangeUniqueId(const int32_t id);

    int32_t Init();

    int32_t RegisterSendTransport(Transport* outgoingTransport);

    RTCPMethod Status() const;
    int32_t SetRTCPStatus(const RTCPMethod method);

    bool Sending() const;
    int32_t SetSendingStatus(const bool enabled); 

    int32_t SetNackStatus(const bool enable);

    void SetStartTimestamp(uint32_t start_timestamp);

    void SetLastRtpTime(uint32_t rtp_timestamp,
                        int64_t capture_time_ms);

    void SetSSRC( const uint32_t ssrc);

    int32_t SetRemoteSSRC( const uint32_t ssrc);

    int32_t SetCameraDelay(const int32_t delayMS);

    int32_t CNAME(char cName[RTCP_CNAME_SIZE]);
    int32_t SetCNAME(const char cName[RTCP_CNAME_SIZE]);

    int32_t AddMixedCNAME(const uint32_t SSRC,
                          const char cName[RTCP_CNAME_SIZE]);

    int32_t RemoveMixedCNAME(const uint32_t SSRC);

    uint32_t SendTimeOfSendReport(const uint32_t sendReport);

    bool TimeToSendRTCPReport(const bool sendKeyframeBeforeRTP = false) const;

    uint32_t LastSendReport(uint32_t& lastRTCPTime);

    int32_t SendRTCP(const uint32_t rtcpPacketTypeFlags,
                     const int32_t nackSize = 0,
                     const uint16_t* nackList = 0,
                     const bool repeat = false,
                     const uint64_t pictureID = 0);

    int32_t AddReportBlock(const uint32_t SSRC,
                           const RTCPReportBlock* receiveBlock);

    int32_t RemoveReportBlock(const uint32_t SSRC);

    


    bool REMB() const;

    int32_t SetREMBStatus(const bool enable);

    int32_t SetREMBData(const uint32_t bitrate,
                        const uint8_t numberOfSSRC,
                        const uint32_t* SSRC);

    


    bool TMMBR() const;

    int32_t SetTMMBRStatus(const bool enable);

    int32_t SetTMMBN(const TMMBRSet* boundingSet,
                     const uint32_t maxBitrateKbit);

    


    bool IJ() const;

    int32_t SetIJStatus(const bool enable);

    



    int32_t SetApplicationSpecificData(const uint8_t subType,
                                       const uint32_t name,
                                       const uint8_t* data,
                                       const uint16_t length);

    int32_t SetRTCPVoIPMetrics(const RTCPVoIPMetric* VoIPMetric);

    int32_t SetCSRCs(const uint32_t arrOfCSRC[kRtpCsrcSize],
                     const uint8_t arrLength);

    int32_t SetCSRCStatus(const bool include);

    void SetTargetBitrate(unsigned int target_bitrate);

private:
    int32_t SendToNetwork(const uint8_t* dataBuffer, const uint16_t length);

    void UpdatePacketRate();

    int32_t AddReportBlocks(uint8_t* rtcpbuffer,
                            uint32_t& pos,
                            uint8_t& numberOfReportBlocks,
                            const RTCPReportBlock* received,
                            const uint32_t NTPsec,
                            const uint32_t NTPfrac);

    int32_t BuildSR(uint8_t* rtcpbuffer,
                    uint32_t& pos,
                    const uint32_t NTPsec,
                    const uint32_t NTPfrac,
                    const RTCPReportBlock* received = NULL);

    int32_t BuildRR(uint8_t* rtcpbuffer,
                    uint32_t& pos,
                    const uint32_t NTPsec,
                    const uint32_t NTPfrac,
                    const RTCPReportBlock* received = NULL);

    int32_t BuildExtendedJitterReport(
        uint8_t* rtcpbuffer,
        uint32_t& pos,
        const uint32_t jitterTransmissionTimeOffset);

    int32_t BuildSDEC(uint8_t* rtcpbuffer, uint32_t& pos);
    int32_t BuildPLI(uint8_t* rtcpbuffer, uint32_t& pos);
    int32_t BuildREMB(uint8_t* rtcpbuffer, uint32_t& pos);
    int32_t BuildTMMBR(uint8_t* rtcpbuffer, uint32_t& pos);
    int32_t BuildTMMBN(uint8_t* rtcpbuffer, uint32_t& pos);
    int32_t BuildAPP(uint8_t* rtcpbuffer, uint32_t& pos);
    int32_t BuildVoIPMetric(uint8_t* rtcpbuffer, uint32_t& pos);
    int32_t BuildBYE(uint8_t* rtcpbuffer, uint32_t& pos);
    int32_t BuildFIR(uint8_t* rtcpbuffer, uint32_t& pos, bool repeat);
    int32_t BuildSLI(uint8_t* rtcpbuffer,
                     uint32_t& pos,
                     const uint8_t pictureID);
    int32_t BuildRPSI(uint8_t* rtcpbuffer,
                      uint32_t& pos,
                      const uint64_t pictureID,
                      const uint8_t payloadType);

    int32_t BuildNACK(uint8_t* rtcpbuffer,
                      uint32_t& pos,
                      const int32_t nackSize,
                      const uint16_t* nackList,
                          std::string* nackString);

private:
    int32_t            _id;
    const bool               _audio;
    Clock*                   _clock;
    RTCPMethod               _method;

    ModuleRtpRtcpImpl&      _rtpRtcp;

    CriticalSectionWrapper* _criticalSectionTransport;
    Transport*              _cbTransport;

    CriticalSectionWrapper* _criticalSectionRTCPSender;
    bool                    _usingNack;
    bool                    _sending;
    bool                    _sendTMMBN;
    bool                    _REMB;
    bool                    _sendREMB;
    bool                    _TMMBR;
    bool                    _IJ;

    int64_t        _nextTimeToSendRTCP;

    uint32_t start_timestamp_;
    uint32_t last_rtp_timestamp_;
    int64_t last_frame_capture_time_ms_;
    uint32_t _SSRC;
    uint32_t _remoteSSRC;  
    char _CNAME[RTCP_CNAME_SIZE];

    std::map<uint32_t, RTCPReportBlock*> _reportBlocks;
    std::map<uint32_t, RTCPUtility::RTCPCnameInformation*> _csrcCNAMEs;

    int32_t         _cameraDelayMS;

    
    uint32_t        _lastSendReport[RTCP_NUMBER_OF_SR];  
    uint32_t        _lastRTCPTime[RTCP_NUMBER_OF_SR];

    
    uint8_t         _CSRCs;
    uint32_t        _CSRC[kRtpCsrcSize];
    bool                _includeCSRCs;

    
    uint8_t         _sequenceNumberFIR;

    
    uint8_t       _lengthRembSSRC;
    uint8_t       _sizeRembSSRC;
    uint32_t*     _rembSSRC;
    uint32_t      _rembBitrate;

    TMMBRHelp           _tmmbrHelp;
    uint32_t      _tmmbr_Send;
    uint32_t      _packetOH_Send;

    
    bool                 _appSend;
    uint8_t        _appSubType;
    uint32_t       _appName;
    uint8_t*       _appData;
    uint16_t       _appLength;

    
    bool                _xrSendVoIPMetric;
    RTCPVoIPMetric      _xrVoIPMetric;

    
    uint32_t      _nackCount;
    uint32_t      _pliCount;
    uint32_t      _fullIntraRequestCount;
};
} 

#endif 
