









#include <cassert>
#include <windows.h>
#include <iostream>
#include <tchar.h>

#include "rtp_rtcp.h"
#include "common_types.h"
#include "rtcp_utility.h"
#include "tmmbr_help.h"

#define TEST_STR "Test TMMBR."
#define TEST_PASSED() std::cerr << TEST_STR << " : [OK]" << std::endl
#define PRINT_LINE std::cout << "------------------------------------------" << std::endl;


const int maxFileLen = 200;
WebRtc_UWord8* dataFile[maxFileLen];


struct InputSet
{
    WebRtc_UWord32 TMMBR;
    WebRtc_UWord32 packetOH;
    WebRtc_UWord32 SSRC;
};

const InputSet set0   = {220,  80, 11111};  
const InputSet set1   = {180,  90, 22222};
const InputSet set2   = {100, 210, 33333};
const InputSet set3   = { 35,  40, 44444};
const InputSet set4   = { 40,  60, 55555};
const InputSet set4_1 = {100,  60, 55555};
const InputSet set4_2 = { 10,  60, 55555};
const InputSet set5   = {200,  40, 66666};
const InputSet set00  = {  0,  40, 66666};

const int maxBitrate = 230;  

void Verify(TMMBRSet* boundingSet, int index, InputSet set)
{
    assert(boundingSet->ptrTmmbrSet[index]    == set.TMMBR);
    assert(boundingSet->ptrPacketOHSet[index] == set.packetOH);
    assert(boundingSet->ptrSsrcSet[index]     == set.SSRC);
};

int ParseRTCPPacket(const void *data, int len, TMMBRSet*& boundingSet)
{
    int numItems = -1;
    RTCPUtility::RTCPParserV2 rtcpParser((const WebRtc_UWord8*)data, len, true);
    RTCPUtility::RTCPPacketTypes pktType = rtcpParser.Begin();
    while (pktType != RTCPUtility::kRtcpNotValidCode)
    {
        const RTCPUtility::RTCPPacket& rtcpPacket = rtcpParser.Packet();
        if (pktType == RTCPUtility::kRtcpRtpfbTmmbnCode)
        {
            assert(0 == rtcpPacket.TMMBN.SenderSSRC);
            assert(0 == rtcpPacket.TMMBN.MediaSSRC);
            numItems = 0;
        }
        if (pktType == RTCPUtility::kRtcpRtpfbTmmbnItemCode)
        {
            boundingSet->ptrTmmbrSet[numItems]    = rtcpPacket.TMMBNItem.MaxTotalMediaBitRate;
            boundingSet->ptrPacketOHSet[numItems] = rtcpPacket.TMMBNItem.MeasuredOverhead;
            boundingSet->ptrSsrcSet[numItems]     = rtcpPacket.TMMBNItem.SSRC;
            ++numItems;
        }
        pktType = rtcpParser.Iterate();
    }
    return numItems;
};

WebRtc_Word32 GetFile(char* fileName)
{
    if (!fileName[0])
    {
        return 0;
    }

    FILE* openFile = fopen(fileName, "rb");
    assert(openFile != NULL);
    fseek(openFile, 0, SEEK_END);
    int len = (WebRtc_Word16)(ftell(openFile));
    rewind(openFile);
    assert(len > 0 && len < maxFileLen);
    fread(dataFile, 1, len, openFile);
    fclose(openFile);
    return len;
};


class LoopBackTransport2 : public webrtc::Transport, private TMMBRHelp
{
public:
    LoopBackTransport2(RtpRtcp* rtpRtcpModule)  :
      TMMBRHelp(false),
      _rtpRtcpModule(rtpRtcpModule),
      _cnt(0)
    {
    }
    virtual int SendPacket(int channel, const void *data, int len)
    {
        if( 0  == _rtpRtcpModule->IncomingPacket((const WebRtc_UWord8*)data, len))
        {
            return len;
        }
        return -1;
    }
    virtual int SendRTCPPacket(int channel, const void *data, int len)
    {
        char fileName[256] = {0};
        TMMBRSet* boundingSet = BoundingSet();
        boundingSet->VerifyAndAllocateSet(3);

        if (_cnt == 0)
        {
            
            
            
            
            assert(2 == ParseRTCPPacket(data, len, boundingSet));
            Verify(boundingSet, 0, set4);
            Verify(boundingSet, 1, set2);

            strcpy(fileName, "RTCPPacketTMMBR3.bin");
        }

        ++_cnt;

        
        len = GetFile(fileName);
        if (len == 0)
        {
            return 1;
        }

        
        if(_rtpRtcpModule->IncomingPacket((const WebRtc_UWord8*)dataFile, len) == 0)
        {
            return len;
        }
        return -1;
    }
    RtpRtcp* _rtpRtcpModule;
    WebRtc_UWord32       _cnt;
};


