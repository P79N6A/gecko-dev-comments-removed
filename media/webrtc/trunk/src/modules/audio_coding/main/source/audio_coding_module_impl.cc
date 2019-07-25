









#include "acm_codec_database.h"
#include "acm_common_defs.h"
#include "acm_dtmf_detection.h"
#include "acm_generic_codec.h"
#include "acm_resampler.h"
#include "audio_coding_module_impl.h"
#include "critical_section_wrapper.h"
#include "engine_configurations.h"
#include "rw_lock_wrapper.h"
#include "trace.h"

#include <assert.h>
#include <stdlib.h>

#ifdef ACM_QA_TEST
#   include <stdio.h>
#endif

#ifdef TIMED_LOGGING
    char message[500];
    #include "../test/timedtrace.h"
    #include <string.h>
    #define LOGWITHTIME(logString)                \
                sprintf(message, logString, _id); \
                _trace.TimedLogg(message);
#else
    #define LOGWITHTIME(logString)
#endif

namespace webrtc
{

enum {
    kACMToneEnd = 999
};


enum {
    kMaxPacketSize = 2560
};

AudioCodingModuleImpl::AudioCodingModuleImpl(
    const WebRtc_Word32 id):
    _packetizationCallback(NULL),
    _id(id),
    _lastTimestamp(0),
    _lastInTimestamp(0),
    _cng_nb_pltype(255),
    _cng_wb_pltype(255),
    _cng_swb_pltype(255),
    _red_pltype(255),
    _cng_reg_receiver(false),
    _vadEnabled(false),
    _dtxEnabled(false),
    _vadMode(VADNormal),
    _stereoReceiveRegistered(false),
    _stereoSend(false),
    _prev_received_channel(0),
    _expected_channels(1),
    _currentSendCodecIdx(-1),  
    _current_receive_codec_idx(-1),  
    _sendCodecRegistered(false),
    _acmCritSect(CriticalSectionWrapper::CreateCriticalSection()),
    _vadCallback(NULL),
    _lastRecvAudioCodecPlType(255),
    _isFirstRED(true),
    _fecEnabled(false),
    _fragmentation(NULL),
    _lastFECTimestamp(0),
    _receiveREDPayloadType(255),  
    _previousPayloadType(255),
    _dummyRTPHeader(NULL),
    _recvPlFrameSizeSmpls(0),
    _receiverInitialized(false),
    _dtmfDetector(NULL),
    _dtmfCallback(NULL),
    _lastDetectedTone(kACMToneEnd),
    _callbackCritSect(CriticalSectionWrapper::CreateCriticalSection())
{
    _lastTimestamp = 0xD87F3F9F;
    _lastInTimestamp = 0xD87F3F9F;

    
    
    memset(&_sendCodecInst, 0, sizeof(CodecInst));
    strncpy(_sendCodecInst.plname, "noCodecRegistered", 31);
    _sendCodecInst.pltype = -1;

    for (int i = 0; i < ACMCodecDB::kMaxNumCodecs; i++)
    {
        _codecs[i]            = NULL;
        _registeredPlTypes[i] = -1;
        _stereoReceive[i]     = false;
        _slaveCodecs[i]       = NULL;
        _mirrorCodecIdx[i]    = -1;
    }

    _netEq.SetUniqueId(_id);

    
    _redBuffer = new WebRtc_UWord8[MAX_PAYLOAD_SIZE_BYTE];
    _fragmentation = new RTPFragmentationHeader;
    _fragmentation->fragmentationVectorSize = 2;
    _fragmentation->fragmentationOffset = new WebRtc_UWord32[2];
    _fragmentation->fragmentationLength = new WebRtc_UWord32[2];
    _fragmentation->fragmentationTimeDiff = new WebRtc_UWord16[2];
    _fragmentation->fragmentationPlType = new WebRtc_UWord8[2];

    
    
    for (int i = (ACMCodecDB::kNumCodecs - 1); i>=0; i--)
    {
        if((STR_CASE_CMP(ACMCodecDB::database_[i].plname, "red") == 0))
        {
          _red_pltype = static_cast<uint8_t>(ACMCodecDB::database_[i].pltype);
        }
        else if ((STR_CASE_CMP(ACMCodecDB::database_[i].plname, "CN") == 0))
        {
            if (ACMCodecDB::database_[i].plfreq == 8000)
            {
              _cng_nb_pltype =
                  static_cast<uint8_t>(ACMCodecDB::database_[i].pltype);
            }
            else if (ACMCodecDB::database_[i].plfreq == 16000)
            {
                _cng_wb_pltype =
                    static_cast<uint8_t>(ACMCodecDB::database_[i].pltype);
            } else if (ACMCodecDB::database_[i].plfreq == 32000)
            {
                _cng_swb_pltype =
                    static_cast<uint8_t>(ACMCodecDB::database_[i].pltype);
            }
        }
    }

    if(InitializeReceiverSafe() < 0 )
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
            "Cannot initialize reciever");
    }
#ifdef TIMED_LOGGING
    _trace.SetUp("TimedLogg.txt");
#endif

#ifdef ACM_QA_TEST
    char fileName[500];
    sprintf(fileName, "ACM_QA_incomingPL_%03d_%d%d%d%d%d%d.dat",
        _id,
        rand() % 10,
        rand() % 10,
        rand() % 10,
        rand() % 10,
        rand() % 10,
        rand() % 10);

    _incomingPL = fopen(fileName, "wb");

    sprintf(fileName, "ACM_QA_outgoingPL_%03d_%d%d%d%d%d%d.dat",
        _id,
        rand() % 10,
        rand() % 10,
        rand() % 10,
        rand() % 10,
        rand() % 10,
        rand() % 10);
    _outgoingPL = fopen(fileName, "wb");
#endif

    WEBRTC_TRACE(webrtc::kTraceMemory, webrtc::kTraceAudioCoding, id, "Created");
}

AudioCodingModuleImpl::~AudioCodingModuleImpl()
{
    {
        CriticalSectionScoped lock(*_acmCritSect);
        _currentSendCodecIdx = -1;

        for (int i=0; i < ACMCodecDB::kMaxNumCodecs; i++)
        {
            if (_codecs[i] != NULL)
            {
                
                
                
                if(_slaveCodecs[i] == _codecs[i]) {
                    _slaveCodecs[i] = NULL;
                }

                
                assert(_mirrorCodecIdx[i] > -1);
                if(_codecs[_mirrorCodecIdx[i]] != NULL)
                {
                    delete _codecs[_mirrorCodecIdx[i]];
                    _codecs[_mirrorCodecIdx[i]] = NULL;
                }

                _codecs[i] = NULL;
            }

            if(_slaveCodecs[i] != NULL)
            {
                
                assert(_mirrorCodecIdx[i] > -1);
                if(_slaveCodecs[_mirrorCodecIdx[i]] != NULL)
                {
                    delete _slaveCodecs[_mirrorCodecIdx[i]];
                    _slaveCodecs[_mirrorCodecIdx[i]] =  NULL;
                }
                _slaveCodecs[i] = NULL;
            }
        }

        if(_dtmfDetector != NULL)
        {
            delete _dtmfDetector;
            _dtmfDetector = NULL;
        }
        if(_dummyRTPHeader != NULL)
        {
            delete _dummyRTPHeader;
            _dummyRTPHeader = NULL;
        }
        if(_redBuffer != NULL)
        {
            delete [] _redBuffer;
            _redBuffer = NULL;
        }
        if(_fragmentation != NULL)
        {
            
            
            delete _fragmentation;
            _fragmentation = NULL;
        }
    }

#ifdef ACM_QA_TEST
        if(_incomingPL != NULL)
        {
            fclose(_incomingPL);
        }

        if(_outgoingPL != NULL)
        {
            fclose(_outgoingPL);
        }
#endif

    delete _callbackCritSect;
    _callbackCritSect = NULL;

    delete _acmCritSect;
    _acmCritSect = NULL;
    WEBRTC_TRACE(webrtc::kTraceMemory, webrtc::kTraceAudioCoding, _id, "Destroyed");
}

WebRtc_Word32
AudioCodingModuleImpl::ChangeUniqueId(
    const WebRtc_Word32 id)
{
    {
        CriticalSectionScoped lock(*_acmCritSect);
        _id = id;
#ifdef ACM_QA_TEST
        if(_incomingPL != NULL)
        {
            fclose(_incomingPL);
        }

        if(_outgoingPL != NULL)
        {
            fclose(_outgoingPL);
        }

        char fileName[500];
        sprintf(fileName, "ACM_QA_incomingPL_%03d_%d%d%d%d%d%d.dat",
            _id,
            rand() % 10,
            rand() % 10,
            rand() % 10,
            rand() % 10,
            rand() % 10,
            rand() % 10);

        _incomingPL = fopen(fileName, "wb");

        sprintf(fileName, "ACM_QA_outgoingPL_%03d_%d%d%d%d%d%d.dat",
            _id,
            rand() % 10,
            rand() % 10,
            rand() % 10,
            rand() % 10,
            rand() % 10,
            rand() % 10);
        _outgoingPL = fopen(fileName, "wb");
#endif

        for (int i = 0; i < ACMCodecDB::kMaxNumCodecs; i++)
        {
            if(_codecs[i] != NULL)
            {
                _codecs[i]->SetUniqueID(id);
            }
        }
    }

    _netEq.SetUniqueId(_id);
    return 0;
}



