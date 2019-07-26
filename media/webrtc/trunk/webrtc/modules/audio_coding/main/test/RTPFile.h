









#ifndef RTPFILE_H
#define RTPFILE_H

#include "audio_coding_module.h"
#include "module_common_types.h"
#include "typedefs.h"
#include "rw_lock_wrapper.h"
#include <stdio.h>
#include <queue>

namespace webrtc {

class RTPStream
{
public:
    virtual ~RTPStream(){}

    virtual void Write(const uint8_t payloadType, const uint32_t timeStamp,
                                     const int16_t seqNo, const uint8_t* payloadData,
                                     const uint16_t payloadSize, uint32_t frequency) = 0;

    
    
    virtual uint16_t Read(WebRtcRTPHeader* rtpInfo,
                          uint8_t* payloadData,
                          uint16_t payloadSize,
                          uint32_t* offset) = 0;
    virtual bool EndOfFile() const = 0;

protected:
    void MakeRTPheader(uint8_t* rtpHeader, 
                                      uint8_t payloadType, int16_t seqNo, 
                                      uint32_t timeStamp, uint32_t ssrc);
    void ParseRTPHeader(WebRtcRTPHeader* rtpInfo, const uint8_t* rtpHeader);
};

class RTPPacket
{
public:
    RTPPacket(uint8_t payloadType, uint32_t timeStamp,
                                     int16_t seqNo, const uint8_t* payloadData,
                                     uint16_t payloadSize, uint32_t frequency);
    ~RTPPacket();
    uint8_t payloadType;
    uint32_t timeStamp;
    int16_t seqNo;
    uint8_t* payloadData;
    uint16_t payloadSize;
    uint32_t frequency;
};

class RTPBuffer : public RTPStream
{
public:
    RTPBuffer();
    ~RTPBuffer();
    void Write(const uint8_t payloadType, const uint32_t timeStamp,
                                     const int16_t seqNo, const uint8_t* payloadData,
                                     const uint16_t payloadSize, uint32_t frequency);
    uint16_t Read(WebRtcRTPHeader* rtpInfo,
                  uint8_t* payloadData,
                  uint16_t payloadSize,
                  uint32_t* offset);
    virtual bool EndOfFile() const;
private:
    RWLockWrapper*             _queueRWLock;
    std::queue<RTPPacket *>   _rtpQueue;
};

class RTPFile : public RTPStream
{
public:
    ~RTPFile(){}
    RTPFile() : _rtpFile(NULL),_rtpEOF(false) {}
    void Open(const char *outFilename, const char *mode);
    void Close();
    void WriteHeader();
    void ReadHeader();
    void Write(const uint8_t payloadType, const uint32_t timeStamp,
                                     const int16_t seqNo, const uint8_t* payloadData,
                                     const uint16_t payloadSize, uint32_t frequency);
    uint16_t Read(WebRtcRTPHeader* rtpInfo,
                  uint8_t* payloadData,
                  uint16_t payloadSize,
                  uint32_t* offset);
    bool EndOfFile() const { return _rtpEOF; }
private:
    FILE*   _rtpFile;
    bool    _rtpEOF;
};

} 
#endif
