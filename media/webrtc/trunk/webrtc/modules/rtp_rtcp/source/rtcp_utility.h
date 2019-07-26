









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTCP_UTILITY_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTCP_UTILITY_H_

#include <cstddef> 

#include "typedefs.h"
#include "rtp_rtcp_config.h"
#include "rtp_rtcp_defines.h"

namespace webrtc {
namespace RTCPUtility {
    
    struct RTCPCnameInformation
    {
        char name[RTCP_CNAME_SIZE];
    };
    struct RTCPPacketRR
    {
        WebRtc_UWord32 SenderSSRC;
        WebRtc_UWord8  NumberOfReportBlocks;
    };
    struct RTCPPacketSR
    {
        WebRtc_UWord32 SenderSSRC;
        WebRtc_UWord8  NumberOfReportBlocks;

        
        WebRtc_UWord32 NTPMostSignificant;
        WebRtc_UWord32 NTPLeastSignificant;
        WebRtc_UWord32 RTPTimestamp;
        WebRtc_UWord32 SenderPacketCount;
        WebRtc_UWord32 SenderOctetCount;
    };
    struct RTCPPacketReportBlockItem
    {
        
        WebRtc_UWord32 SSRC;
        WebRtc_UWord8  FractionLost;
        WebRtc_UWord32 CumulativeNumOfPacketsLost;
        WebRtc_UWord32 ExtendedHighestSequenceNumber;
        WebRtc_UWord32 Jitter;
        WebRtc_UWord32 LastSR;
        WebRtc_UWord32 DelayLastSR;
    };
    struct RTCPPacketSDESCName
    {
        
        WebRtc_UWord32 SenderSSRC;
        char CName[RTCP_CNAME_SIZE];
    };

    struct RTCPPacketExtendedJitterReportItem
    {
        
        WebRtc_UWord32 Jitter;
    };

    struct RTCPPacketBYE
    {
        WebRtc_UWord32 SenderSSRC;
    };
    struct RTCPPacketXR
    {
        
        WebRtc_UWord32 OriginatorSSRC;
    };
    struct RTCPPacketXRVOIPMetricItem
    {
        
        WebRtc_UWord32    SSRC;
        WebRtc_UWord8     lossRate;
        WebRtc_UWord8     discardRate;
        WebRtc_UWord8     burstDensity;
        WebRtc_UWord8     gapDensity;
        WebRtc_UWord16    burstDuration;
        WebRtc_UWord16    gapDuration;
        WebRtc_UWord16    roundTripDelay;
        WebRtc_UWord16    endSystemDelay;
        WebRtc_UWord8     signalLevel;
        WebRtc_UWord8     noiseLevel;
        WebRtc_UWord8     RERL;
        WebRtc_UWord8     Gmin;
        WebRtc_UWord8     Rfactor;
        WebRtc_UWord8     extRfactor;
        WebRtc_UWord8     MOSLQ;
        WebRtc_UWord8     MOSCQ;
        WebRtc_UWord8     RXconfig;
        WebRtc_UWord16    JBnominal;
        WebRtc_UWord16    JBmax;
        WebRtc_UWord16    JBabsMax;
    };

    struct RTCPPacketRTPFBNACK
    {
        WebRtc_UWord32 SenderSSRC;
        WebRtc_UWord32 MediaSSRC;
    };
    struct RTCPPacketRTPFBNACKItem
    {
        
        WebRtc_UWord16 PacketID;
        WebRtc_UWord16 BitMask;
    };

    struct RTCPPacketRTPFBTMMBR
    {
        WebRtc_UWord32 SenderSSRC;
        WebRtc_UWord32 MediaSSRC; 
    };
    struct RTCPPacketRTPFBTMMBRItem
    {
        
        WebRtc_UWord32 SSRC;
        WebRtc_UWord32 MaxTotalMediaBitRate; 
        WebRtc_UWord32 MeasuredOverhead;
    };

    struct RTCPPacketRTPFBTMMBN
    {
        WebRtc_UWord32 SenderSSRC;
        WebRtc_UWord32 MediaSSRC; 
    };
    struct RTCPPacketRTPFBTMMBNItem
    {
        