WebRtc_Word32
AudioCodingModuleImpl::TimeUntilNextProcess()
{
    CriticalSectionScoped lock(*_acmCritSect);

    if(!HaveValidEncoder("TimeUntilNextProcess"))
    {
        return -1;
    }
    return _codecs[_currentSendCodecIdx]->SamplesLeftToEncode() /
        (_sendCodecInst.plfreq / 1000);
}


WebRtc_Word32
AudioCodingModuleImpl::Process()
{
    WebRtc_UWord8 bitStream[2 * MAX_PAYLOAD_SIZE_BYTE]; 
    WebRtc_Word16 lengthBytes = 2 * MAX_PAYLOAD_SIZE_BYTE;
    WebRtc_Word16 redLengthBytes = lengthBytes;
    WebRtc_UWord32 rtpTimestamp;
    WebRtc_Word16 status;
    WebRtcACMEncodingType encodingType;
    FrameType frameType = kAudioFrameSpeech;
    WebRtc_UWord8 currentPayloadType = 0;
    bool hasDataToSend = false;
    bool fecActive = false;

    
    {
        CriticalSectionScoped lock(*_acmCritSect);
        if(!HaveValidEncoder("Process"))
        {
            return -1;
        }

        status = _codecs[_currentSendCodecIdx]->Encode(bitStream, &lengthBytes,
                 &rtpTimestamp, &encodingType);
        if (status < 0) 
        {
            
            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                "Process(): Encoding Failed");
            lengthBytes = 0;
            return -1;
        }
        else if(status == 0)
        {
            
            return 0;
        }
        else
        {
            switch(encodingType)
            {
            case kNoEncoding:
                {
                    currentPayloadType = _previousPayloadType;
                    frameType = kFrameEmpty;
                    lengthBytes = 0;
                    break;
                }
            case kActiveNormalEncoded:
            case kPassiveNormalEncoded:
                {
                    currentPayloadType = (WebRtc_UWord8)_sendCodecInst.pltype;
                    frameType = kAudioFrameSpeech;
                    break;
                }
            case kPassiveDTXNB:
                {
                    currentPayloadType =  _cng_nb_pltype;
                    frameType = kAudioFrameCN;
                    _isFirstRED = true;
                    break;
                }
            case kPassiveDTXWB:
                {
                    currentPayloadType =  _cng_wb_pltype;
                    frameType = kAudioFrameCN;
                    _isFirstRED = true;
                    break;
                }
            case kPassiveDTXSWB:
                {
                    currentPayloadType =  _cng_swb_pltype;
                    frameType = kAudioFrameCN;
                    _isFirstRED = true;
                    break;
                }
            }
            hasDataToSend = true;
            _previousPayloadType = currentPayloadType;

            
            
            
            
            
            if((_fecEnabled) &&
                ((encodingType == kActiveNormalEncoded) ||
                 (encodingType == kPassiveNormalEncoded)))
            {
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                fecActive = true;

                hasDataToSend = false;
                if(!_isFirstRED)    
                {
                    
                    
                    memcpy(bitStream + _fragmentation->fragmentationOffset[1], _redBuffer,
                        _fragmentation->fragmentationLength[1]);
                    
                    WebRtc_UWord16 timeSinceLastTimestamp =
                            WebRtc_UWord16(rtpTimestamp - _lastFECTimestamp);

                    
                    _fragmentation->fragmentationPlType[1] =
                            _fragmentation->fragmentationPlType[0];
                    _fragmentation->fragmentationTimeDiff[1] = timeSinceLastTimestamp;
                    hasDataToSend = true;
                }

                
                _fragmentation->fragmentationLength[0] = lengthBytes;

                
                _fragmentation->fragmentationPlType[0] = currentPayloadType;
                _lastFECTimestamp = rtpTimestamp;

                
                redLengthBytes = lengthBytes;
                
                
                
                
                lengthBytes =
                    static_cast<WebRtc_Word16> (_fragmentation->fragmentationLength[0] +
                            _fragmentation->fragmentationLength[1]);

                
                
                
                
                if (_codecs[_currentSendCodecIdx]->GetRedPayload(_redBuffer,
                        &redLengthBytes) == -1)
                {
                    
                    
                    memcpy(_redBuffer, bitStream, redLengthBytes);
                }

                _isFirstRED = false;
                
                currentPayloadType = _red_pltype;
            }
        }
    }

    if(hasDataToSend)
    {
        CriticalSectionScoped lock(*_callbackCritSect);
#ifdef ACM_QA_TEST
        if(_outgoingPL != NULL)
        {
            fwrite(&rtpTimestamp,       sizeof(WebRtc_UWord32), 1, _outgoingPL);
            fwrite(&currentPayloadType, sizeof(WebRtc_UWord8),  1, _outgoingPL);
            fwrite(&lengthBytes,        sizeof(WebRtc_Word16),  1, _outgoingPL);
        }
#endif

        if(_packetizationCallback != NULL)
        {
            if (fecActive) {
                _packetizationCallback->SendData(frameType, currentPayloadType,
                    rtpTimestamp, bitStream, lengthBytes, _fragmentation);
            } else {
                _packetizationCallback->SendData(frameType, currentPayloadType,
                    rtpTimestamp, bitStream, lengthBytes, NULL);
            }
        }

        
        if(_vadCallback != NULL)
        {
            _vadCallback->InFrameType(((WebRtc_Word16)encodingType));
        }
    }
    if (fecActive) {
        _fragmentation->fragmentationLength[1] = redLengthBytes;
    }
    return lengthBytes;
}









WebRtc_Word32
AudioCodingModuleImpl::InitializeSender()
{
    CriticalSectionScoped lock(*_acmCritSect);

    _sendCodecRegistered = false;
    _currentSendCodecIdx = -1; 

    _sendCodecInst.plname[0] = '\0';

    for(int codecCntr = 0; codecCntr < ACMCodecDB::kMaxNumCodecs; codecCntr++)
    {
        if(_codecs[codecCntr] != NULL)
        {
            _codecs[codecCntr]->DestructEncoder();
        }
    }
    
    _isFirstRED = true;
    if(_fecEnabled)
    {
        if(_redBuffer != NULL)
        {
            memset(_redBuffer, 0, MAX_PAYLOAD_SIZE_BYTE);
        }
        if(_fragmentation != NULL)
        {
            _fragmentation->fragmentationVectorSize = 2;
            _fragmentation->fragmentationOffset[0] = 0;
            _fragmentation->fragmentationOffset[0] = MAX_PAYLOAD_SIZE_BYTE;
            memset(_fragmentation->fragmentationLength, 0, sizeof(WebRtc_UWord32) * 2);
            memset(_fragmentation->fragmentationTimeDiff, 0, sizeof(WebRtc_UWord16) * 2);
            memset(_fragmentation->fragmentationPlType, 0, sizeof(WebRtc_UWord8) * 2);
        }
    }

    return 0;
}

WebRtc_Word32
AudioCodingModuleImpl::ResetEncoder()
{
    CriticalSectionScoped lock(*_acmCritSect);
    if(!HaveValidEncoder("ResetEncoder"))
    {
        return -1;
    }
    return _codecs[_currentSendCodecIdx]->ResetEncoder();
}

void
AudioCodingModuleImpl::UnregisterSendCodec()
{
    CriticalSectionScoped lock(*_acmCritSect);
    _sendCodecRegistered = false;
    _currentSendCodecIdx = -1;    

    return;
}

ACMGenericCodec*
AudioCodingModuleImpl::CreateCodec(
    const CodecInst& codec)
{
    ACMGenericCodec* myCodec = NULL;

    myCodec = ACMCodecDB::CreateCodecInstance(&codec);
    if(myCodec == NULL)
    {
        

        
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                    "ACMCodecDB::CreateCodecInstance() failed in \
CreateCodec()");
        return myCodec;
    }
    myCodec->SetUniqueID(_id);
    myCodec->SetNetEqDecodeLock(_netEq.DecodeLock());

    return myCodec;
}