class LoopBackTransportVideo : public webrtc::Transport, private TMMBRHelp
{
public:
    LoopBackTransportVideo(RtpRtcp* rtpRtcpModule)  :
      TMMBRHelp(false),
      _rtpRtcpModule(rtpRtcpModule),
      _cnt(0)
    {
    }
    virtual int SendPacket(int channel, const void *data, int len)
    {
        if(_rtpRtcpModule->IncomingPacket((const WebRtc_UWord8*)data, len)== 0)
        {
            return len;
        }
        return -1;
    }
    virtual int SendRTCPPacket(int channel, const void *data, int len)
    {
        char fileName[256] = {0};
        TMMBRSet* boundingSet = BoundingSet();
        boundingSet->VerifyAndAllocateSet(3);

        if (_cnt == 0)
        {
            strcpy(fileName, "RTCPPacketTMMBR0.bin");
        }
        else if (_cnt == 1)
        {
            
            assert(1 == ParseRTCPPacket(data, len, boundingSet));
            Verify(boundingSet, 0, set0);

            strcpy(fileName, "RTCPPacketTMMBR1.bin");
        }
        else if (_cnt == 2)
        {
            
            assert(1 == ParseRTCPPacket(data, len, boundingSet));
            Verify(boundingSet, 0, set1);

            strcpy(fileName, "RTCPPacketTMMBR2.bin");
        }
        else if (_cnt == 3)
        {
            
            assert(1 == ParseRTCPPacket(data, len, boundingSet));
            Verify(boundingSet, 0, set2);

            strcpy(fileName, "RTCPPacketTMMBR3.bin");
        }
        else if (_cnt == 4)
        {
            
            assert(2 == ParseRTCPPacket(data, len, boundingSet));
            Verify(boundingSet, 0, set3);
            Verify(boundingSet, 1, set2);

            strcpy(fileName, "RTCPPacketTMMBR4.bin");
        }
        else if (_cnt == 5)
        {
            
            assert(3 == ParseRTCPPacket(data, len, boundingSet));
            Verify(boundingSet, 0, set3);
            Verify(boundingSet, 1, set4);
            Verify(boundingSet, 2, set2);

            strcpy(fileName, "RTCPPacketTMMBR5.bin");
        }
        else if (_cnt == 6)
        {
            
            assert(3 == ParseRTCPPacket(data, len, boundingSet));
            Verify(boundingSet, 0, set3);
            Verify(boundingSet, 1, set4);
            Verify(boundingSet, 2, set2);

            strcpy(fileName, "RTCPPacketTMMBR4_2.bin");
        }
        else if (_cnt == 7)
        {
            
            assert(1 == ParseRTCPPacket(data, len, boundingSet));
            Verify(boundingSet, 0, set4_2);

            ++_cnt;
            ::Sleep(5*RTCP_INTERVAL_AUDIO_MS + 1000); 
            _rtpRtcpModule->Process();             
                                                   
        }
        else if (_cnt == 8)
        {
            
            assert(-1 == ParseRTCPPacket(data, len, boundingSet));
        }
        else if (_cnt == 10)
        {
            
            assert(0 == ParseRTCPPacket(data, len, boundingSet));

            strcpy(fileName, "RTCPPacketTMMBR2.bin");
        }
        else if (_cnt == 11)
        {
            
            assert(1 == ParseRTCPPacket(data, len, boundingSet));
            Verify(boundingSet, 0, set2);
        }
        else if (_cnt == 12) 
        {
            
            assert(-1 == ParseRTCPPacket(data, len, boundingSet));

            strcpy(fileName, "RTCPPacketTMMBR4.bin");
        }
        else if (_cnt == 13)
        {
            
            
            
            
            assert(2 == ParseRTCPPacket(data, len, boundingSet));
            Verify(boundingSet, 0, set4);
            Verify(boundingSet, 1, set2);

            strcpy(fileName, "RTCPPacketTMMBR0.bin");
        }
        else if (_cnt == 14)
        {
            
            
            
            
            assert(3 == ParseRTCPPacket(data, len, boundingSet));
            Verify(boundingSet, 0, set3);
            Verify(boundingSet, 1, set4);
            Verify(boundingSet, 2, set2);

            strcpy(fileName, "RTCPPacketTMMBR1.bin");
        }
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        else if (_cnt == 15)
        {
            
            
            
            
            assert(0 == ParseRTCPPacket(data, len, boundingSet));
        }

        ++_cnt;

        
        len = GetFile(fileName);
        if (len == 0)
        {
            return 1;
        }

        
        if( 0 == _rtpRtcpModule->IncomingPacket((const WebRtc_UWord8*)dataFile, len))
        {
            return len;
        }
        return -1;
    }