        WebRtc_UWord32 SSRC; 
        WebRtc_UWord32 MaxTotalMediaBitRate;
        WebRtc_UWord32 MeasuredOverhead;
    };

    struct RTCPPacketPSFBFIR
    {
        WebRtc_UWord32 SenderSSRC;
        WebRtc_UWord32 MediaSSRC; 
    };
    struct RTCPPacketPSFBFIRItem
    {
        
        WebRtc_UWord32 SSRC;
        WebRtc_UWord8  CommandSequenceNumber;
    };

    struct RTCPPacketPSFBPLI
    {
        
        WebRtc_UWord32 SenderSSRC;
        WebRtc_UWord32 MediaSSRC;
    };

    struct RTCPPacketPSFBSLI
    {
        
        WebRtc_UWord32 SenderSSRC;
        WebRtc_UWord32 MediaSSRC;
    };
    struct RTCPPacketPSFBSLIItem
    {
        
        WebRtc_UWord16 FirstMB;
        WebRtc_UWord16 NumberOfMB;
        WebRtc_UWord8 PictureId;
    };
    struct RTCPPacketPSFBRPSI
    {
        
        WebRtc_UWord32 SenderSSRC;
        WebRtc_UWord32 MediaSSRC;
        WebRtc_UWord8  PayloadType;
        WebRtc_UWord16 NumberOfValidBits;
        WebRtc_UWord8  NativeBitString[RTCP_RPSI_DATA_SIZE];
    };
    struct RTCPPacketPSFBAPP
    {
        WebRtc_UWord32 SenderSSRC;
        WebRtc_UWord32 MediaSSRC;
    };
    struct RTCPPacketPSFBREMBItem
    {
        WebRtc_UWord32 BitRate;
        WebRtc_UWord8 NumberOfSSRCs;
        WebRtc_UWord32 SSRCs[MAX_NUMBER_OF_REMB_FEEDBACK_SSRCS];
    };
    
    struct RTCPPacketAPP
    {
        WebRtc_UWord8     SubType;
        WebRtc_UWord32    Name;
        WebRtc_UWord8     Data[kRtcpAppCode_DATA_SIZE];
        WebRtc_UWord16    Size;
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
        const WebRtc_UWord8* _ptrPacketBegin;
        const WebRtc_UWord8* _ptrPacketEnd;
    };

    struct RTCPModRawPacket
    {
        WebRtc_UWord8* _ptrPacketBegin;
        WebRtc_UWord8* _ptrPacketEnd;
    };

    struct RTCPCommonHeader
    {
        WebRtc_UWord8  V;  
        bool           P;  
        WebRtc_UWord8  IC; 
        WebRtc_UWord8  PT; 
        WebRtc_UWord16 LengthInOctets;
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

    bool RTCPParseCommonHeader( const WebRtc_UWord8* ptrDataBegin,
                                const WebRtc_UWord8* ptrDataEnd,
                                RTCPCommonHeader& parsedHeader);

    class RTCPParserV2
    {
    public:
        RTCPParserV2(const WebRtc_UWord8* rtcpData,
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
        const WebRtc_UWord8* const _ptrRTCPDataBegin;
        const bool                 _RTCPReducedSizeEnable;
        const WebRtc_UWord8* const _ptrRTCPDataEnd;

        bool                     _validPacket;
        const WebRtc_UWord8*     _ptrRTCPData;
        const WebRtc_UWord8*     _ptrRTCPBlockEnd;

        ParseState               _state;
        WebRtc_UWord8            _numberOfBlocks;

        RTCPPacketTypes          _packetType;
        RTCPPacket               _packet;
    };

    class RTCPPacketIterator
    {
    public:
        RTCPPacketIterator(WebRtc_UWord8* rtcpData,
                            size_t rtcpDataLength);
        ~RTCPPacketIterator();

        const RTCPCommonHeader* Begin();
        const RTCPCommonHeader* Iterate();
        const RTCPCommonHeader* Current();

    private:
        WebRtc_UWord8* const     _ptrBegin;
        WebRtc_UWord8* const     _ptrEnd;

        WebRtc_UWord8*           _ptrBlock;

        RTCPCommonHeader         _header;
    };
} 
} 
#endif