WebRtc_Word32
AudioCodingModuleImpl::RegisterSendCodec(
    const CodecInst& sendCodec)
{
    if((sendCodec.channels != 1) && (sendCodec.channels != 2))
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
            "Registering Send codec failed due to wrong number of channels, %d. Only\
mono codecs are supported, i.e. channels=1.", sendCodec.channels);
        return -1;
    }

    char errMsg[500];
    int mirrorId;
    int codecID = ACMCodecDB::CodecNumber(&sendCodec, &mirrorId, errMsg,
                                          sizeof(errMsg));
    CriticalSectionScoped lock(*_acmCritSect);

    
    if(codecID < 0)
    {
        if(!_sendCodecRegistered)
        {
            
            _currentSendCodecIdx = -1;  
        }
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id, errMsg);
        
        return -1;
    }

    
    if(!STR_CASE_CMP(sendCodec.plname, "telephone-event"))
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
            "telephone-event cannot be registered as send codec");
        return -1;
    }

    
    
    if(!STR_CASE_CMP(sendCodec.plname, "red"))
    {
        
        
        
        if(!ACMCodecDB::ValidPayloadType(sendCodec.pltype))
        {
            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                "Invalid payload-type %d for %s.",
                sendCodec.pltype, sendCodec.plname);
            return -1;
        }
        
        _red_pltype = static_cast<uint8_t>(sendCodec.pltype);
        return 0;
    }

    
    
    if(!STR_CASE_CMP(sendCodec.plname, "CN"))
    {
        
        switch(sendCodec.plfreq)
        {
        case 8000:
            {
                _cng_nb_pltype = static_cast<uint8_t>(sendCodec.pltype);
                break;
            }
        case 16000:
            {
                _cng_wb_pltype = static_cast<uint8_t>(sendCodec.pltype);
                break;
            }
        case 32000:
            {
                _cng_swb_pltype = static_cast<uint8_t>(sendCodec.pltype);
                break;
            }
        default :
            {
                WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                    "RegisterSendCodec() failed, invalid frequency for CNG registeration");
                return -1;
            }
        }

        return 0;
    }

    
    
    
    if(!ACMCodecDB::ValidPayloadType(sendCodec.pltype))
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                "Invalid payload-type %d for %s.",
                sendCodec.pltype, sendCodec.plname);
        return -1;
    }

    
    if(ACMCodecDB::codec_settings_[codecID].channel_support < sendCodec.channels)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                "%d number of channels not supportedn for %s.",
                sendCodec.channels, sendCodec.plname);
        return -1;
    }

    
    if (sendCodec.channels == 2)
    {
        _stereoSend = true;
    }

    
    bool oldCodecFamily;
    if(_sendCodecRegistered)
    {
        int sendCodecMirrorID;
        int sendCodecID =
                ACMCodecDB::CodecNumber(&_sendCodecInst, &sendCodecMirrorID);
        assert(sendCodecID >= 0);
        oldCodecFamily = (sendCodecID == codecID) || (mirrorId == sendCodecMirrorID);
    }
    else
    {
        oldCodecFamily = false;
    }

    
    if (!oldCodecFamily)
    {
        if(_codecs[mirrorId] == NULL)
        {

            _codecs[mirrorId] = CreateCodec(sendCodec);
            if(_codecs[mirrorId] == NULL)
            {
                WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                    "Cannot Create the codec");
                return -1;
            }
            _mirrorCodecIdx[mirrorId] = mirrorId;
        }

        if(mirrorId != codecID)
        {
            _codecs[codecID] = _codecs[mirrorId];
            _mirrorCodecIdx[codecID] = mirrorId;
        }

        ACMGenericCodec* tmpCodecPtr = _codecs[codecID];
        WebRtc_Word16 status;
        WebRtcACMCodecParams codecParams;

        memcpy(&(codecParams.codecInstant), &sendCodec,
            sizeof(CodecInst));
        codecParams.enableVAD = _vadEnabled;
        codecParams.enableDTX = _dtxEnabled;
        codecParams.vadMode   = _vadMode;
        
        status = tmpCodecPtr->InitEncoder(&codecParams, true);

        
        if (status == 1) {
            _vadEnabled = true;
        } else if (status < 0)
        {
            

            
            
            if(!_sendCodecRegistered)
            {
                _currentSendCodecIdx = -1;     
                WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                    "Cannot Initialize the encoder No Encoder is registered");
            }
            else
            {
                WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                    "Cannot Initialize the encoder, continue encoding \
with the previously registered codec");
            }
            return -1;
        }

        
        
        if(_sendCodecRegistered)
        {
            
            
            _isFirstRED = true;

            if(tmpCodecPtr->SetVAD(_dtxEnabled, _vadEnabled, _vadMode) < 0){
                
                _vadEnabled = false;
                _dtxEnabled = false;
            }

        }

        _currentSendCodecIdx = codecID;
        _sendCodecRegistered = true;
        memcpy(&_sendCodecInst, &sendCodec, sizeof(CodecInst));
        _previousPayloadType = _sendCodecInst.pltype;
        return 0;
    }
    else
    {
        
        
        
        bool forceInit = false;

        if(mirrorId != codecID)
        {
            _codecs[codecID] = _codecs[mirrorId];
            _mirrorCodecIdx[codecID] = mirrorId;
        }

        
        if(sendCodec.pltype != _sendCodecInst.pltype)
        {
            
            
            
            if(!ACMCodecDB::ValidPayloadType(sendCodec.pltype))
            {
                WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                    "Out of range payload type");
                return -1;
            }

        }

        
        
        
        
        
        if(_sendCodecInst.plfreq != sendCodec.plfreq)
        {
            forceInit = true;

            
            _isFirstRED = true;
        }

        
        
        if(_sendCodecInst.pacsize != sendCodec.pacsize)
        {
            forceInit = true;
        }
        if(_sendCodecInst.channels != sendCodec.channels)
        {
            forceInit = true;
        }

        if(forceInit)
        {
            WebRtcACMCodecParams codecParams;

            memcpy(&(codecParams.codecInstant), &sendCodec,
                sizeof(CodecInst));
            codecParams.enableVAD = _vadEnabled;
            codecParams.enableDTX = _dtxEnabled;
            codecParams.vadMode   = _vadMode;

            
            if(_codecs[_currentSendCodecIdx]->InitEncoder(&codecParams, true) < 0)
            {
                WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                    "Could not change the codec packet-size.");
                return -1;
            }

            _sendCodecInst.plfreq = sendCodec.plfreq;
            _sendCodecInst.pacsize = sendCodec.pacsize;
            _sendCodecInst.channels = sendCodec.channels;
        }

        
        
        _sendCodecInst.pltype = sendCodec.pltype;

        
        if(sendCodec.rate != _sendCodecInst.rate)
        {
            if(_codecs[codecID]->SetBitRate(sendCodec.rate) < 0)
            {
                WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                    "Could not change the codec rate.");
                return -1;
            }
            _sendCodecInst.rate = sendCodec.rate;
        }
        _previousPayloadType = _sendCodecInst.pltype;

        return 0;
    }
}


WebRtc_Word32
AudioCodingModuleImpl::SendCodec(
    CodecInst& currentSendCodec) const
{
    WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, _id,
        "SendCodec()");
    CriticalSectionScoped lock(*_acmCritSect);

    if(!_sendCodecRegistered)
    {
        WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, _id,
            "SendCodec Failed, no codec is registered");

        return -1;
    }
    WebRtcACMCodecParams encoderParam;
    _codecs[_currentSendCodecIdx]->EncoderParams(&encoderParam);
    encoderParam.codecInstant.pltype = _sendCodecInst.pltype;
    memcpy(&currentSendCodec, &(encoderParam.codecInstant),
        sizeof(CodecInst));

    return 0;
}


WebRtc_Word32
AudioCodingModuleImpl::SendFrequency() const
{
    WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, _id,
        "SendFrequency()");
    CriticalSectionScoped lock(*_acmCritSect);

    if(!_sendCodecRegistered)
    {
        WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, _id,
            "SendFrequency Failed, no codec is registered");

        return -1;
    }

    return _sendCodecInst.plfreq;
}




WebRtc_Word32
AudioCodingModuleImpl::SendBitrate() const
{
    CriticalSectionScoped lock(*_acmCritSect);

    if(!_sendCodecRegistered)
    {
        WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, _id,
            "SendBitrate Failed, no codec is registered");

        return -1;
    }

    WebRtcACMCodecParams encoderParam;
    _codecs[_currentSendCodecIdx]->EncoderParams(&encoderParam);

    return encoderParam.codecInstant.rate;
}



WebRtc_Word32
AudioCodingModuleImpl::SetReceivedEstimatedBandwidth(
    const WebRtc_Word32  bw )
{
    return _codecs[_currentSendCodecIdx]->SetEstimatedBandwidth(bw);
}



WebRtc_Word32
AudioCodingModuleImpl::RegisterTransportCallback(
    AudioPacketizationCallback* transport)
{
    CriticalSectionScoped lock(*_callbackCritSect);
    _packetizationCallback = transport;
    return 0;
}



