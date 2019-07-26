









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTCP_UTILITY_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTCP_UTILITY_H_

#include <stddef.h> 

#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_rtcp_config.h"
#include "webrtc/typedefs.h"

namespace webrtc {
namespace RTCPUtility {
    
    struct RTCPCnameInformation
    {
        char name[RTCP_CNAME_SIZE];
    };
    struct RTCPPacketRR
    {
        uint32_t SenderSSRC;
        uint8_t  NumberOfReportBlocks;
    };
    struct RTCPPacketSR
    {
        uint32_t SenderSSRC;
        uint8_t  NumberOfReportBlocks;

        
        uint32_t NTPMostSignificant;
        uint32_t NTPLeastSignificant;
        uint32_t RTPTimestamp;
        uint32_t SenderPacketCount;
        uint32_t SenderOctetCount;
    };
    struct RTCPPacketReportBlockItem
    {
        
        uint32_t SSRC;
        uint8_t  FractionLost;
        uint32_t CumulativeNumOfPacketsLost;
        uint32_t ExtendedHighestSequenceNumber;
        uint32_t Jitter;
        uint32_t LastSR;
        uint32_t DelayLastSR;
    };
    struct RTCPPacketSDESCName
    {
        
        uint32_t SenderSSRC;
        char CName[RTCP_CNAME_SIZE];
    };

    struct RTCPPacketExtendedJitterReportItem
    {
        
        uint32_t Jitter;
    };

    struct RTCPPacketBYE
    {
        uint32_t SenderSSRC;
    };
    struct RTCPPacketXR
    {
        
        uint32_t OriginatorSSRC;
    };
    struct RTCPPacketXRVOIPMetricItem
    {
        
        uint32_t    SSRC;
        uint8_t     lossRate;
        uint8_t     discardRate;
        uint8_t     burstDensity;
        uint8_t     gapDensity;
        uint16_t    burstDuration;
        uint16_t    gapDuration;
        uint16_t    roundTripDelay;
        uint16_t    endSystemDelay;
        uint8_t     signalLevel;
        uint8_t     noiseLevel;
        uint8_t     RERL;
        uint8_t     Gmin;
        uint8_t     Rfactor;
        uint8_t     extRfactor;
        uint8_t     MOSLQ;
        uint8_t     MOSCQ;
        uint8_t     RXconfig;
        uint16_t    JBnominal;
        uint16_t    JBmax;
        uint16_t    JBabsMax;
    };

    struct RTCPPacketRTPFBNACK
    {
        uint32_t SenderSSRC;
        uint32_t MediaSSRC;
    };
    struct RTCPPacketRTPFBNACKItem
    {
        
        uint16_t PacketID;
        uint16_t BitMask;
    };

    struct RTCPPacketRTPFBTMMBR
    {
        uint32_t SenderSSRC;
        uint32_t MediaSSRC; 
    };
    struct RTCPPacketRTPFBTMMBRItem
    {
        
        uint32_t SSRC;
        uint32_t MaxTotalMediaBitRate; 
        uint32_t MeasuredOverhead;
    };

    struct RTCPPacketRTPFBTMMBN
    {
        uint32_t SenderSSRC;
        uint32_t MediaSSRC; 
    };
    struct RTCPPacketRTPFBTMMBNItem
    {
        
        uint32_t SSRC; 
        uint32_t MaxTotalMediaBitRate;
        uint32_t MeasuredOverhead;
    };

    struct RTCPPacketPSFBFIR
    {
        uint32_t SenderSSRC;
        uint32_t MediaSSRC; 
    };
    struct RTCPPacketPSFBFIRItem
    {
        
        uint32_t SSRC;
        uint8_t  CommandSequenceNumber;
    };

    struct RTCPPacketPSFBPLI
    {
        
        uint32_t SenderSSRC;
        uint32_t MediaSSRC;
    };

    struct RTCPPacketPSFBSLI
    {
        
        uint32_t SenderSSRC;
        uint32_t MediaSSRC;
    };
    struct RTCPPacketPSFBSLIItem
    {
        
