









#ifndef MODULE_COMMON_TYPES_H
#define MODULE_COMMON_TYPES_H

#include <cstring> 
#include <assert.h>

#include "typedefs.h"
#include "common_types.h"

#ifdef _WIN32
    #pragma warning(disable:4351)       // remove warning "new behavior: elements of array
                                        
#endif

namespace webrtc
{
struct RTPHeader
{
    bool           markerBit;
    WebRtc_UWord8  payloadType;
    WebRtc_UWord16 sequenceNumber;
    WebRtc_UWord32 timestamp;
    WebRtc_UWord32 ssrc;
    WebRtc_UWord8  numCSRCs;
    WebRtc_UWord32 arrOfCSRCs[kRtpCsrcSize];
    WebRtc_UWord8  paddingLength;
    WebRtc_UWord16 headerLength;
};

struct RTPHeaderExtension
{
    WebRtc_Word32  transmissionTimeOffset;
};

struct RTPAudioHeader
{
    WebRtc_UWord8  numEnergy;                         
    WebRtc_UWord8  arrOfEnergy[kRtpCsrcSize];   
    bool           isCNG;                             
    WebRtc_UWord8  channel;                           
};

enum {kNoPictureId = -1};
enum {kNoTl0PicIdx = -1};
enum {kNoTemporalIdx = -1};
enum {kNoKeyIdx = -1};
enum {kNoSimulcastIdx = 0};

struct RTPVideoHeaderVP8
{
    void InitRTPVideoHeaderVP8()
    {
        nonReference = false;
        pictureId = kNoPictureId;
        tl0PicIdx = kNoTl0PicIdx;
        temporalIdx = kNoTemporalIdx;
        layerSync = false;
        keyIdx = kNoKeyIdx;
        partitionId = 0;
        beginningOfPartition = false;
        frameWidth = 0;
        frameHeight = 0;
    }

    bool           nonReference;   
    WebRtc_Word16  pictureId;      
                                   
    WebRtc_Word16  tl0PicIdx;      
                                   
    WebRtc_Word8   temporalIdx;    
    bool           layerSync;      
                                   
    int            keyIdx;         
    int            partitionId;    
    bool           beginningOfPartition;  
                                          
    int            frameWidth;     
    int            frameHeight;    
};
union RTPVideoTypeHeader
{
    RTPVideoHeaderVP8       VP8;
};

enum RTPVideoCodecTypes
{
    kRTPVideoGeneric  = 0,
    kRTPVideoVP8      = 8,
    kRTPVideoNoVideo  = 10,
    kRTPVideoFEC      = 11,
    kRTPVideoI420     = 12
};
struct RTPVideoHeader
{
    WebRtc_UWord16          width;                  
    WebRtc_UWord16          height;

    bool                    isFirstPacket;   
    WebRtc_UWord8           simulcastIdx;    
                                             
    RTPVideoCodecTypes      codec;
    RTPVideoTypeHeader      codecHeader;
};
union RTPTypeHeader
{
    RTPAudioHeader  Audio;
    RTPVideoHeader  Video;
};

struct WebRtcRTPHeader
{
    RTPHeader       header;
    FrameType       frameType;
    RTPTypeHeader   type;
    RTPHeaderExtension extension;
};

class RTPFragmentationHeader
{
public:
    RTPFragmentationHeader() :
        fragmentationVectorSize(0),
        fragmentationOffset(NULL),
        fragmentationLength(NULL),
        fragmentationTimeDiff(NULL),
        fragmentationPlType(NULL)
    {};

    ~RTPFragmentationHeader()
    {
        delete [] fragmentationOffset;
        delete [] fragmentationLength;
        delete [] fragmentationTimeDiff;
        delete [] fragmentationPlType;
    }