WebRtc_Word32
AudioCodingModuleImpl::RegisterIncomingMessagesCallback(
#ifndef WEBRTC_DTMF_DETECTION
    AudioCodingFeedback* ,
    const ACMCountries   )
{
    return -1;
#else
    AudioCodingFeedback* incomingMessagesCallback,
    const ACMCountries   cpt)
{
    WebRtc_Word16 status = 0;

    
    {
        CriticalSectionScoped lock(*_callbackCritSect);
        _dtmfCallback = incomingMessagesCallback;
    }
    
    {
        CriticalSectionScoped lock(*_acmCritSect);
        
        if(incomingMessagesCallback == NULL)
        {
            
            if(_dtmfDetector != NULL)
            {
                delete _dtmfDetector;
                _dtmfDetector = NULL;
            }
            status = 0;
        }
        else
        {
            status = 0;
            if(_dtmfDetector == NULL)
            {
                _dtmfDetector = new(ACMDTMFDetection);
                if(_dtmfDetector == NULL)
                {
                    status = -1;
                }
            }
            if(status >= 0)
            {
                status = _dtmfDetector->Enable(cpt);
                if(status < 0)
                {
                    
                    
                    delete _dtmfDetector;
                    _dtmfDetector = NULL;
                }
            }
        }
    }
    
    if((status < 0))
    {
        
        CriticalSectionScoped lock(*_callbackCritSect);
        _dtmfCallback = NULL;
    }

    return status;
#endif
}



WebRtc_Word32
AudioCodingModuleImpl::Add10MsData(
    const AudioFrame& audioFrame)
{
    
    CriticalSectionScoped lock(*_acmCritSect);
    if(!HaveValidEncoder("Add10MsData"))
    {
        return -1;
    }

    if(audioFrame._payloadDataLengthInSamples == 0)
    {
        assert(false);
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
            "Cannot Add 10 ms audio, payload length is zero");
        return -1;
    }
    
    if((audioFrame._frequencyInHz  != 8000)  &&
        (audioFrame._frequencyInHz != 16000) &&
        (audioFrame._frequencyInHz != 32000) &&
        (audioFrame._frequencyInHz != 48000))
    {
        assert(false);
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
            "Cannot Add 10 ms audio, input frequency not valid");
        return -1;
    }


    
    if((audioFrame._frequencyInHz/ 100) !=
        audioFrame._payloadDataLengthInSamples)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
            "Cannot Add 10 ms audio, input frequency and length doesn't \
match");
        return -1;
    }

    
    
    
    bool resamplingRequired =
        ((WebRtc_Word32)audioFrame._frequencyInHz != _sendCodecInst.plfreq);

    
    
    WebRtc_Word16 audio[WEBRTC_10MS_PCM_AUDIO];
    int audio_channels = _sendCodecInst.channels;
    if (audioFrame._audioChannel != _sendCodecInst.channels) {
      if (_sendCodecInst.channels == 2) {
        
        for (int k = 0; k < audioFrame._payloadDataLengthInSamples; k++) {
          audio[k * 2] = audioFrame._payloadData[k];
          audio[(k * 2) + 1] = audioFrame._payloadData[k];
        }
      } else if (_sendCodecInst.channels == 1) {
        
        
        for (int k = 0; k < audioFrame._payloadDataLengthInSamples; k++) {
          audio[k] = (audioFrame._payloadData[k * 2] +
              audioFrame._payloadData[(k * 2) + 1]) >> 1;
        }
      }
    } else {
      
      size_t length = static_cast<size_t>(
          audioFrame._payloadDataLengthInSamples * audio_channels);
      memcpy(audio, audioFrame._payloadData, length * sizeof(WebRtc_UWord16));
    }

    WebRtc_UWord32 currentTimestamp;
    WebRtc_Word32 status;
    
    if(resamplingRequired)
    {
        WebRtc_Word16 resampledAudio[WEBRTC_10MS_PCM_AUDIO];
        WebRtc_Word32 sendPlFreq = _sendCodecInst.plfreq;
        WebRtc_UWord32 diffInputTimestamp;
        WebRtc_Word16 newLengthSmpl;

        
        if(_lastInTimestamp > audioFrame._timeStamp)
        {
            
            diffInputTimestamp = ((WebRtc_UWord32)0xFFFFFFFF - _lastInTimestamp)
                + audioFrame._timeStamp;
        }
        else
        {
            diffInputTimestamp = audioFrame._timeStamp - _lastInTimestamp;
        }
        currentTimestamp = _lastTimestamp + (WebRtc_UWord32)(diffInputTimestamp *
            ((double)_sendCodecInst.plfreq / (double)audioFrame._frequencyInHz));

         newLengthSmpl = _inputResampler.Resample10Msec(
             audio, audioFrame._frequencyInHz, resampledAudio, sendPlFreq,
             audio_channels);

        if(newLengthSmpl < 0)
        {
            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                "Cannot add 10 ms audio, resmapling failed");
            return -1;
        }
        status = _codecs[_currentSendCodecIdx]->Add10MsData(currentTimestamp,
            resampledAudio, newLengthSmpl, audio_channels);
    }
    else
    {
        currentTimestamp = audioFrame._timeStamp;

        status = _codecs[_currentSendCodecIdx]->Add10MsData(currentTimestamp,
            audio, audioFrame._payloadDataLengthInSamples,
            audio_channels);
    }
    _lastInTimestamp = audioFrame._timeStamp;
    _lastTimestamp = currentTimestamp;
    return status;
}





bool
AudioCodingModuleImpl::FECStatus() const
{
    CriticalSectionScoped lock(*_acmCritSect);
    return _fecEnabled;
}


WebRtc_Word32
AudioCodingModuleImpl::SetFECStatus(
#ifdef WEBRTC_CODEC_RED
    const bool enableFEC)
{
    CriticalSectionScoped lock(*_acmCritSect);

    if (_fecEnabled != enableFEC)
    {
        
        memset(_redBuffer, 0, MAX_PAYLOAD_SIZE_BYTE);

        
        _fragmentation->fragmentationVectorSize = 2;
        _fragmentation->fragmentationOffset[0] = 0;
        _fragmentation->fragmentationOffset[1] = MAX_PAYLOAD_SIZE_BYTE;
        memset(_fragmentation->fragmentationLength, 0, sizeof(WebRtc_UWord32) * 2);
        memset(_fragmentation->fragmentationTimeDiff, 0, sizeof(WebRtc_UWord16) * 2);
        memset(_fragmentation->fragmentationPlType, 0, sizeof(WebRtc_UWord8) * 2);

        
        _fecEnabled = enableFEC;
    }
    _isFirstRED = true; 
    return 0;
#else
    const bool /* enableFEC */)
{
    _fecEnabled = false;
    WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceAudioCoding, _id,
               "  WEBRTC_CODEC_RED is undefined => _fecEnabled = %d", _fecEnabled);
    return -1;
#endif
}






WebRtc_Word32
AudioCodingModuleImpl::SetVAD(
    const bool       enableDTX,
    const bool       enableVAD,
    const ACMVADMode vadMode)
{
    CriticalSectionScoped lock(*_acmCritSect);

    
    if((vadMode != VADNormal)      &&
       (vadMode != VADLowBitrate) &&
       (vadMode != VADAggr)       &&
       (vadMode != VADVeryAggr))
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
            "Invalid VAD Mode %d, no change is made to VAD/DTX status",
            (int)vadMode);
        return -1;
    }

    
    if(HaveValidEncoder("SetVAD")) {
        WebRtc_Word16 status =
                _codecs[_currentSendCodecIdx]->SetVAD(enableDTX, enableVAD, vadMode);
        if(status == 1) {
            
            _vadEnabled = true;
            _dtxEnabled = enableDTX;
            _vadMode = vadMode;

            return 0;
        } else if (status < 0) {
            
            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
            "SetVAD failed");

            _vadEnabled = false;
            _dtxEnabled = false;

            return -1;
        }
    }

    _vadEnabled = enableVAD;
    _dtxEnabled = enableDTX;
    _vadMode = vadMode;

    return 0;
}

WebRtc_Word32
AudioCodingModuleImpl::VAD(
    bool&       dtxEnabled,
    bool&       vadEnabled,
    ACMVADMode& vadMode) const
{
    CriticalSectionScoped lock(*_acmCritSect);

    dtxEnabled = _dtxEnabled;
    vadEnabled = _vadEnabled;
    vadMode = _vadMode;

    return 0;
}





WebRtc_Word32
AudioCodingModuleImpl::InitializeReceiver()
{
    CriticalSectionScoped lock(*_acmCritSect);
    return InitializeReceiverSafe();
}