        uint16_t FirstMB;
        uint16_t NumberOfMB;
        uint8_t PictureId;
    };
    struct RTCPPacketPSFBRPSI
    {
        
        uint32_t SenderSSRC;
        uint32_t MediaSSRC;
        uint8_t  PayloadType;
        uint16_t NumberOfValidBits;
        uint8_t  NativeBitString[RTCP_RPSI_DATA_SIZE];
    };
    struct RTCPPacketPSFBAPP
    {
        uint32_t SenderSSRC;
        uint32_t MediaSSRC;
    };
    struct RTCPPacketPSFBREMBItem
    {
        uint32_t BitRate;
        uint8_t NumberOfSSRCs;
        uint32_t SSRCs[MAX_NUMBER_OF_REMB_FEEDBACK_SSRCS];
    };
    
    struct RTCPPacketAPP
    {
        uint8_t     SubType;
        uint32_t    Name;
        uint8_t     Data[kRtcpAppCode_DATA_SIZE];
        uint16_t    Size;
    };

    union RTCPPacket
    {
        RTCPPacketRR              RR;
        RTCPPacketSR              SR;
        RTCPPacketReportBlockItem ReportBlockItem;

        RTCPPacketSDESCName       CName;
        RTCPPacketBYE             BYE;

        RTCPPacketExtendedJitterReportItem ExtendedJitterReportItem;

        RTCPPacketRTPFBNACK       NACK;
        RTCPPacketRTPFBNACKItem   NACKItem;

        RTCPPacketPSFBPLI         PLI;
        RTCPPacketPSFBSLI         SLI;
        RTCPPacketPSFBSLIItem     SLIItem;
        RTCPPacketPSFBRPSI        RPSI;
        RTCPPacketPSFBAPP         PSFBAPP;
        RTCPPacketPSFBREMBItem    REMBItem;

        RTCPPacketRTPFBTMMBR      TMMBR;
        RTCPPacketRTPFBTMMBRItem  TMMBRItem;
        RTCPPacketRTPFBTMMBN      TMMBN;
        RTCPPacketRTPFBTMMBNItem  TMMBNItem;
        RTCPPacketPSFBFIR         FIR;
        RTCPPacketPSFBFIRItem     FIRItem;

        RTCPPacketXR               XR;
        RTCPPacketXRVOIPMetricItem XRVOIPMetricItem;

        RTCPPacketAPP             APP;
    };

    enum RTCPPacketTypes
    {
        kRtcpNotValidCode,

        
        kRtcpRrCode,
        kRtcpSrCode,
        kRtcpReportBlockItemCode,

        kRtcpSdesCode,
        kRtcpSdesChunkCode,
        kRtcpByeCode,

        
        kRtcpExtendedIjCode,
        kRtcpExtendedIjItemCode,

        
        kRtcpRtpfbNackCode,
        kRtcpRtpfbNackItemCode,

        kRtcpPsfbPliCode,
        kRtcpPsfbRpsiCode,
        kRtcpPsfbSliCode,
        kRtcpPsfbSliItemCode,
        kRtcpPsfbAppCode,
        kRtcpPsfbRembCode,
        kRtcpPsfbRembItemCode,

        
        kRtcpRtpfbTmmbrCode,
        kRtcpRtpfbTmmbrItemCode,
        kRtcpRtpfbTmmbnCode,
        kRtcpRtpfbTmmbnItemCode,
        kRtcpPsfbFirCode,
        kRtcpPsfbFirItemCode,

        
        kRtcpRtpfbSrReqCode,

        
        kRtcpXrVoipMetricCode,

        kRtcpAppCode,
        kRtcpAppItemCode,
    };

    struct RTCPRawPacket
    {
        const uint8_t* _ptrPacketBegin;
        const uint8_t* _ptrPacketEnd;
    };

    struct RTCPModRawPacket
    {
        uint8_t* _ptrPacketBegin;
        uint8_t* _ptrPacketEnd;
    };

    struct RTCPCommonHeader
    {
        uint8_t  V;  
        bool           P;  
        uint8_t  IC; 
        uint8_t  PT; 
        uint16_t LengthInOctets;
    };