    RtpRtcp* _rtpRtcpModule;
    WebRtc_UWord32       _cnt;
};

class TestTMMBR : private TMMBRHelp
{
public:
    TestTMMBR() : TMMBRHelp(false) {};

    void Add(TMMBRSet* candidateSet, int index, InputSet set)
    {
        candidateSet->ptrTmmbrSet[index]    = set.TMMBR;
        candidateSet->ptrPacketOHSet[index] = set.packetOH;
        candidateSet->ptrSsrcSet[index]     = set.SSRC;
    };

    void Start()
    {
        
        TMMBRSet* candidateSet = CandidateSet();
        assert(0 == candidateSet->sizeOfSet);
        TMMBRSet* boundingSet = BoundingSet();
        assert(0 == boundingSet->sizeOfSet);
        TMMBRSet* boundingSetToSend = BoundingSetToSend();
        assert(0 == boundingSetToSend->sizeOfSet);

        WebRtc_Word32 numBoundingSet = FindTMMBRBoundingSet(boundingSet);
        assert(0 == numBoundingSet); 

        assert( 0 == SetTMMBRBoundingSetToSend(NULL,0));        
        assert( 0 == SetTMMBRBoundingSetToSend(boundingSet,0)); 

        WebRtc_UWord32 minBitrateKbit = 0;
        WebRtc_UWord32 maxBitrateKbit = 0;
        assert(-1 == CalcMinMaxBitRate(0, 0, 1, false, minBitrateKbit, maxBitrateKbit)); 

        
        
        
        candidateSet = VerifyAndAllocateCandidateSet(1);
        assert(1 == candidateSet->sizeOfSet);
        Add(candidateSet, 0, set0);

        
        numBoundingSet = FindTMMBRBoundingSet(boundingSet);
        assert(1 == numBoundingSet);
        Verify(boundingSet, 0, set0);

        
        assert(!IsOwner(set0.SSRC, 0));   
        assert(!IsOwner(set1.SSRC, 100)); 

        assert( IsOwner(set0.SSRC, numBoundingSet));
        assert(!IsOwner(set1.SSRC, numBoundingSet));
        assert(!IsOwner(set2.SSRC, numBoundingSet));

        
        assert(0 == SetTMMBRBoundingSetToSend(boundingSet, maxBitrate));

        
        boundingSetToSend = BoundingSetToSend();
        assert(boundingSetToSend->sizeOfSet == numBoundingSet);
        Verify(boundingSetToSend, 0, set0);

        
        assert( 0 == CalcMinMaxBitRate(0, numBoundingSet, false,0, minBitrateKbit, maxBitrateKbit));
        assert(set0.TMMBR == minBitrateKbit);
        assert(set0.TMMBR == maxBitrateKbit);
        assert(0 == CalcMinMaxBitRate(0, 100, false,0, minBitrateKbit, maxBitrateKbit));  
        assert(set0.TMMBR == minBitrateKbit);
        assert(set0.TMMBR == maxBitrateKbit);

        
        
        
        candidateSet = VerifyAndAllocateCandidateSet(2);
        assert(2 == candidateSet->sizeOfSet);
        Add(candidateSet, 0, set0);
        Add(candidateSet, 1, set1);

        
        numBoundingSet = FindTMMBRBoundingSet(boundingSet);
        assert(1 == numBoundingSet);
        Verify(boundingSet, 0, set1);

        
        assert(!IsOwner(set0.SSRC, numBoundingSet));
        assert( IsOwner(set1.SSRC, numBoundingSet));
        assert(!IsOwner(set2.SSRC, numBoundingSet));

        
        assert(0 == SetTMMBRBoundingSetToSend(boundingSet, maxBitrate));

        
        boundingSetToSend = BoundingSetToSend();
        assert(boundingSetToSend->sizeOfSet == numBoundingSet);
        Verify(boundingSetToSend, 0, set1);

        
        assert(0 == CalcMinMaxBitRate(0, numBoundingSet, true,0, minBitrateKbit, maxBitrateKbit));
        assert(set1.TMMBR == minBitrateKbit);
        assert(set0.TMMBR == maxBitrateKbit);

        
        
        
        candidateSet = VerifyAndAllocateCandidateSet(3);
        assert(3 == candidateSet->sizeOfSet);
        Add(candidateSet, 0, set0);
        Add(candidateSet, 1, set1);
        Add(candidateSet, 2, set2);

        
        numBoundingSet = FindTMMBRBoundingSet(boundingSet);
        assert(1 == numBoundingSet);
        Verify(boundingSet, 0, set2);

        
        assert(!IsOwner(set0.SSRC, numBoundingSet));
        assert(!IsOwner(set1.SSRC, numBoundingSet));
        assert( IsOwner(set2.SSRC, numBoundingSet));

        
        assert(0 == SetTMMBRBoundingSetToSend(boundingSet, maxBitrate));

        
        boundingSetToSend = BoundingSetToSend();
        assert(boundingSetToSend->sizeOfSet == numBoundingSet);
        Verify(boundingSetToSend, 0, set2);

        
        assert(0 == CalcMinMaxBitRate(0, numBoundingSet, true,0, minBitrateKbit, maxBitrateKbit));
        assert(set2.TMMBR == minBitrateKbit);
        assert(set0.TMMBR == maxBitrateKbit);

        
        
        
        candidateSet = VerifyAndAllocateCandidateSet(4);
        assert(4 == candidateSet->sizeOfSet);
        Add(candidateSet, 0, set0);
        Add(candidateSet, 1, set1);
        Add(candidateSet, 2, set2);
        Add(candidateSet, 3, set3);

        
        numBoundingSet = FindTMMBRBoundingSet(boundingSet);
        assert(2 == numBoundingSet);
        Verify(boundingSet, 0, set3);
        Verify(boundingSet, 1, set2);

        
        assert(!IsOwner(set0.SSRC, numBoundingSet));
        assert(!IsOwner(set1.SSRC, numBoundingSet));
        assert( IsOwner(set2.SSRC, numBoundingSet));
        assert( IsOwner(set3.SSRC, numBoundingSet));

        
        assert(0 == SetTMMBRBoundingSetToSend(boundingSet, maxBitrate));

        
        boundingSetToSend = BoundingSetToSend();
        assert(boundingSetToSend->sizeOfSet == numBoundingSet);
        Verify(boundingSetToSend, 0, set3);
        Verify(boundingSetToSend, 1, set2);

        
        assert(0 == CalcMinMaxBitRate(0, numBoundingSet, true,0, minBitrateKbit, maxBitrateKbit));
        assert(set3.TMMBR == minBitrateKbit);
        assert(set0.TMMBR == maxBitrateKbit);

        
        
        
        candidateSet = VerifyAndAllocateCandidateSet(5);
        assert(5 == candidateSet->sizeOfSet);
        Add(candidateSet, 0, set0);
        Add(candidateSet, 1, set1);
        Add(candidateSet, 2, set2);
        Add(candidateSet, 3, set3);
        Add(candidateSet, 4, set4);

        
        numBoundingSet = FindTMMBRBoundingSet(boundingSet);
        assert(3 == numBoundingSet);
        Verify(boundingSet, 0, set3);
        Verify(boundingSet, 1, set4);
        Verify(boundingSet, 2, set2);

        
        assert(!IsOwner(set0.SSRC, numBoundingSet));
        assert(!IsOwner(set1.SSRC, numBoundingSet));
        assert( IsOwner(set2.SSRC, numBoundingSet));
        assert( IsOwner(set3.SSRC, numBoundingSet));
        assert( IsOwner(set4.SSRC, numBoundingSet));

        
        assert(0 == SetTMMBRBoundingSetToSend(boundingSet, maxBitrate));

        
        boundingSetToSend = BoundingSetToSend();
        assert(boundingSetToSend->sizeOfSet == numBoundingSet);
        Verify(boundingSetToSend, 0, set3);
        Verify(boundingSetToSend, 1, set4);
        Verify(boundingSetToSend, 2, set2);

        
        assert(0 == CalcMinMaxBitRate(0,numBoundingSet, true,0, minBitrateKbit, maxBitrateKbit));
        assert(set3.TMMBR == minBitrateKbit);
        assert(set0.TMMBR == maxBitrateKbit);

        
        
        
        candidateSet = VerifyAndAllocateCandidateSet(6);
        assert(6 == candidateSet->sizeOfSet);
        Add(candidateSet, 0, set0);
        Add(candidateSet, 1, set1);
        Add(candidateSet, 2, set2);
        Add(candidateSet, 3, set3);
        Add(candidateSet, 4, set4);
        Add(candidateSet, 5, set5);

        
        numBoundingSet = FindTMMBRBoundingSet(boundingSet);
        assert(3 == numBoundingSet);
        Verify(boundingSet, 0, set3);
        Verify(boundingSet, 1, set4);
        Verify(boundingSet, 2, set2);

        
        assert(!IsOwner(set0.SSRC, numBoundingSet));
        assert(!IsOwner(set1.SSRC, numBoundingSet));
        assert( IsOwner(set2.SSRC, numBoundingSet));
        assert( IsOwner(set3.SSRC, numBoundingSet));
        assert( IsOwner(set4.SSRC, numBoundingSet));
        assert(!IsOwner(set5.SSRC, numBoundingSet));

        
        assert(0 == SetTMMBRBoundingSetToSend(boundingSet, maxBitrate));

        
        boundingSetToSend = BoundingSetToSend();
        assert(boundingSetToSend->sizeOfSet == numBoundingSet);
        Verify(boundingSetToSend, 0, set3);
        Verify(boundingSetToSend, 1, set4);
        Verify(boundingSetToSend, 2, set2);

        
        assert(0 == CalcMinMaxBitRate(0,numBoundingSet, true,0, minBitrateKbit, maxBitrateKbit));
        assert(set3.TMMBR == minBitrateKbit);
        assert(set0.TMMBR == maxBitrateKbit);


        
        
        
        candidateSet = VerifyAndAllocateCandidateSet(5);
        assert(6 == candidateSet->sizeOfSet);
        Add(candidateSet, 0, set1);
        Add(candidateSet, 1, set2);
        Add(candidateSet, 2, set3);
        Add(candidateSet, 3, set4);
        Add(candidateSet, 4, set5);

        
        numBoundingSet = FindTMMBRBoundingSet(boundingSet);
        assert(3 == numBoundingSet);
        Verify(boundingSet, 0, set3);
        Verify(boundingSet, 1, set4);
        Verify(boundingSet, 2, set2);

        
        assert(!IsOwner(set0.SSRC, numBoundingSet));
        assert(!IsOwner(set1.SSRC, numBoundingSet));
        assert( IsOwner(set2.SSRC, numBoundingSet));
        assert( IsOwner(set3.SSRC, numBoundingSet));
        assert( IsOwner(set4.SSRC, numBoundingSet));
        assert(!IsOwner(set5.SSRC, numBoundingSet));

        
        assert(0 == SetTMMBRBoundingSetToSend(boundingSet, maxBitrate));

        
        boundingSetToSend = BoundingSetToSend();
        assert(boundingSetToSend->sizeOfSet == numBoundingSet);
        Verify(boundingSetToSend, 0, set3);
        Verify(boundingSetToSend, 1, set4);
        Verify(boundingSetToSend, 2, set2);

        
        assert(0 == CalcMinMaxBitRate(0,numBoundingSet, true,0, minBitrateKbit, maxBitrateKbit));
        assert(set3.TMMBR == minBitrateKbit);
        assert(set5.TMMBR == maxBitrateKbit);


        
        
        
        candidateSet = VerifyAndAllocateCandidateSet(4);
        assert(6 == candidateSet->sizeOfSet);
        Add(candidateSet, 0, set1);
        Add(candidateSet, 1, set3);
        Add(candidateSet, 2, set4);
        Add(candidateSet, 3, set5);

        
        numBoundingSet = FindTMMBRBoundingSet(boundingSet);
        assert(2 == numBoundingSet);
        Verify(boundingSet, 0, set3);
        Verify(boundingSet, 1, set4);

        
        assert(!IsOwner(set0.SSRC, numBoundingSet));
        assert(!IsOwner(set1.SSRC, numBoundingSet));
        assert(!IsOwner(set2.SSRC, numBoundingSet));
        assert( IsOwner(set3.SSRC, numBoundingSet));
        assert( IsOwner(set4.SSRC, numBoundingSet));
        assert(!IsOwner(set5.SSRC, numBoundingSet));

        
        assert(0 == SetTMMBRBoundingSetToSend(boundingSet, maxBitrate));

        
        boundingSetToSend = BoundingSetToSend();
        Verify(boundingSetToSend, 0, set3);
        Verify(boundingSetToSend, 1, set4);

        
        assert(0 == CalcMinMaxBitRate(0, numBoundingSet,true,0,  minBitrateKbit, maxBitrateKbit));
        assert(set3.TMMBR == minBitrateKbit);
        assert(set5.TMMBR == maxBitrateKbit);

        
        
        
        candidateSet = VerifyAndAllocateCandidateSet(4);
        assert(6 == candidateSet->sizeOfSet);
        Add(candidateSet, 0, set1);
        Add(candidateSet, 1, set2);
        Add(candidateSet, 2, set4);
        Add(candidateSet, 3, set5);

        
        numBoundingSet = FindTMMBRBoundingSet(boundingSet);
        assert(2 == numBoundingSet);
        Verify(boundingSet, 0, set4);
        Verify(boundingSet, 1, set2);

        
        assert(!IsOwner(set0.SSRC, numBoundingSet));
        assert(!IsOwner(set1.SSRC, numBoundingSet));
        assert( IsOwner(set2.SSRC, numBoundingSet));
        assert(!IsOwner(set3.SSRC, numBoundingSet));
        assert( IsOwner(set4.SSRC, numBoundingSet));
        assert(!IsOwner(set5.SSRC, numBoundingSet));

        
        assert(0 == SetTMMBRBoundingSetToSend(boundingSet, maxBitrate));

        
        boundingSetToSend = BoundingSetToSend();
        Verify(boundingSetToSend, 0, set4);
        Verify(boundingSetToSend, 1, set2);

        
        assert(0 == CalcMinMaxBitRate(0, numBoundingSet, true,0, minBitrateKbit, maxBitrateKbit));
        assert(set4.TMMBR == minBitrateKbit);
        assert(set5.TMMBR == maxBitrateKbit);

        
        
        
        candidateSet = VerifyAndAllocateCandidateSet(4);
        assert(6 == candidateSet->sizeOfSet);
        Add(candidateSet, 0, set1);
        Add(candidateSet, 1, set2);
        Add(candidateSet, 2, set3);
        Add(candidateSet, 3, set5);

        
        numBoundingSet = FindTMMBRBoundingSet(boundingSet);
        assert(2 == numBoundingSet);
        Verify(boundingSet, 0, set3);
        Verify(boundingSet, 1, set2);

        
        assert(!IsOwner(set0.SSRC, numBoundingSet));
        assert(!IsOwner(set1.SSRC, numBoundingSet));
        assert( IsOwner(set2.SSRC, numBoundingSet));
        assert( IsOwner(set3.SSRC, numBoundingSet));
        assert(!IsOwner(set4.SSRC, numBoundingSet));
        assert(!IsOwner(set5.SSRC, numBoundingSet));

        
        assert(0 == SetTMMBRBoundingSetToSend(boundingSet, maxBitrate));

        
        boundingSetToSend = BoundingSetToSend();
        Verify(boundingSetToSend, 0, set3);
        Verify(boundingSetToSend, 1, set2);

        
        assert(0 == CalcMinMaxBitRate(0, numBoundingSet, true,0, minBitrateKbit, maxBitrateKbit));
        assert(set3.TMMBR == minBitrateKbit);
        assert(set5.TMMBR == maxBitrateKbit);

        
        
        
        candidateSet = VerifyAndAllocateCandidateSet(5);
        assert(6 == candidateSet->sizeOfSet);
        Add(candidateSet, 0, set1);
        Add(candidateSet, 1, set2);
        Add(candidateSet, 2, set3);
        Add(candidateSet, 3, set4_1);
        Add(candidateSet, 4, set5);

        
        numBoundingSet = FindTMMBRBoundingSet(boundingSet);
        assert(2 == numBoundingSet);
        Verify(boundingSet, 0, set3);
        Verify(boundingSet, 1, set2);

        
        assert(!IsOwner(set0.SSRC, numBoundingSet));
        assert(!IsOwner(set1.SSRC, numBoundingSet));
        assert( IsOwner(set2.SSRC, numBoundingSet));
        assert( IsOwner(set3.SSRC, numBoundingSet));
        assert(!IsOwner(set4.SSRC, numBoundingSet));
        assert(!IsOwner(set5.SSRC, numBoundingSet));

        
        assert(0 == SetTMMBRBoundingSetToSend(boundingSet, maxBitrate));

        
        boundingSetToSend = BoundingSetToSend();
        Verify(boundingSetToSend, 0, set3);
        Verify(boundingSetToSend, 1, set2);

        
        assert(0 == CalcMinMaxBitRate(0, numBoundingSet, true,0, minBitrateKbit, maxBitrateKbit));
        assert(set3.TMMBR == minBitrateKbit);
        assert(set5.TMMBR == maxBitrateKbit);

        
        
        
        candidateSet = VerifyAndAllocateCandidateSet(5);
        assert(6 == candidateSet->sizeOfSet);
        Add(candidateSet, 0, set1);
        Add(candidateSet, 1, set2);
        Add(candidateSet, 2, set3);
        Add(candidateSet, 3, set4_2);
        Add(candidateSet, 4, set5);

        
        numBoundingSet = FindTMMBRBoundingSet(boundingSet);
        assert(1 == numBoundingSet);
        Verify(boundingSet, 0, set4_2);

        
        assert(!IsOwner(set0.SSRC, numBoundingSet));
        assert(!IsOwner(set1.SSRC, numBoundingSet));
        assert(!IsOwner(set2.SSRC, numBoundingSet));
        assert(!IsOwner(set3.SSRC, numBoundingSet));
        assert( IsOwner(set4.SSRC, numBoundingSet));
        assert(!IsOwner(set5.SSRC, numBoundingSet));

        
        assert(0 == SetTMMBRBoundingSetToSend(boundingSet, maxBitrate));

        
        boundingSetToSend = BoundingSetToSend();
        Verify(boundingSetToSend, 0, set4_2);

        
        assert(0 == CalcMinMaxBitRate(0, numBoundingSet, true,0, minBitrateKbit, maxBitrateKbit));
        assert(MIN_VIDEO_BW_MANAGEMENT_BITRATE == minBitrateKbit);
        assert(set5.TMMBR == maxBitrateKbit);

        
        
        
        candidateSet = VerifyAndAllocateCandidateSet(0);
        assert(6 == candidateSet->sizeOfSet);

        
        numBoundingSet = FindTMMBRBoundingSet(boundingSet);
        assert(0 == numBoundingSet);

        
        assert(!IsOwner(set0.SSRC, numBoundingSet));
        assert(!IsOwner(set1.SSRC, numBoundingSet));
        assert(!IsOwner(set2.SSRC, numBoundingSet));
        assert(!IsOwner(set3.SSRC, numBoundingSet));
        assert(!IsOwner(set4.SSRC, numBoundingSet));
        assert(!IsOwner(set5.SSRC, numBoundingSet));

        
        assert(0 == SetTMMBRBoundingSetToSend(boundingSet, maxBitrate));

        
        boundingSetToSend = BoundingSetToSend();

        
        assert(-1 == CalcMinMaxBitRate(0,numBoundingSet, true,0, minBitrateKbit, maxBitrateKbit));

        
        
        
        candidateSet = VerifyAndAllocateCandidateSet(2);
        assert(6 == candidateSet->sizeOfSet);
        Add(candidateSet, 0, set00);
        Add(candidateSet, 1, set5);

        
        numBoundingSet = FindTMMBRBoundingSet(boundingSet);
        assert(1 == numBoundingSet);
        Verify(boundingSet, 0, set5);

        
        assert(!IsOwner(set0.SSRC, numBoundingSet));
        assert(!IsOwner(set1.SSRC, numBoundingSet));
        assert(!IsOwner(set2.SSRC, numBoundingSet));
        assert(!IsOwner(set3.SSRC, numBoundingSet));
        assert(!IsOwner(set4.SSRC, numBoundingSet));
        assert( IsOwner(set5.SSRC, numBoundingSet));

        
        assert(0 == SetTMMBRBoundingSetToSend(boundingSet, maxBitrate));

        
        boundingSetToSend = BoundingSetToSend();
        Verify(boundingSetToSend, 0, set5);

        
        assert(0 == CalcMinMaxBitRate(0,numBoundingSet, true,0, minBitrateKbit, maxBitrateKbit));
        assert(set5.TMMBR == minBitrateKbit);
        assert(set5.TMMBR == maxBitrateKbit);

        
        
        
        candidateSet = VerifyAndAllocateCandidateSet(3);
        assert(6 == candidateSet->sizeOfSet);
        Add(candidateSet, 0, set00);
        Add(candidateSet, 1, set4);
        Add(candidateSet, 2, set2);

        
        numBoundingSet = FindTMMBRBoundingSet(boundingSet);
        assert(2 == numBoundingSet);
        Verify(boundingSet, 0, set4);
        Verify(boundingSet, 1, set2);

        
        assert(!IsOwner(set0.SSRC, numBoundingSet));
        assert(!IsOwner(set1.SSRC, numBoundingSet));
        assert( IsOwner(set2.SSRC, numBoundingSet));
        assert(!IsOwner(set3.SSRC, numBoundingSet));
        assert( IsOwner(set4.SSRC, numBoundingSet));
        assert(!IsOwner(set5.SSRC, numBoundingSet));

        
        assert(0 == SetTMMBRBoundingSetToSend(boundingSet, maxBitrate));

        
        boundingSetToSend = BoundingSetToSend();
        Verify(boundingSetToSend, 0, set4);
        Verify(boundingSetToSend, 1, set2);

        
        assert(0 == CalcMinMaxBitRate(0,numBoundingSet, true,0, minBitrateKbit, maxBitrateKbit));
        assert(set4.TMMBR == minBitrateKbit);
        assert(set2.TMMBR == maxBitrateKbit);
    };
};