WebRtc_Word32
AudioCodingModuleImpl::InitializeReceiverSafe()
{
    
    
    
    if(_receiverInitialized)
    {
        for(int codecCntr = 0; codecCntr < ACMCodecDB::kNumCodecs; codecCntr++)
        {
            if(UnregisterReceiveCodecSafe(codecCntr) < 0)
            {
                WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                    "InitializeReceiver() failed, Could not unregister codec");
                return -1;
            }
        }
    }
    if (_netEq.Init() != 0)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
            "InitializeReceiver() failed, Could not initialize NetEQ");
        return -1;
    }
    _netEq.SetUniqueId(_id);
    if (_netEq.AllocatePacketBuffer(ACMCodecDB::NetEQDecoders(),
        ACMCodecDB::kNumCodecs) != 0)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
            "NetEQ cannot allocatePacket Buffer");
        return -1;
    }

    
    int regInNeteq = 0;
    for (int i = (ACMCodecDB::kNumCodecs - 1); i>-1; i--) {
        if((STR_CASE_CMP(ACMCodecDB::database_[i].plname, "red") == 0)) {
            regInNeteq = 1;
        } else if ((STR_CASE_CMP(ACMCodecDB::database_[i].plname, "CN") == 0)) {
            regInNeteq = 1;
        }

        if (regInNeteq == 1) {
           if(RegisterRecCodecMSSafe(ACMCodecDB::database_[i], i, i,
                ACMNetEQ::masterJB) < 0)
            {
                WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                    "Cannot register master codec.");
                return -1;
            }
            _registeredPlTypes[i] = ACMCodecDB::database_[i].pltype;
            regInNeteq = 0;
        }
    }
    _cng_reg_receiver = true;

    _receiverInitialized = true;
    return 0;
}


WebRtc_Word32
AudioCodingModuleImpl::ResetDecoder()
{
    CriticalSectionScoped lock(*_acmCritSect);

    for(int codecCntr = 0; codecCntr < ACMCodecDB::kMaxNumCodecs; codecCntr++)
    {
        if((_codecs[codecCntr] != NULL) && (_registeredPlTypes[codecCntr] != -1))
        {
            if(_codecs[codecCntr]->ResetDecoder(_registeredPlTypes[codecCntr]) < 0)
            {
                WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                    "ResetDecoder failed:");
                return -1;
            }
        }
    }
    return _netEq.FlushBuffers();
}


WebRtc_Word32
AudioCodingModuleImpl::ReceiveFrequency() const
{
    WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, _id,
        "ReceiveFrequency()");
    WebRtcACMCodecParams codecParams;

    CriticalSectionScoped lock(*_acmCritSect);
    if(DecoderParamByPlType(_lastRecvAudioCodecPlType, codecParams) < 0)
    {
        return _netEq.CurrentSampFreqHz();
    }
    else
    {
        return codecParams.codecInstant.plfreq;
    }
}


WebRtc_Word32
AudioCodingModuleImpl::PlayoutFrequency() const
{
    WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, _id,
        "PlayoutFrequency()");

    CriticalSectionScoped lock(*_acmCritSect);

    return _netEq.CurrentSampFreqHz();
}




WebRtc_Word32
AudioCodingModuleImpl::RegisterReceiveCodec(
    const CodecInst& receiveCodec)
{
    CriticalSectionScoped lock(*_acmCritSect);

    if(receiveCodec.channels > 2)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
            "More than 2 audio channel is not supported.");
        return -1;
    }

    int mirrorId;
    int codecId = ACMCodecDB::ReceiverCodecNumber(&receiveCodec, &mirrorId);

    if(codecId < 0 || codecId >= ACMCodecDB::kNumCodecs)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
            "Wrong codec params to be registered as receive codec");
        return -1;
    }
    
    if(!ACMCodecDB::ValidPayloadType(receiveCodec.pltype))
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                "Invalid payload-type %d for %s.",
                receiveCodec.pltype, receiveCodec.plname);
        return -1;
    }

    if(!_receiverInitialized)
    {
        if(InitializeReceiverSafe() < 0)
        {
            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                "Cannot initialize reciver, so failed registering a codec.");
            return -1;
        }
    }

    
    
    if ((_registeredPlTypes[codecId] == receiveCodec.pltype) &&
        (STR_CASE_CMP(receiveCodec.plname, "CN") == 0)) {
      
      
      return 0;
    } else if (_registeredPlTypes[codecId] != -1) {
        if(UnregisterReceiveCodecSafe(codecId) < 0) {
            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                "Cannot register master codec.");
            return -1;
        }
    }

    if(RegisterRecCodecMSSafe(receiveCodec, codecId, mirrorId,
        ACMNetEQ::masterJB) < 0)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
            "Cannot register master codec.");
        return -1;
    }

    
    
    if (STR_CASE_CMP(receiveCodec.plname, "CN") == 0) {
        _cng_reg_receiver = true;
        if (_stereoReceiveRegistered) {
            
            
            if(RegisterRecCodecMSSafe(receiveCodec, codecId, mirrorId,
                                      ACMNetEQ::slaveJB) < 0) {
               WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding,
                            _id, "Cannot register slave codec.");
               return -1;
            }
            _stereoReceive[codecId] = true;
            _registeredPlTypes[codecId] = receiveCodec.pltype;
            return 0;
        }
    }

    
    
    if(receiveCodec.channels == 2)
    {
        if(_netEq.NumSlaves() < 1)
        {
            if(_netEq.AddSlave(ACMCodecDB::NetEQDecoders(),
                   ACMCodecDB::kNumCodecs) < 0)
            {
                WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding,
                             _id, "Cannot Add Slave jitter buffer to NetEQ.");
                return -1;
            }
        }

        
        
        if (!_stereoReceiveRegistered) {
            bool reg_in_neteq = false;
            for (int i = (ACMCodecDB::kNumCodecs - 1); i > -1; i--) {
                if (STR_CASE_CMP(ACMCodecDB::database_[i].plname, "RED") == 0) {
                    if (_registeredPlTypes[i] != -1) {
                        
                        
                        _stereoReceive[i] = true;
                        reg_in_neteq = true;
                    }
                } else if (STR_CASE_CMP(ACMCodecDB::database_[i].plname, "CN")
                    == 0) {
                    if (_cng_reg_receiver) {
                        
                        
                        _stereoReceive[i] = true;
                        reg_in_neteq = true;
                    }
                }

                if (reg_in_neteq) {
                    CodecInst tmp_codec;
                    memcpy(&tmp_codec, &ACMCodecDB::database_[i],
                           sizeof(CodecInst));
                    tmp_codec.pltype = _registeredPlTypes[i];
                    
                    
                    if(RegisterRecCodecMSSafe(tmp_codec, i, i,
                                              ACMNetEQ::slaveJB) < 0) {
                        WEBRTC_TRACE(webrtc::kTraceError,
                                     webrtc::kTraceAudioCoding, _id,
                                     "Cannot register slave codec.");
                        return -1;
                    }
                    reg_in_neteq = false;
                }
            }
        }

        if(RegisterRecCodecMSSafe(receiveCodec, codecId, mirrorId,
            ACMNetEQ::slaveJB) < 0)
        {
            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                "Cannot register slave codec.");
            return -1;
        }

        
        
        if((_stereoReceive[codecId] == false) &&
            (_lastRecvAudioCodecPlType == receiveCodec.pltype))
        {
            _lastRecvAudioCodecPlType = -1;
        }
        _stereoReceive[codecId] = true;
        _stereoReceiveRegistered = true;
    }
    else
    {
        _stereoReceive[codecId] = false;
    }

    _registeredPlTypes[codecId] = receiveCodec.pltype;

    if(!STR_CASE_CMP(receiveCodec.plname, "RED"))
    {
        _receiveREDPayloadType = receiveCodec.pltype;
    }
    return 0;
}



WebRtc_Word32
AudioCodingModuleImpl::RegisterRecCodecMSSafe(
    const CodecInst& receiveCodec,
    WebRtc_Word16    codecId,
    WebRtc_Word16    mirrorId,
    ACMNetEQ::JB     jitterBuffer)
{
    ACMGenericCodec** codecArray;
    if(jitterBuffer == ACMNetEQ::masterJB)
    {
        codecArray = &_codecs[0];
    }
    else if(jitterBuffer == ACMNetEQ::slaveJB)
    {
        codecArray = &_slaveCodecs[0];
        if (_codecs[codecId]->IsTrueStereoCodec()) {
          
          
          _slaveCodecs[mirrorId] = _codecs[mirrorId];
          _mirrorCodecIdx[mirrorId] = mirrorId;
        }
    }
    else
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
            "RegisterReceiveCodecMSSafe failed, jitterBuffer is neither master or slave ");
        return -1;
    }

    if (codecArray[mirrorId] == NULL)
    {
        codecArray[mirrorId] = CreateCodec(receiveCodec);
        if(codecArray[mirrorId] == NULL)
        {
            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                "Cannot create codec to register as receive codec");
            return -1;
        }
        _mirrorCodecIdx[mirrorId] = mirrorId;
    }
    if(mirrorId != codecId)
    {
        codecArray[codecId] = codecArray[mirrorId];
        _mirrorCodecIdx[codecId] = mirrorId;
    }

    codecArray[codecId]->SetIsMaster(jitterBuffer == ACMNetEQ::masterJB);

    WebRtc_Word16 status = 0;
    bool registerInNetEq = true;
    WebRtcACMCodecParams codecParams;
    memcpy(&(codecParams.codecInstant), &receiveCodec,
        sizeof(CodecInst));
    codecParams.enableVAD = false;
    codecParams.enableDTX = false;
    codecParams.vadMode   = VADNormal;
    if (!codecArray[codecId]->DecoderInitialized())
    {
        
        status = codecArray[codecId]->InitDecoder(&codecParams, true);
        if(status < 0)
        {
            
            
            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                "could not initialize the receive codec, codec not registered");

            return -1;
        }
    }
    else if(mirrorId != codecId)
    {
        
        

        codecArray[codecId]->SaveDecoderParam(&codecParams);
    }
    if (registerInNetEq)
    {
        if(codecArray[codecId]->RegisterInNetEq(&_netEq, receiveCodec)
            != 0)
        {
            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                "Receive codec could not be registered in NetEQ");

            return -1;
        }
        
        
        codecArray[codecId]->SaveDecoderParam(&codecParams);
    }

    return status;
}