    RTPFragmentationHeader& operator=(const RTPFragmentationHeader& header)
    {
        if(this == &header)
        {
            return *this;
        }

        if(header.fragmentationVectorSize != fragmentationVectorSize)
        {
            

            
            delete [] fragmentationOffset;
            fragmentationOffset = NULL;
            delete [] fragmentationLength;
            fragmentationLength = NULL;
            delete [] fragmentationTimeDiff;
            fragmentationTimeDiff = NULL;
            delete [] fragmentationPlType;
            fragmentationPlType = NULL;

            if(header.fragmentationVectorSize > 0)
            {
                
                if(header.fragmentationOffset)
                {
                    fragmentationOffset = new WebRtc_UWord32[header.fragmentationVectorSize];
                }
                if(header.fragmentationLength)
                {
                    fragmentationLength = new WebRtc_UWord32[header.fragmentationVectorSize];
                }
                if(header.fragmentationTimeDiff)
                {
                    fragmentationTimeDiff = new WebRtc_UWord16[header.fragmentationVectorSize];
                }
                if(header.fragmentationPlType)
                {
                    fragmentationPlType = new WebRtc_UWord8[header.fragmentationVectorSize];
                }
            }
            
            fragmentationVectorSize =   header.fragmentationVectorSize;
        }

        if(header.fragmentationVectorSize > 0)
        {
            
            if(header.fragmentationOffset)
            {
                memcpy(fragmentationOffset, header.fragmentationOffset,
                        header.fragmentationVectorSize * sizeof(WebRtc_UWord32));
            }
            if(header.fragmentationLength)
            {
                memcpy(fragmentationLength, header.fragmentationLength,
                        header.fragmentationVectorSize * sizeof(WebRtc_UWord32));
            }
            if(header.fragmentationTimeDiff)
            {
                memcpy(fragmentationTimeDiff, header.fragmentationTimeDiff,
                        header.fragmentationVectorSize * sizeof(WebRtc_UWord16));
            }
            if(header.fragmentationPlType)
            {
                memcpy(fragmentationPlType, header.fragmentationPlType,
                        header.fragmentationVectorSize * sizeof(WebRtc_UWord8));
            }
        }
        return *this;
    }
    void VerifyAndAllocateFragmentationHeader( const WebRtc_UWord16 size)
    {
        if( fragmentationVectorSize < size)
        {
            WebRtc_UWord16 oldVectorSize = fragmentationVectorSize;
            {
                
                WebRtc_UWord32* oldOffsets = fragmentationOffset;
                fragmentationOffset = new WebRtc_UWord32[size];
                memset(fragmentationOffset+oldVectorSize, 0,
                       sizeof(WebRtc_UWord32)*(size-oldVectorSize));
                
                memcpy(fragmentationOffset,oldOffsets, sizeof(WebRtc_UWord32) * oldVectorSize);
                delete[] oldOffsets;
            }
            
            {
                WebRtc_UWord32* oldLengths = fragmentationLength;
                fragmentationLength = new WebRtc_UWord32[size];
                memset(fragmentationLength+oldVectorSize, 0,
                       sizeof(WebRtc_UWord32) * (size- oldVectorSize));
                memcpy(fragmentationLength, oldLengths,
                       sizeof(WebRtc_UWord32) * oldVectorSize);
                delete[] oldLengths;
            }
            
            {
                WebRtc_UWord16* oldTimeDiffs = fragmentationTimeDiff;
                fragmentationTimeDiff = new WebRtc_UWord16[size];
                memset(fragmentationTimeDiff+oldVectorSize, 0,
                       sizeof(WebRtc_UWord16) * (size- oldVectorSize));
                memcpy(fragmentationTimeDiff, oldTimeDiffs,
                       sizeof(WebRtc_UWord16) * oldVectorSize);
                delete[] oldTimeDiffs;
            }
            
            {
                WebRtc_UWord8* oldTimePlTypes = fragmentationPlType;
                fragmentationPlType = new WebRtc_UWord8[size];
                memset(fragmentationPlType+oldVectorSize, 0,
                       sizeof(WebRtc_UWord8) * (size- oldVectorSize));
                memcpy(fragmentationPlType, oldTimePlTypes,
                       sizeof(WebRtc_UWord8) * oldVectorSize);
                delete[] oldTimePlTypes;
            }
            fragmentationVectorSize = size;
        }
    }