class NULLDataZink: public RtpData
{
    virtual WebRtc_Word32 OnReceivedPayloadData(const WebRtc_UWord8* payloadData,
                                              const WebRtc_UWord16 payloadSize,
                                              const webrtc::WebRtcRTPHeader* rtpHeader,
                                              const WebRtc_UWord8* incomingRtpPacket,
                                              const WebRtc_UWord16 incomingRtpPacketLengt)
    {
        return 0;
    };
};


int _tmain(int argc, _TCHAR* argv[])
{

    std::string str;
    std::cout << "------------------------" << std::endl;
    std::cout << "------ Test TMMBR ------" << std::endl;
    std::cout << "------------------------" << std::endl;
    std::cout << "  "  << std::endl;

    
    
    
    TestTMMBR test;
    test.Start();

    printf("TMMBRHelp-class test done.\n");

    
    
    
    RtpRtcp* rtpRtcpModuleVideo = RtpRtcp::CreateRtpRtcp(0, false);

    LoopBackTransportVideo* myLoopBackTransportVideo = new LoopBackTransportVideo(rtpRtcpModuleVideo);
    assert(0 == rtpRtcpModuleVideo->RegisterSendTransport(myLoopBackTransportVideo));

    assert(false == rtpRtcpModuleVideo->TMMBR());
    rtpRtcpModuleVideo->SetTMMBRStatus(true);
    assert(true == rtpRtcpModuleVideo->TMMBR());

    assert(0 == rtpRtcpModuleVideo->RegisterSendPayload( "I420", 96));
    assert(0 == rtpRtcpModuleVideo->RegisterReceivePayload( "I420", 96));

    
    assert(0 == rtpRtcpModuleVideo->SetSSRC(11111));
    const WebRtc_UWord8 testStream[9] = "testtest";
    assert(0 == rtpRtcpModuleVideo->RegisterIncomingDataCallback(new NULLDataZink())); 
    assert(0 == rtpRtcpModuleVideo->SendOutgoingData(webrtc::kVideoFrameKey,96, 0, testStream, 8));

    
    assert(0 == rtpRtcpModuleVideo->SetSSRC(0));

    
    assert(0 == rtpRtcpModuleVideo->SetRTCPStatus(kRtcpCompound));

    assert(0 == rtpRtcpModuleVideo->SendRTCP());  
    assert(0 == rtpRtcpModuleVideo->SendRTCP());  
    assert(0 == rtpRtcpModuleVideo->SendRTCP());  
    assert(0 == rtpRtcpModuleVideo->SendRTCP());  
    assert(0 == rtpRtcpModuleVideo->SendRTCP());  
    assert(0 == rtpRtcpModuleVideo->SendRTCP());  
    assert(0 == rtpRtcpModuleVideo->SendRTCP());  
    assert(0 == rtpRtcpModuleVideo->SendRTCP()); 
    assert(0 == rtpRtcpModuleVideo->SendRTCP());  
    assert(0 == rtpRtcpModuleVideo->SendRTCP());  

    printf("Single module test done.\n");

    
    
    
    RtpRtcp* rtpRtcpModuleVideoDef = RtpRtcp::CreateRtpRtcp(10, false);
    assert(0 == rtpRtcpModuleVideo->RegisterDefaultModule(rtpRtcpModuleVideoDef));

    RtpRtcp* rtpRtcpModuleVideo1 = RtpRtcp::CreateRtpRtcp(1, false);
    assert(0 == rtpRtcpModuleVideo1->RegisterDefaultModule(rtpRtcpModuleVideoDef));

    RtpRtcp* rtpRtcpModuleVideo2 = RtpRtcp::CreateRtpRtcp(2, false);
    assert(0 == rtpRtcpModuleVideo2->RegisterDefaultModule(rtpRtcpModuleVideoDef));

    RtpRtcp* rtpRtcpModuleVideo3 = RtpRtcp::CreateRtpRtcp(3, false);
    assert(0 == rtpRtcpModuleVideo3->RegisterDefaultModule(rtpRtcpModuleVideoDef));

    LoopBackTransport2* myLoopBackTransport2 = new LoopBackTransport2(rtpRtcpModuleVideo2);
    assert(0 == rtpRtcpModuleVideo2->RegisterSendTransport(myLoopBackTransport2));

    assert(0 == rtpRtcpModuleVideo2->SetRTCPStatus(kRtcpCompound));

    
    assert(0 == rtpRtcpModuleVideo2->SetSSRC(0));

    assert(0 == rtpRtcpModuleVideo->SendRTCP());   
    assert(0 == rtpRtcpModuleVideo->SendRTCP());   
    assert(0 == rtpRtcpModuleVideo2->SendRTCP());  
    assert(0 == rtpRtcpModuleVideo->SendRTCP());   
    ::Sleep(5*RTCP_INTERVAL_AUDIO_MS + 1000);
    rtpRtcpModuleVideo2->Process();                
    assert(0 == rtpRtcpModuleVideo->SendRTCP());   

    printf("Multi module test done.\n");


    RtpRtcp::DestroyRtpRtcp(rtpRtcpModuleVideo);
    RtpRtcp::DestroyRtpRtcp(rtpRtcpModuleVideo1);
    RtpRtcp::DestroyRtpRtcp(rtpRtcpModuleVideo2);
    RtpRtcp::DestroyRtpRtcp(rtpRtcpModuleVideo3);
    RtpRtcp::DestroyRtpRtcp(rtpRtcpModuleVideoDef);

    TEST_PASSED();
    ::Sleep(5000);

    return 0;
}