    enum RTCPPT
    {
        PT_IJ    = 195,
        PT_SR    = 200,
        PT_RR    = 201,
        PT_SDES  = 202,
        PT_BYE   = 203,
        PT_APP   = 204,
        PT_RTPFB = 205,
        PT_PSFB  = 206,
        PT_XR    = 207
    };

    bool RTCPParseCommonHeader( const uint8_t* ptrDataBegin,
                                const uint8_t* ptrDataEnd,
                                RTCPCommonHeader& parsedHeader);

    class RTCPParserV2
    {
    public:
        RTCPParserV2(const uint8_t* rtcpData,
                     size_t rtcpDataLength,
                     bool rtcpReducedSizeEnable); 
        ~RTCPParserV2();

        RTCPPacketTypes PacketType() const;
        const RTCPPacket& Packet() const;
        const RTCPRawPacket& RawPacket() const;
        ptrdiff_t LengthLeft() const;

        bool IsValid() const;

        RTCPPacketTypes Begin();
        RTCPPacketTypes Iterate();

    private:
        enum ParseState
        {
            State_TopLevel,        
            State_ReportBlockItem, 
            State_SDESChunk,       
            State_BYEItem,         
            State_ExtendedJitterItem, 
            State_RTPFB_NACKItem,  
            State_RTPFB_TMMBRItem, 
            State_RTPFB_TMMBNItem, 
            State_PSFB_SLIItem,    
            State_PSFB_RPSIItem,   
            State_PSFB_FIRItem,    
            State_PSFB_AppItem,    
            State_PSFB_REMBItem,   
            State_XRItem,
            State_AppItem
        };

    private:
        void IterateTopLevel();
        void IterateReportBlockItem();
        void IterateSDESChunk();
        void IterateBYEItem();
        void IterateExtendedJitterItem();
        void IterateNACKItem();
        void IterateTMMBRItem();
        void IterateTMMBNItem();
        void IterateSLIItem();
        void IterateRPSIItem();
        void IterateFIRItem();
        void IteratePsfbAppItem();
        void IteratePsfbREMBItem();
        void IterateAppItem();

        void Validate();
        void EndCurrentBlock();

        bool ParseRR();
        bool ParseSR();
        bool ParseReportBlockItem();

        bool ParseSDES();
        bool ParseSDESChunk();
        bool ParseSDESItem();

        bool ParseBYE();
        bool ParseBYEItem();

        bool ParseIJ();
        bool ParseIJItem();

        bool ParseXR();
        bool ParseXRItem();
        bool ParseXRVOIPMetricItem();

        bool ParseFBCommon(const RTCPCommonHeader& header);
        bool ParseNACKItem();
        bool ParseTMMBRItem();
        bool ParseTMMBNItem();
        bool ParseSLIItem();
        bool ParseRPSIItem();
        bool ParseFIRItem();
        bool ParsePsfbAppItem();
        bool ParsePsfbREMBItem();

        bool ParseAPP(const RTCPCommonHeader& header);
        bool ParseAPPItem();

    private:
        const uint8_t* const _ptrRTCPDataBegin;
        const bool                 _RTCPReducedSizeEnable;
        const uint8_t* const _ptrRTCPDataEnd;

        bool                     _validPacket;
        const uint8_t*     _ptrRTCPData;
        const uint8_t*     _ptrRTCPBlockEnd;

        ParseState               _state;
        uint8_t            _numberOfBlocks;

        RTCPPacketTypes          _packetType;
        RTCPPacket               _packet;
    };

    class RTCPPacketIterator
    {
    public:
        RTCPPacketIterator(uint8_t* rtcpData,
                            size_t rtcpDataLength);
        ~RTCPPacketIterator();

        const RTCPCommonHeader* Begin();
        const RTCPCommonHeader* Iterate();
        const RTCPCommonHeader* Current();

    private:
        uint8_t* const     _ptrBegin;
        uint8_t* const     _ptrEnd;

        uint8_t*           _ptrBlock;

        RTCPCommonHeader         _header;
    };
}  
}  
#endif