    WebRtc_UWord16    fragmentationVectorSize;    
    WebRtc_UWord32*   fragmentationOffset;        
    WebRtc_UWord32*   fragmentationLength;        
    WebRtc_UWord16*   fragmentationTimeDiff;      
                                                  
    WebRtc_UWord8*    fragmentationPlType;        
};

struct RTCPVoIPMetric
{
    
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


struct FecProtectionParams {
  int fec_rate;
  bool use_uep_protection;
  int max_fec_frames;
};


class EncodedVideoData
{
public:
    EncodedVideoData() :
        payloadType(0),
        timeStamp(0),
        renderTimeMs(0),
        encodedWidth(0),
        encodedHeight(0),
        completeFrame(false),
        missingFrame(false),
        payloadData(NULL),
        payloadSize(0),
        bufferSize(0),
        fragmentationHeader(),
        frameType(kVideoFrameDelta),
        codec(kVideoCodecUnknown)
    {};

    EncodedVideoData(const EncodedVideoData& data)
    {
        payloadType         = data.payloadType;
        timeStamp           = data.timeStamp;
        renderTimeMs        = data.renderTimeMs;
        encodedWidth        = data.encodedWidth;
        encodedHeight       = data.encodedHeight;
        completeFrame       = data.completeFrame;
        missingFrame        = data.missingFrame;
        payloadSize         = data.payloadSize;
        fragmentationHeader = data.fragmentationHeader;
        frameType           = data.frameType;
        codec               = data.codec;
        if (data.payloadSize > 0)
        {
            payloadData = new WebRtc_UWord8[data.payloadSize];
            memcpy(payloadData, data.payloadData, data.payloadSize);
        }
        else
        {
            payloadData = NULL;
        }
    }


    ~EncodedVideoData()
    {
        delete [] payloadData;
    };

    EncodedVideoData& operator=(const EncodedVideoData& data)
    {
        if (this == &data)
        {
            return *this;
        }
        payloadType         = data.payloadType;
        timeStamp           = data.timeStamp;
        renderTimeMs        = data.renderTimeMs;
        encodedWidth        = data.encodedWidth;
        encodedHeight       = data.encodedHeight;
        completeFrame       = data.completeFrame;
        missingFrame        = data.missingFrame;
        payloadSize         = data.payloadSize;
        fragmentationHeader = data.fragmentationHeader;
        frameType           = data.frameType;
        codec               = data.codec;
        if (data.payloadSize > 0)
        {
            delete [] payloadData;
            payloadData = new WebRtc_UWord8[data.payloadSize];
            memcpy(payloadData, data.payloadData, data.payloadSize);
            bufferSize = data.payloadSize;
        }
        return *this;
    };
    void VerifyAndAllocate( const WebRtc_UWord32 size)
    {
        if (bufferSize < size)
        {
            WebRtc_UWord8* oldPayload = payloadData;
            payloadData = new WebRtc_UWord8[size];
            memcpy(payloadData, oldPayload, sizeof(WebRtc_UWord8) * payloadSize);

            bufferSize = size;
            delete[] oldPayload;
        }
    }

    WebRtc_UWord8               payloadType;
    WebRtc_UWord32              timeStamp;
    WebRtc_Word64               renderTimeMs;
    WebRtc_UWord32              encodedWidth;
    WebRtc_UWord32              encodedHeight;
    bool                        completeFrame;
    bool                        missingFrame;
    WebRtc_UWord8*              payloadData;
    WebRtc_UWord32              payloadSize;
    WebRtc_UWord32              bufferSize;
    RTPFragmentationHeader      fragmentationHeader;
    FrameType                   frameType;
    VideoCodecType              codec;
};

struct VideoContentMetrics {
  VideoContentMetrics()
      : motion_magnitude(0.0f),
        spatial_pred_err(0.0f),
        spatial_pred_err_h(0.0f),
        spatial_pred_err_v(0.0f) {
  }

