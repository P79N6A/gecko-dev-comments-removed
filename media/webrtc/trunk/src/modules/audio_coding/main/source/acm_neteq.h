









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_NETEQ_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_NETEQ_H_

#include "audio_coding_module.h"
#include "audio_coding_module_typedefs.h"
#include "engine_configurations.h"
#include "module_common_types.h"
#include "typedefs.h"
#include "webrtc_neteq.h"
#include "webrtc_vad.h"

namespace webrtc {

class CriticalSectionWrapper;
class RWLockWrapper;
struct CodecInst;
enum AudioPlayoutMode;
enum ACMSpeechType;

#define MAX_NUM_SLAVE_NETEQ 1

class ACMNetEQ
{
public:
    
    ACMNetEQ();

    
    ~ACMNetEQ();

    
    
    
    
    
    
    
    
    WebRtc_Word32 Init();

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    WebRtc_Word32 RecIn(
        const WebRtc_UWord8*    incomingPayload,
        const WebRtc_Word32    payloadLength,
        const WebRtcRTPHeader&   rtpInfo);

    
    
    
    
    
    
    
    
    
    
    
    WebRtc_Word32 RecOut(
        AudioFrame& audioFrame);

    
    
    
    
    
    
    
    
    
    
    
    
    
    WebRtc_Word32 AddCodec(
        WebRtcNetEQ_CodecDef *codecDef,
        bool                  toMaster = true);

    
    
    
    
    
    
    
    
    
    
    
    WebRtc_Word32 AllocatePacketBuffer(
        const WebRtcNetEQDecoder* usedCodecs,
        WebRtc_Word16    noOfCodecs);

    
    
    
    
    
    
    
    
    
    
    WebRtc_Word32 SetExtraDelay(
        const WebRtc_Word32 delayInMS);

    
    
    
    
    
    
    
    
    
    
    WebRtc_Word32 SetAVTPlayout(
        const bool enable);

    
    
    
    
    
    
    
    bool AVTPlayout() const;

    
    
    
    
    
    
    WebRtc_Word32 CurrentSampFreqHz() const;

    
    
    
    
    
    
    
    
    
    
    
    WebRtc_Word32 SetPlayoutMode(
        const AudioPlayoutMode mode);

    
    
    
    
    
    
    AudioPlayoutMode PlayoutMode() const;

    
    
    
    
    
    
    
    
    
    
    WebRtc_Word32 NetworkStatistics(
        ACMNetworkStatistics* statistics) const;

    
    
    
    
    
    
    ACMVADMode VADMode() const;

    
    
    
    
    
    
    
    
    
    
    WebRtc_Word16 SetVADMode(
        const ACMVADMode mode);

    
    
    
    
    
    
    RWLockWrapper* DecodeLock() const
    {
        return _decodeLock;
    }

    
    
    
    
    
    
    
    WebRtc_Word32 FlushBuffers();

    
    
    
    
    
    
    
    
    
    
    WebRtc_Word16 RemoveCodec(
        WebRtcNetEQDecoder codecIdx,
        bool isStereo = false);


    
    
    
    
    
    
    
    
    
    
    
    WebRtc_Word16 SetBackgroundNoiseMode(
        const ACMBackgroundNoiseMode mode);

    
    
    
    
    
    
    WebRtc_Word16 BackgroundNoiseMode(
        ACMBackgroundNoiseMode& mode);

    void SetUniqueId(
        WebRtc_Word32 id);

    WebRtc_Word32 PlayoutTimestamp(
        WebRtc_UWord32& timestamp);

    void SetReceivedStereo(
        bool receivedStereo);

    WebRtc_UWord8 NumSlaves();

    enum JB {masterJB = 0, slaveJB = 1};

    
    void RemoveSlaves();

    WebRtc_Word16 AddSlave(
        const WebRtcNetEQDecoder*    usedCodecs,
        WebRtc_Word16       noOfCodecs);

private:
    
    
    
    
    
    
    
    
    
    
    
    
    
    static void RTPPack(
        WebRtc_Word16*         rtpPacket,
        const WebRtc_Word8*    payload,
        const WebRtc_Word32    payloadLengthW8,
        const WebRtcRTPHeader& rtpInfo);

    void LogError(
        const char* neteqFuncName,
        const WebRtc_Word16 idx) const;

    WebRtc_Word16 InitByIdxSafe(
        const WebRtc_Word16 idx);

    
    
    
    
    
    
    WebRtc_Word16 EnableVAD();

    WebRtc_Word16 EnableVADByIdxSafe(
        const WebRtc_Word16 idx);

    WebRtc_Word16 AllocatePacketBufferByIdxSafe(
        const WebRtcNetEQDecoder* usedCodecs,
        WebRtc_Word16       noOfCodecs,
        const WebRtc_Word16 idx);

    
    void RemoveNetEQSafe(int index);

    void RemoveSlavesSafe();

    void*                   _inst[MAX_NUM_SLAVE_NETEQ + 1];
    void*                   _instMem[MAX_NUM_SLAVE_NETEQ + 1];

    WebRtc_Word16*          _netEqPacketBuffer[MAX_NUM_SLAVE_NETEQ + 1];

    WebRtc_Word32           _id;
    float                   _currentSampFreqKHz;
    bool                    _avtPlayout;
    AudioPlayoutMode        _playoutMode;
    CriticalSectionWrapper* _netEqCritSect;

    WebRtcVadInst*          _ptrVADInst[MAX_NUM_SLAVE_NETEQ + 1];

    bool                    _vadStatus;
    ACMVADMode              _vadMode;
    RWLockWrapper*          _decodeLock;
    bool                    _isInitialized[MAX_NUM_SLAVE_NETEQ + 1];
    WebRtc_UWord8           _numSlaves;
    bool                    _receivedStereo;
    void*                   _masterSlaveInfo;
    AudioFrame::VADActivity _previousAudioActivity;
    WebRtc_Word32           _extraDelay;

    CriticalSectionWrapper* _callbackCritSect;
};

} 

#endif  