WebRtc_Word32
AudioCodingModuleImpl::ReceiveCodec(
    CodecInst& currentReceiveCodec) const
{
    WebRtcACMCodecParams decoderParam;
    CriticalSectionScoped lock(*_acmCritSect);

    for(int decCntr = 0; decCntr < ACMCodecDB::kMaxNumCodecs; decCntr++)
    {
        if(_codecs[decCntr] != NULL)
        {
            if(_codecs[decCntr]->DecoderInitialized())
            {
                if(_codecs[decCntr]->DecoderParams(&decoderParam,
                    _lastRecvAudioCodecPlType))
                {
                    memcpy(&currentReceiveCodec, &decoderParam.codecInstant,
                        sizeof(CodecInst));
                    return 0;
                }
            }
        }
    }

    
    
    
    currentReceiveCodec.pltype = -1;
    return -1;
}


WebRtc_Word32
AudioCodingModuleImpl::IncomingPacket(
    const WebRtc_UWord8*   incomingPayload,
    const WebRtc_Word32    payloadLength,
    const WebRtcRTPHeader& rtpInfo)
{
    WebRtcRTPHeader rtp_header;

    memcpy(&rtp_header, &rtpInfo, sizeof(WebRtcRTPHeader));

    if (payloadLength < 0)
    {
        
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
            "IncomingPacket() Error, payload-length cannot be negative");
        return -1;
    }
    {
        
        
        CriticalSectionScoped lock(*_acmCritSect);
#ifdef ACM_QA_TEST
        if(_incomingPL != NULL)
        {
            fwrite(&rtpInfo.header.timestamp,   sizeof(WebRtc_UWord32), 1, _incomingPL);
            fwrite(&rtpInfo.header.payloadType, sizeof(WebRtc_UWord8),  1, _incomingPL);
            fwrite(&payloadLength,              sizeof(WebRtc_Word16),  1, _incomingPL);
        }
#endif

        WebRtc_UWord8 myPayloadType;

        
        if(rtpInfo.header.payloadType == _receiveREDPayloadType)
        {
            
            myPayloadType = incomingPayload[0] & 0x7F;
        }
        else
        {
            myPayloadType = rtpInfo.header.payloadType;
        }

        
        
        if(!rtpInfo.type.Audio.isCNG)
        {
            

            if(myPayloadType != _lastRecvAudioCodecPlType)
            {
                
                
                
                
                for(int i = 0; i < ACMCodecDB::kMaxNumCodecs; i++)
                {
                    if(_registeredPlTypes[i] == myPayloadType)
                    {
                        if(_codecs[i] == NULL)
                        {
                            
                            
                            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                                "IncomingPacket() Error, payload type found but corresponding "
                                "codec is NULL");
                            return -1;
                        }
                        _codecs[i]->UpdateDecoderSampFreq(i);
                        _netEq.SetReceivedStereo(_stereoReceive[i]);
                        _current_receive_codec_idx = i;

                        
                        
                        if ((_stereoReceive[i] && (_expected_channels == 1)) ||
                            (!_stereoReceive[i] && (_expected_channels == 2))) {
                          _netEq.FlushBuffers();
                          _codecs[i]->ResetDecoder(myPayloadType);
                        }

                        
                        
                        if (_stereoReceive[i]) {
                          _expected_channels = 2;
                        } else {
                          _expected_channels = 1;
                        }

                        
                        _prev_received_channel = 0;

                        break;
                    }
                }
            }
            _lastRecvAudioCodecPlType = myPayloadType;
        }
    }

    
    
    if (_expected_channels == 2) {
      
      WebRtc_Word32 length = payloadLength;
      WebRtc_UWord8 payload[kMaxPacketSize];
      assert(payloadLength <= kMaxPacketSize);
      memcpy(payload, incomingPayload, payloadLength);
      _codecs[_current_receive_codec_idx]->SplitStereoPacket(payload, &length);
      rtp_header.type.Audio.channel = 2;
      
      return _netEq.RecIn(payload, length, rtp_header);
    } else {
      return _netEq.RecIn(incomingPayload, payloadLength, rtp_header);
    }
}


WebRtc_Word32
AudioCodingModuleImpl::SetMinimumPlayoutDelay(
    const WebRtc_Word32 timeMs)
{
    if((timeMs < 0) || (timeMs > 1000))
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
            "Delay must be in the range of 0-1000 milliseconds.");
        return -1;
    }
    return _netEq.SetExtraDelay(timeMs);
}


bool
AudioCodingModuleImpl::DtmfPlayoutStatus() const
{
#ifndef WEBRTC_CODEC_AVT
    return false;
#else
    return _netEq.AVTPlayout();
#endif
}



WebRtc_Word32
AudioCodingModuleImpl::SetDtmfPlayoutStatus(
#ifndef WEBRTC_CODEC_AVT
    const bool )
{
    WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceAudioCoding, _id,
        "SetDtmfPlayoutStatus() failed: AVT is not supported.");
    return -1;
#else
    const bool enable)
{
    return _netEq.SetAVTPlayout(enable);
#endif
}




WebRtc_Word32
AudioCodingModuleImpl::DecoderEstimatedBandwidth() const
{
    CodecInst codecInst;
    WebRtc_Word16 codecID = -1;
    int plTypWB;
    int plTypSWB;

    
    for(int codecCntr = 0; codecCntr < ACMCodecDB::kNumCodecs; codecCntr++)
    {
        
        ACMCodecDB::Codec(codecCntr, &codecInst);

        if(!STR_CASE_CMP(codecInst.plname, "isac"))
        {
            codecID = 1;
            plTypWB = codecInst.pltype;

            ACMCodecDB::Codec(codecCntr+1, &codecInst);
            plTypSWB = codecInst.pltype;

            break;
        }
    }

    if(codecID < 0)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
            "DecoderEstimatedBandwidth failed");
        return -1;
    }

    if ((_lastRecvAudioCodecPlType == plTypWB) || (_lastRecvAudioCodecPlType == plTypSWB))
    {
        return _codecs[codecID]->GetEstimatedBandwidth();
    } else {
        return -1;
    }
}


WebRtc_Word32
AudioCodingModuleImpl::SetPlayoutMode(
    const AudioPlayoutMode mode)
{
    if((mode  != voice) &&
        (mode != fax)   &&
        (mode != streaming))
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
            "Invalid playout mode.");
        return -1;
    }
    return _netEq.SetPlayoutMode(mode);
}


AudioPlayoutMode
AudioCodingModuleImpl::PlayoutMode() const
{
    return _netEq.PlayoutMode();
}