  void Reset() {
    motion_magnitude = 0.0f;
    spatial_pred_err = 0.0f;
    spatial_pred_err_h = 0.0f;
    spatial_pred_err_v = 0.0f;
  }
  float motion_magnitude;
  float spatial_pred_err;
  float spatial_pred_err_h;
  float spatial_pred_err_v;
};










class VideoFrame
{
public:
    VideoFrame();
    ~VideoFrame();
    





    WebRtc_Word32 VerifyAndAllocate(const WebRtc_UWord32 minimumSize);
    



    WebRtc_Word32 SetLength(const WebRtc_UWord32 newLength);
    


    WebRtc_Word32 Swap(WebRtc_UWord8*& newMemory,
                       WebRtc_UWord32& newLength,
                       WebRtc_UWord32& newSize);
    


    WebRtc_Word32 SwapFrame(VideoFrame& videoFrame);
    



    WebRtc_Word32 CopyFrame(const VideoFrame& videoFrame);
    



    WebRtc_Word32 CopyFrame(WebRtc_UWord32 length, const WebRtc_UWord8* sourceBuffer);
    


    void Free();
    


    void SetTimeStamp(const WebRtc_UWord32 timeStamp) {_timeStamp = timeStamp;}
    


    WebRtc_UWord8*    Buffer() const {return _buffer;}

    WebRtc_UWord8*&   Buffer() {return _buffer;}

    


    WebRtc_UWord32    Size() const {return _bufferSize;}
    


    WebRtc_UWord32    Length() const {return _bufferLength;}
    


    WebRtc_UWord32    TimeStamp() const {return _timeStamp;}
    


    WebRtc_UWord32    Width() const {return _width;}
    


    WebRtc_UWord32    Height() const {return _height;}
    


    void   SetWidth(const WebRtc_UWord32 width)  {_width = width;}
    


    void  SetHeight(const WebRtc_UWord32 height) {_height = height;}
    


    void SetRenderTime(const WebRtc_Word64 renderTimeMs) {_renderTimeMs = renderTimeMs;}
    


    WebRtc_Word64    RenderTimeMs() const {return _renderTimeMs;}

private:
    void Set(WebRtc_UWord8* buffer,
             WebRtc_UWord32 size,
             WebRtc_UWord32 length,
             WebRtc_UWord32 timeStamp);