WebRtc_Word32
AudioCodingModuleImpl::PlayoutData10Ms(
    const WebRtc_Word32 desiredFreqHz,
    AudioFrame&         audioFrame)
{
    bool stereoMode;

     
    if (_netEq.RecOut(_audioFrame) != 0)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
            "PlayoutData failed, RecOut Failed");
        return -1;
    }

    audioFrame._audioChannel = _audioFrame._audioChannel;
    audioFrame._vadActivity  = _audioFrame._vadActivity;
    audioFrame._speechType   = _audioFrame._speechType;

    stereoMode =  (_audioFrame._audioChannel > 1);
    
    

    const WebRtc_UWord16 recvFreq = static_cast<WebRtc_UWord16>(_audioFrame._frequencyInHz);
    bool toneDetected = false;
    WebRtc_Word16 lastDetectedTone;
    WebRtc_Word16 tone;

     
    
    
    
    {
        CriticalSectionScoped lock(*_acmCritSect);

        if ((recvFreq != desiredFreqHz) && (desiredFreqHz != -1))
        {
            
            WebRtc_Word16 tmpLen = _outputResampler.Resample10Msec(
                _audioFrame._payloadData, recvFreq, audioFrame._payloadData, desiredFreqHz,
                _audioFrame._audioChannel);

            if(tmpLen < 0)
            {
                WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                    "PlayoutData failed, resampler failed");
                return -1;
            }

            
            audioFrame._payloadDataLengthInSamples = (WebRtc_UWord16)tmpLen;
            
            audioFrame._frequencyInHz = desiredFreqHz;
        }
        else
        {
            memcpy(audioFrame._payloadData, _audioFrame._payloadData,
              _audioFrame._payloadDataLengthInSamples * audioFrame._audioChannel
              * sizeof(WebRtc_Word16));
            
            audioFrame._payloadDataLengthInSamples = _audioFrame._payloadDataLengthInSamples;
            
            audioFrame._frequencyInHz = recvFreq;
        }

        
        if(_dtmfDetector != NULL)
        {
            
            if(audioFrame._frequencyInHz == 8000)
            {
                
                
                if(!stereoMode)
                {
                    _dtmfDetector->Detect(audioFrame._payloadData,
                        audioFrame._payloadDataLengthInSamples,
                        audioFrame._frequencyInHz, toneDetected, tone);
                }
                else
                {
                    
                    WebRtc_Word16 masterChannel[80];
                    for(int n = 0; n < 80; n++)
                    {
                        masterChannel[n] = audioFrame._payloadData[n<<1];
                    }
                    _dtmfDetector->Detect(masterChannel,
                        audioFrame._payloadDataLengthInSamples,
                        audioFrame._frequencyInHz, toneDetected, tone);
                }
            }
            else
            {
                
                if(!stereoMode)
                {
                    _dtmfDetector->Detect(_audioFrame._payloadData,
                        _audioFrame._payloadDataLengthInSamples, recvFreq,
                        toneDetected, tone);
                }
                else
                {
                    WebRtc_Word16 masterChannel[WEBRTC_10MS_PCM_AUDIO];
                    for(int n = 0; n < _audioFrame._payloadDataLengthInSamples; n++)
                    {
                        masterChannel[n] = _audioFrame._payloadData[n<<1];
                    }
                    _dtmfDetector->Detect(masterChannel,
                        _audioFrame._payloadDataLengthInSamples, recvFreq,
                        toneDetected, tone);
                }
            }
        }

        
        
        
        lastDetectedTone = kACMToneEnd;
        if(toneDetected)
        {
            lastDetectedTone = _lastDetectedTone;
            _lastDetectedTone = tone;
        }
    }

    if(toneDetected)
    {
        
        
        CriticalSectionScoped lock(*_callbackCritSect);

        if(_dtmfCallback != NULL)
        {
            if(tone != kACMToneEnd)
            {
                
                _dtmfCallback->IncomingDtmf((WebRtc_UWord8)tone, false);
            }
            else if((tone == kACMToneEnd) &&
                (lastDetectedTone != kACMToneEnd))
            {
                
                
                _dtmfCallback->IncomingDtmf((WebRtc_UWord8)lastDetectedTone,
                    true);
            }
        }
    }

    audioFrame._id = _id;
    audioFrame._volume = -1;
    audioFrame._energy = -1;
    audioFrame._timeStamp = 0;

    return 0;
}







ACMVADMode
AudioCodingModuleImpl::ReceiveVADMode() const
{
    return _netEq.VADMode();
}


WebRtc_Word16
AudioCodingModuleImpl::SetReceiveVADMode(
    const ACMVADMode mode)
{
    return _netEq.SetVADMode(mode);
}





WebRtc_Word32
AudioCodingModuleImpl::NetworkStatistics(
    ACMNetworkStatistics& statistics) const
{
    WebRtc_Word32 status;
    status = _netEq.NetworkStatistics(&statistics);
    return status;
}

void
AudioCodingModuleImpl::DestructEncoderInst(
    void* ptrInst)
{
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceAudioCoding, _id,
        "DestructEncoderInst()");
    if(!HaveValidEncoder("DestructEncoderInst"))
    {
        return;
    }

    _codecs[_currentSendCodecIdx]->DestructEncoderInst(ptrInst);
}

WebRtc_Word16
AudioCodingModuleImpl::AudioBuffer(
    WebRtcACMAudioBuff& audioBuff)
{
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceAudioCoding, _id,
        "AudioBuffer()");
    if(!HaveValidEncoder("AudioBuffer"))
    {
        return -1;
    }

    audioBuff.lastInTimestamp = _lastInTimestamp;
    return _codecs[_currentSendCodecIdx]->AudioBuffer(audioBuff);
}

WebRtc_Word16
AudioCodingModuleImpl::SetAudioBuffer(
    WebRtcACMAudioBuff& audioBuff)
{
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceAudioCoding, _id,
        "SetAudioBuffer()");
    if(!HaveValidEncoder("SetAudioBuffer"))
    {
        return -1;
    }

    return _codecs[_currentSendCodecIdx]->SetAudioBuffer(audioBuff);
}


WebRtc_UWord32
AudioCodingModuleImpl::EarliestTimestamp() const
{
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceAudioCoding, _id,
        "EarliestTimestamp()");
    if(!HaveValidEncoder("EarliestTimestamp"))
    {
        return -1;
    }

    return _codecs[_currentSendCodecIdx]->EarliestTimestamp();
}

WebRtc_Word32
AudioCodingModuleImpl::RegisterVADCallback(
    ACMVADCallback* vadCallback)
{
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceAudioCoding, _id,
        "RegisterVADCallback()");
    CriticalSectionScoped lock(*_callbackCritSect);
    _vadCallback = vadCallback;
    return 0;
}


WebRtc_Word32
AudioCodingModuleImpl::IncomingPayload(
    const WebRtc_UWord8* incomingPayload,
    const WebRtc_Word32  payloadLength,
    const WebRtc_UWord8  payloadType,
    const WebRtc_UWord32 timestamp)
{
    if (payloadLength < 0)
    {
        
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
            "IncomingPacket() Error, payload-length cannot be negative");
        return -1;
    }

    if(_dummyRTPHeader == NULL)
    {
        
        
        WebRtcACMCodecParams codecParams;
        _dummyRTPHeader = new WebRtcRTPHeader;
        if (_dummyRTPHeader == NULL)
        {
            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                "IncomingPacket() Error, out of memory");
            return -1;
        }
        _dummyRTPHeader->header.payloadType = payloadType;
        
        _dummyRTPHeader->header.ssrc = 0;
        _dummyRTPHeader->header.markerBit = false;
        
        _dummyRTPHeader->header.sequenceNumber = rand();
        _dummyRTPHeader->header.timestamp = (((WebRtc_UWord32)rand()) << 16) +
            (WebRtc_UWord32)rand();
        _dummyRTPHeader->type.Audio.channel = 1;

        if(DecoderParamByPlType(payloadType, codecParams) < 0)
        {
            
            
            
            delete _dummyRTPHeader;
            _dummyRTPHeader = NULL;
            return -1;
        }
        _recvPlFrameSizeSmpls = codecParams.codecInstant.pacsize;
    }

    if(payloadType != _dummyRTPHeader->header.payloadType)
    {
        
        
        WebRtcACMCodecParams codecParams;
        if(DecoderParamByPlType(payloadType, codecParams) < 0)
        {
            
            
            return -1;
        }
        _recvPlFrameSizeSmpls = codecParams.codecInstant.pacsize;
        _dummyRTPHeader->header.payloadType = payloadType;
    }

    if(timestamp > 0)
    {
        _dummyRTPHeader->header.timestamp = timestamp;
    }

    
    
    _lastRecvAudioCodecPlType = payloadType;

    
    if(_netEq.RecIn(incomingPayload, payloadLength, (*_dummyRTPHeader)) < 0)
    {
        return -1;
    }

    
    _dummyRTPHeader->header.sequenceNumber++;
    _dummyRTPHeader->header.timestamp += _recvPlFrameSizeSmpls;
    return 0;
}

WebRtc_Word16
AudioCodingModuleImpl::DecoderParamByPlType(
    const WebRtc_UWord8    payloadType,
    WebRtcACMCodecParams&  codecParams) const
{
    CriticalSectionScoped lock(*_acmCritSect);
    for(WebRtc_Word16 codecCntr = 0; codecCntr < ACMCodecDB::kMaxNumCodecs; codecCntr++)
    {
        if(_codecs[codecCntr] != NULL)
        {
            if(_codecs[codecCntr]->DecoderInitialized())
            {
                if(_codecs[codecCntr]->DecoderParams(&codecParams,
                    payloadType))
                {
                    return 0;
                }
            }
        }
    }
    
    
    
    codecParams.codecInstant.plname[0] = '\0';
    codecParams.codecInstant.pacsize   = 0;
    codecParams.codecInstant.rate      = 0;
    codecParams.codecInstant.pltype    = -1;
    return -1;
}



WebRtc_Word16
AudioCodingModuleImpl::DecoderListIDByPlName(
    const char*  payloadName,
    const WebRtc_UWord16 sampFreqHz) const
{
    WebRtcACMCodecParams codecParams;
    CriticalSectionScoped lock(*_acmCritSect);
    for(WebRtc_Word16 codecCntr = 0; codecCntr < ACMCodecDB::kMaxNumCodecs; codecCntr++)
    {
        if((_codecs[codecCntr] != NULL))
        {
            if(_codecs[codecCntr]->DecoderInitialized())
            {
                assert(_registeredPlTypes[codecCntr] >= 0);
                assert(_registeredPlTypes[codecCntr] <= 255);
                _codecs[codecCntr]->DecoderParams(&codecParams,
                    (WebRtc_UWord8)_registeredPlTypes[codecCntr]);
                if(!STR_CASE_CMP(codecParams.codecInstant.plname, payloadName))
                {
                    
                    
                    
                    
                    
                    
                    if((sampFreqHz == 0) ||
                        (codecParams.codecInstant.plfreq == sampFreqHz))
                    {
                        return codecCntr;
                    }
                }
            }
        }
    }
    
    
    return -1;
}

WebRtc_Word32
AudioCodingModuleImpl::LastEncodedTimestamp(WebRtc_UWord32& timestamp) const
{
    CriticalSectionScoped lock(*_acmCritSect);
    if(!HaveValidEncoder("LastEncodedTimestamp"))
    {
        return -1;
    }
    timestamp = _codecs[_currentSendCodecIdx]->LastEncodedTimestamp();
    return 0;
}

WebRtc_Word32
AudioCodingModuleImpl::ReplaceInternalDTXWithWebRtc(bool useWebRtcDTX)
{
    CriticalSectionScoped lock(*_acmCritSect);

    if(!HaveValidEncoder("ReplaceInternalDTXWithWebRtc"))
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
            "Cannot replace codec internal DTX when no send codec is registered.");
        return -1;
    }

    WebRtc_Word32 res = _codecs[_currentSendCodecIdx]->ReplaceInternalDTX(useWebRtcDTX);
    
    if(res == 1)
    {
        _vadEnabled = true;
    } else if(res < 0)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
            "Failed to set ReplaceInternalDTXWithWebRtc(%d)", useWebRtcDTX);
        return res;
    }

    return 0;
}

WebRtc_Word32
AudioCodingModuleImpl::IsInternalDTXReplacedWithWebRtc(bool& usesWebRtcDTX)
{
    CriticalSectionScoped lock(*_acmCritSect);

    if(!HaveValidEncoder("IsInternalDTXReplacedWithWebRtc"))
    {
        return -1;
    }
    if(_codecs[_currentSendCodecIdx]->IsInternalDTXReplaced(&usesWebRtcDTX) < 0)
    {
        return -1;
    }
    return 0;
}


WebRtc_Word32
AudioCodingModuleImpl::SetISACMaxRate(
    const WebRtc_UWord32 maxRateBitPerSec)
{
    CriticalSectionScoped lock(*_acmCritSect);

    if(!HaveValidEncoder("SetISACMaxRate"))
    {
        return -1;
    }

    return _codecs[_currentSendCodecIdx]->SetISACMaxRate(maxRateBitPerSec);
}


WebRtc_Word32
AudioCodingModuleImpl::SetISACMaxPayloadSize(
    const WebRtc_UWord16 maxPayloadLenBytes)
{
    CriticalSectionScoped lock(*_acmCritSect);

    if(!HaveValidEncoder("SetISACMaxPayloadSize"))
    {
        return -1;
    }

    return _codecs[_currentSendCodecIdx]->SetISACMaxPayloadSize(maxPayloadLenBytes);
}

WebRtc_Word32
AudioCodingModuleImpl::ConfigISACBandwidthEstimator(
    const WebRtc_UWord8  initFrameSizeMsec,
    const WebRtc_UWord16 initRateBitPerSec,
    const bool           enforceFrameSize)
{
    CriticalSectionScoped lock(*_acmCritSect);

    if(!HaveValidEncoder("ConfigISACBandwidthEstimator"))
    {
        return -1;
    }

    return _codecs[_currentSendCodecIdx]->ConfigISACBandwidthEstimator(
        initFrameSizeMsec, initRateBitPerSec, enforceFrameSize);
}

WebRtc_Word32
AudioCodingModuleImpl::SetBackgroundNoiseMode(
    const ACMBackgroundNoiseMode mode)
{
    if((mode < On) ||
        (mode > Off))
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
            "The specified background noise is out of range.\n");
        return -1;
    }
    return _netEq.SetBackgroundNoiseMode(mode);
}

WebRtc_Word32
AudioCodingModuleImpl::BackgroundNoiseMode(
    ACMBackgroundNoiseMode& mode)
{
    return _netEq.BackgroundNoiseMode(mode);
}

WebRtc_Word32
AudioCodingModuleImpl::PlayoutTimestamp(
    WebRtc_UWord32& timestamp)
{
    WEBRTC_TRACE(webrtc::kTraceStream, webrtc::kTraceAudioCoding, _id,
        "PlayoutTimestamp()");
    return _netEq.PlayoutTimestamp(timestamp);
}

bool
AudioCodingModuleImpl::HaveValidEncoder(
    const char* callerName) const
{
    if((!_sendCodecRegistered) ||
        (_currentSendCodecIdx < 0) ||
        (_currentSendCodecIdx >= ACMCodecDB::kNumCodecs))
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
            "%s failed: No send codec is registered.", callerName);
        return false;
    }
    if((_currentSendCodecIdx < 0) ||
        (_currentSendCodecIdx >= ACMCodecDB::kNumCodecs))
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
            "%s failed: Send codec index out of range.", callerName);
        return false;
    }
    if(_codecs[_currentSendCodecIdx] == NULL)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
            "%s failed: Send codec is NULL pointer.", callerName);
        return false;
    }
    return true;
}

WebRtc_Word32
AudioCodingModuleImpl::UnregisterReceiveCodec(
    const WebRtc_Word16 payloadType)
{
    CriticalSectionScoped lock(*_acmCritSect);
    WebRtc_Word16 codecID;

    
    for (codecID = 0; codecID < ACMCodecDB::kMaxNumCodecs; codecID++)
    {
        if (_registeredPlTypes[codecID] == payloadType)
        {
            
            break;
        }
    }

    if(codecID >= ACMCodecDB::kNumCodecs)
    {
        
        return 0;
    }

    
    return UnregisterReceiveCodecSafe(codecID);
}

WebRtc_Word32
AudioCodingModuleImpl::UnregisterReceiveCodecSafe(
    const WebRtc_Word16 codecID)
{
    const WebRtcNetEQDecoder *neteqDecoder = ACMCodecDB::NetEQDecoders();
    WebRtc_Word16 mirrorID = ACMCodecDB::MirrorID(codecID);
    bool stereo_receiver = false;

    if(_codecs[codecID] != NULL)
    {
        if(_registeredPlTypes[codecID] != -1)
        {
            
            stereo_receiver = _stereoReceive[codecID];

            
            if(_netEq.RemoveCodec(neteqDecoder[codecID], _stereoReceive[codecID]) < 0)
            {
                CodecInst codecInst;
                ACMCodecDB::Codec(codecID, &codecInst);
                WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, _id,
                    "Unregistering %s-%d from NetEQ failed.",
                    codecInst.plname, codecInst.plfreq);
                return -1;
            }

            
            
            if(STR_CASE_CMP(ACMCodecDB::database_[codecID].plname, "CN") == 0)
            {
                
                
                
                for (int i=-2; i<3; i++)
                {
                    if (STR_CASE_CMP(ACMCodecDB::database_[codecID+i].plname, "CN") == 0)
                    {
                        if(_stereoReceive[codecID+i])
                        {
                            _stereoReceive[codecID+i] = false;
                        }
                        _registeredPlTypes[codecID+i] = -1;
                    }
                }
                _cng_reg_receiver = false;
            } else
            {
                if(codecID == mirrorID)
                {
                    _codecs[codecID]->DestructDecoder();
                    if(_stereoReceive[codecID])
                    {
                        _slaveCodecs[codecID]->DestructDecoder();
                        _stereoReceive[codecID] = false;

                    }
                }
            }

            
            if (stereo_receiver) {
                bool no_stereo = true;

                for (int i = 0; i < ACMCodecDB::kNumCodecs; i++) {
                    if (_stereoReceive[i]) {
                        
                        no_stereo = false;
                       break;
                    }
                }

                
                if (no_stereo) {
                  _stereoReceiveRegistered = false;
                }
            }
        }
    }

    if(_registeredPlTypes[codecID] == _receiveREDPayloadType)
    {
        
        
        _receiveREDPayloadType = 255;
    }
    _registeredPlTypes[codecID] = -1;

    return 0;
}

WebRtc_Word32
AudioCodingModuleImpl::REDPayloadISAC(
    const WebRtc_Word32  isacRate,
    const WebRtc_Word16  isacBwEstimate,
    WebRtc_UWord8*       payload,
    WebRtc_Word16*       payloadLenByte)
{
   if(!HaveValidEncoder("EncodeData"))
   {
       return -1;
   }
   WebRtc_Word16 status;

   status = _codecs[_currentSendCodecIdx]->REDPayloadISAC(isacRate, isacBwEstimate,
       payload, payloadLenByte);

   return status;
}

} 