    WebRtc_UWord8*          _buffer;          
    WebRtc_UWord32          _bufferSize;      
    WebRtc_UWord32          _bufferLength;    
    WebRtc_UWord32          _timeStamp;       
    WebRtc_UWord32          _width;
    WebRtc_UWord32          _height;
    WebRtc_Word64           _renderTimeMs;
}; 


inline
VideoFrame::VideoFrame():
    _buffer(0),
    _bufferSize(0),
    _bufferLength(0),
    _timeStamp(0),
    _width(0),
    _height(0),
    _renderTimeMs(0)
{
    
}
inline
VideoFrame::~VideoFrame()
{
    if(_buffer)
    {
        delete [] _buffer;
        _buffer = NULL;
    }
}


inline
WebRtc_Word32
VideoFrame::VerifyAndAllocate(const WebRtc_UWord32 minimumSize)
{
    if (minimumSize < 1)
    {
        return -1;
    }
    if(minimumSize > _bufferSize)
    {
        
        WebRtc_UWord8* newBufferBuffer = new WebRtc_UWord8[minimumSize];
        if(_buffer)
        {
            
            memcpy(newBufferBuffer, _buffer, _bufferSize);
            delete [] _buffer;
        }
        else
        {
            memset(newBufferBuffer, 0, minimumSize * sizeof(WebRtc_UWord8));
        }
        _buffer = newBufferBuffer;
        _bufferSize = minimumSize;
    }
    return 0;
}

inline
WebRtc_Word32
VideoFrame::SetLength(const WebRtc_UWord32 newLength)
{
    if (newLength >_bufferSize )
    { 
        return -1;
    }
     _bufferLength = newLength;
     return 0;
}

inline
WebRtc_Word32
VideoFrame::SwapFrame(VideoFrame& videoFrame)
{
    WebRtc_UWord32 tmpTimeStamp  = _timeStamp;
    WebRtc_UWord32 tmpWidth      = _width;
    WebRtc_UWord32 tmpHeight     = _height;
    WebRtc_Word64  tmpRenderTime = _renderTimeMs;

    _timeStamp = videoFrame._timeStamp;
    _width = videoFrame._width;
    _height = videoFrame._height;
    _renderTimeMs = videoFrame._renderTimeMs;

    videoFrame._timeStamp = tmpTimeStamp;
    videoFrame._width = tmpWidth;
    videoFrame._height = tmpHeight;
    videoFrame._renderTimeMs = tmpRenderTime;

    return Swap(videoFrame._buffer, videoFrame._bufferLength, videoFrame._bufferSize);
}

inline
WebRtc_Word32
VideoFrame::Swap(WebRtc_UWord8*& newMemory, WebRtc_UWord32& newLength, WebRtc_UWord32& newSize)
{
    WebRtc_UWord8* tmpBuffer = _buffer;
    WebRtc_UWord32 tmpLength = _bufferLength;
    WebRtc_UWord32 tmpSize = _bufferSize;
    _buffer = newMemory;
    _bufferLength = newLength;
    _bufferSize = newSize;
    newMemory = tmpBuffer;
    newLength = tmpLength;
    newSize = tmpSize;
    return 0;
}

inline
WebRtc_Word32
VideoFrame::CopyFrame(WebRtc_UWord32 length, const WebRtc_UWord8* sourceBuffer)
{
    if (length > _bufferSize)
    {
        WebRtc_Word32 ret = VerifyAndAllocate(length);
        if (ret < 0)
        {
            return ret;
        }
    }
     memcpy(_buffer, sourceBuffer, length);
    _bufferLength = length;
    return 0;
}

inline
WebRtc_Word32
VideoFrame::CopyFrame(const VideoFrame& videoFrame)
{
    if(CopyFrame(videoFrame.Length(), videoFrame.Buffer()) != 0)
    {
        return -1;
    }
    _timeStamp = videoFrame._timeStamp;
    _width = videoFrame._width;
    _height = videoFrame._height;
    _renderTimeMs = videoFrame._renderTimeMs;
    return 0;
}

inline
void
VideoFrame::Free()
{
    _timeStamp = 0;
    _bufferLength = 0;
    _bufferSize = 0;
    _height = 0;
    _width = 0;
    _renderTimeMs = 0;

    if(_buffer)
    {
        delete [] _buffer;
        _buffer = NULL;
    }
}



























class AudioFrame
{
public:
    enum{kMaxAudioFrameSizeSamples = 3840}; 

    enum VADActivity
    {
        kVadActive  = 0,
        kVadPassive = 1,
        kVadUnknown = 2
    };
    enum SpeechType
    {
        kNormalSpeech = 0,
        kPLC          = 1,
        kCNG          = 2,
        kPLCCNG       = 3,
        kUndefined    = 4
    };

    AudioFrame();
    virtual ~AudioFrame();

    WebRtc_Word32 UpdateFrame(
        const WebRtc_Word32  id,
        const WebRtc_UWord32 timeStamp,
        const WebRtc_Word16* payloadData,
        const WebRtc_UWord16 payloadDataLengthInSamples,
        const int frequencyInHz,
        const SpeechType     speechType,
        const VADActivity    vadActivity,
        const WebRtc_UWord8  audioChannel = 1,
        const WebRtc_Word32  volume = -1,
        const WebRtc_Word32  energy = -1);

    AudioFrame& Append(const AudioFrame& rhs);

    void Mute();

    AudioFrame& operator=(const AudioFrame& rhs);
    AudioFrame& operator>>=(const WebRtc_Word32 rhs);
    AudioFrame& operator+=(const AudioFrame& rhs);
    AudioFrame& operator-=(const AudioFrame& rhs);

    WebRtc_Word32  _id;
    WebRtc_UWord32 _timeStamp;

    
    WebRtc_Word16 _payloadData[kMaxAudioFrameSizeSamples];
    WebRtc_UWord16 _payloadDataLengthInSamples;
    int _frequencyInHz;
    WebRtc_UWord8  _audioChannel;
    SpeechType   _speechType;
    VADActivity  _vadActivity;

    WebRtc_UWord32 _energy;
    WebRtc_Word32  _volume;
};

inline
AudioFrame::AudioFrame()
    :
    _id(-1),
    _timeStamp(0),
    _payloadData(),
    _payloadDataLengthInSamples(0),
    _frequencyInHz(0),
    _audioChannel(1),
    _speechType(kUndefined),
    _vadActivity(kVadUnknown),
    _energy(0xffffffff),
    _volume(0xffffffff)
{
}

inline
AudioFrame::~AudioFrame()
{
}

inline
WebRtc_Word32
AudioFrame::UpdateFrame(
    const WebRtc_Word32  id,
    const WebRtc_UWord32 timeStamp,
    const WebRtc_Word16* payloadData,
    const WebRtc_UWord16 payloadDataLengthInSamples,
    const int frequencyInHz,
    const SpeechType     speechType,
    const VADActivity    vadActivity,
    const WebRtc_UWord8  audioChannel,
    const WebRtc_Word32  volume,
    const WebRtc_Word32  energy)
{
    _id            = id;
    _timeStamp     = timeStamp;
    _frequencyInHz = frequencyInHz;
    _speechType    = speechType;
    _vadActivity   = vadActivity;
    _volume        = volume;
    _audioChannel  = audioChannel;
    _energy        = energy;

    if((payloadDataLengthInSamples > kMaxAudioFrameSizeSamples) ||
        (audioChannel > 2) || (audioChannel < 1))
    {
        _payloadDataLengthInSamples = 0;
        return -1;
    }
    _payloadDataLengthInSamples = payloadDataLengthInSamples;
    if(payloadData != NULL)
    {
        memcpy(_payloadData, payloadData, sizeof(WebRtc_Word16) *
            payloadDataLengthInSamples * _audioChannel);
    }
    else
    {
        memset(_payloadData,0,sizeof(WebRtc_Word16) *
            payloadDataLengthInSamples * _audioChannel);
    }
    return 0;
}

inline
void
AudioFrame::Mute()
{
  memset(_payloadData, 0, _payloadDataLengthInSamples * sizeof(WebRtc_Word16));
}

inline
AudioFrame&
AudioFrame::operator=(const AudioFrame& rhs)
{
    
    if((rhs._payloadDataLengthInSamples > kMaxAudioFrameSizeSamples) ||
        (rhs._audioChannel > 2) ||
        (rhs._audioChannel < 1))
    {
        return *this;
    }
    if(this == &rhs)
    {
        return *this;
    }
    _id               = rhs._id;
    _timeStamp        = rhs._timeStamp;
    _frequencyInHz    = rhs._frequencyInHz;
    _speechType       = rhs._speechType;
    _vadActivity      = rhs._vadActivity;
    _volume           = rhs._volume;
    _audioChannel     = rhs._audioChannel;
    _energy           = rhs._energy;

    _payloadDataLengthInSamples = rhs._payloadDataLengthInSamples;
    memcpy(_payloadData, rhs._payloadData,
        sizeof(WebRtc_Word16) * rhs._payloadDataLengthInSamples * _audioChannel);

    return *this;
}

inline
AudioFrame&
AudioFrame::operator>>=(const WebRtc_Word32 rhs)
{
    assert((_audioChannel > 0) && (_audioChannel < 3));
    if((_audioChannel > 2) ||
        (_audioChannel < 1))
    {
        return *this;
    }
    for(WebRtc_UWord16 i = 0; i < _payloadDataLengthInSamples * _audioChannel; i++)
    {
        _payloadData[i] = WebRtc_Word16(_payloadData[i] >> rhs);
    }
    return *this;
}

inline
AudioFrame&
AudioFrame::Append(const AudioFrame& rhs)
{
    
    assert((_audioChannel > 0) && (_audioChannel < 3));
    if((_audioChannel > 2) ||
        (_audioChannel < 1))
    {
        return *this;
    }
    if(_audioChannel != rhs._audioChannel)
    {
        return *this;
    }
    if((_vadActivity == kVadActive) ||
        rhs._vadActivity == kVadActive)
    {
        _vadActivity = kVadActive;
    }
    else if((_vadActivity == kVadUnknown) ||
        rhs._vadActivity == kVadUnknown)
    {
        _vadActivity = kVadUnknown;
    }
    if(_speechType != rhs._speechType)
    {
        _speechType = kUndefined;
    }

    WebRtc_UWord16 offset = _payloadDataLengthInSamples * _audioChannel;
    for(WebRtc_UWord16 i = 0;
        i < rhs._payloadDataLengthInSamples * rhs._audioChannel;
        i++)
    {
        _payloadData[offset+i] = rhs._payloadData[i];
    }
    _payloadDataLengthInSamples += rhs._payloadDataLengthInSamples;
    return *this;
}


inline
AudioFrame&
AudioFrame::operator+=(const AudioFrame& rhs)
{
    
    assert((_audioChannel > 0) && (_audioChannel < 3));
    if((_audioChannel > 2) ||
        (_audioChannel < 1))
    {
        return *this;
    }
    if(_audioChannel != rhs._audioChannel)
    {
        return *this;
    }
    bool noPrevData = false;
    if(_payloadDataLengthInSamples != rhs._payloadDataLengthInSamples)
    {
        if(_payloadDataLengthInSamples == 0)
        {
            
            _payloadDataLengthInSamples = rhs._payloadDataLengthInSamples;
            noPrevData = true;
        } else
        {
          return *this;
        }
    }

    if((_vadActivity == kVadActive) ||
        rhs._vadActivity == kVadActive)
    {
        _vadActivity = kVadActive;
    }
    else if((_vadActivity == kVadUnknown) ||
        rhs._vadActivity == kVadUnknown)
    {
        _vadActivity = kVadUnknown;
    }

    if(_speechType != rhs._speechType)
    {
        _speechType = kUndefined;
    }

    if(noPrevData)
    {
        memcpy(_payloadData, rhs._payloadData,
          sizeof(WebRtc_Word16) * rhs._payloadDataLengthInSamples * _audioChannel);
    } else
    {
      
      for(WebRtc_UWord16 i = 0; i < _payloadDataLengthInSamples * _audioChannel; i++)
      {
          WebRtc_Word32 wrapGuard = (WebRtc_Word32)_payloadData[i] +
                  (WebRtc_Word32)rhs._payloadData[i];
          if(wrapGuard < -32768)
          {
              _payloadData[i] = -32768;
          }else if(wrapGuard > 32767)
          {
              _payloadData[i] = 32767;
          }else
          {
              _payloadData[i] = (WebRtc_Word16)wrapGuard;
          }
      }
    }
    _energy = 0xffffffff;
    _volume = 0xffffffff;
    return *this;
}

inline
AudioFrame&
AudioFrame::operator-=(const AudioFrame& rhs)
{
    
    assert((_audioChannel > 0) && (_audioChannel < 3));
    if((_audioChannel > 2)||
        (_audioChannel < 1))
    {
        return *this;
    }
    if((_payloadDataLengthInSamples != rhs._payloadDataLengthInSamples) ||
        (_audioChannel != rhs._audioChannel))
    {
        return *this;
    }
    if((_vadActivity != kVadPassive) ||
        rhs._vadActivity != kVadPassive)
    {
        _vadActivity = kVadUnknown;
    }
    _speechType = kUndefined;

    for(WebRtc_UWord16 i = 0; i < _payloadDataLengthInSamples * _audioChannel; i++)
    {
        WebRtc_Word32 wrapGuard = (WebRtc_Word32)_payloadData[i] -
                (WebRtc_Word32)rhs._payloadData[i];
        if(wrapGuard < -32768)
        {
            _payloadData[i] = -32768;
        }
        else if(wrapGuard > 32767)
        {
            _payloadData[i] = 32767;
        }
        else
        {
            _payloadData[i] = (WebRtc_Word16)wrapGuard;
        }
    }
    _energy = 0xffffffff;
    _volume = 0xffffffff;
    return *this;
}

} 

#endif 
