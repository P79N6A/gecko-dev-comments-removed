









#include "packet_loss_test.h"
#include "video_source.h"
#include <sstream>
#include <cassert>
#include <string.h>

using namespace webrtc;

PacketLossTest::PacketLossTest()
:
NormalAsyncTest("PacketLossTest", "Encode, remove lost packets, decode", 300,
                5),
_lossRate(0.1),
_lossProbability(0.1),
_lastFrame(NULL),
_lastFrameLength(0)
{
}

PacketLossTest::PacketLossTest(std::string name, std::string description)
:
NormalAsyncTest(name, description, 300, 5),
_lossRate(0.1),
_lossProbability(0.1),
_lastFrame(NULL),
_lastFrameLength(0)
{
}

PacketLossTest::PacketLossTest(std::string name, std::string description, double lossRate, bool useNack, unsigned int rttFrames )
:
NormalAsyncTest(name, description, 300, 5, rttFrames),
_lossRate(lossRate),
_lastFrame(NULL),
_lastFrameLength(0)
{
    assert(lossRate >= 0 && lossRate <= 1);
    if (useNack)
    {
        _lossProbability = 0;
    }
    else
    {
        _lossProbability = lossRate;
    }
}

void
PacketLossTest::Encoded(const EncodedImage& encodedImage)
{
    
    _frameQueue.push_back(encodedImage._timeStamp);
    NormalAsyncTest::Encoded(encodedImage);
}

void
PacketLossTest::Decoded(const I420VideoFrame& decodedImage)
{
    
    assert(!_frameQueue.empty()); 
    while(_frameQueue.front() < decodedImage.timestamp())
    {
        
        
        if (_decodedFile && _lastFrame)
        {
          if (fwrite(_lastFrame, 1, _lastFrameLength,
                     _decodedFile) != _lastFrameLength) {
            return;
          }
        }

        
        _frameQueue.pop_front();
    }
    
    assert(_frameQueue.front() == decodedImage.timestamp());

    
    _frameQueue.pop_front();

    
    unsigned int length = CalcBufferSize(kI420, decodedImage.width(),
                                         decodedImage.height());
    if (_lastFrameLength < length)
    {
        if (_lastFrame) delete [] _lastFrame;

        _lastFrame = new WebRtc_UWord8[length];
    }
    
    ExtractBuffer(decodedImage, length, _lastFrame);
    _lastFrameLength = length;

    NormalAsyncTest::Decoded(decodedImage);
}

void
PacketLossTest::Teardown()
{
    if (_totalKept + _totalThrown > 0)
    {
        printf("Target packet loss rate: %.4f\n", _lossProbability);
        printf("Actual packet loss rate: %.4f\n", (_totalThrown * 1.0f) / (_totalKept + _totalThrown));
        printf("Channel rate: %.2f kbps\n",
            0.001 * 8.0 * _sumChannelBytes / ((_framecnt * 1.0f) / _inst.maxFramerate));
    }
    else
    {
        printf("No packet losses inflicted\n");
    }

    NormalAsyncTest::Teardown();
}

void
PacketLossTest::Setup()
{
    const VideoSource source(_inname, _inst.width, _inst.height, _inst.maxFramerate);

    std::stringstream ss;
    std::string lossRateStr;
    ss << _lossRate;
    ss >> lossRateStr;
    _encodedName = source.GetName() + "-" + lossRateStr;
    _outname = "out-" + source.GetName() + "-" + lossRateStr;

    if (_lossProbability != _lossRate)
    {
        _encodedName += "-nack";
        _outname += "-nack";
    }
    _encodedName += ".vp8";
    _outname += ".yuv";

    _totalKept = 0;
    _totalThrown = 0;
    _sumChannelBytes = 0;

    NormalAsyncTest::Setup();
}

void
PacketLossTest::CodecSpecific_InitBitrate()
{
    assert(_bitRate > 0);
    WebRtc_UWord32 simulatedBitRate;
    if (_lossProbability != _lossRate)
    {
        
        simulatedBitRate = WebRtc_UWord32(_bitRate / (1 + _lossRate));
    }
    else
    {
        simulatedBitRate = _bitRate;
    }
    int rtt = 0;
    if (_inst.maxFramerate > 0)
      rtt = _rttFrames * (1000 / _inst.maxFramerate);
    _encoder->SetChannelParameters((WebRtc_UWord32)(_lossProbability * 255.0),
                                                    rtt);
    _encoder->SetRates(simulatedBitRate, _inst.maxFramerate);
}

int PacketLossTest::DoPacketLoss()
{
    
    
    
    if (_frameToDecode->_frame->Length() == 0 || _sumChannelBytes == 0)
    {
        _sumChannelBytes += _frameToDecode->_frame->Length();
        return 0;
    }
    unsigned char *packet = NULL;
    VideoFrame newEncBuf;
    newEncBuf.VerifyAndAllocate(_lengthSourceFrame);
    _inBufIdx = 0;
    _outBufIdx = 0;
    int size = 1;
    int kept = 0;
    int thrown = 0;
    while ((size = NextPacket(1500, &packet)) > 0)
    {
        if (!PacketLoss(_lossProbability, thrown))
        {
            InsertPacket(&newEncBuf, packet, size);
            kept++;
        }
        else
        {
            
            

            
            thrown++;
            
            
            
            
        }
    }
    int	lossResult  = (thrown!=0);	
    if (lossResult)
    {
        lossResult += (kept==0);	
    }
    _frameToDecode->_frame->CopyFrame(newEncBuf.Length(), newEncBuf.Buffer());
    _sumChannelBytes += newEncBuf.Length();
    _totalKept += kept;
    _totalThrown += thrown;

    return lossResult;
    
    
}

int PacketLossTest::NextPacket(int mtu, unsigned char **pkg)
{
    unsigned char *buf = _frameToDecode->_frame->Buffer();
    *pkg = buf + _inBufIdx;
    if (static_cast<long>(_frameToDecode->_frame->Length()) - _inBufIdx <= mtu)
    {
        int size = _frameToDecode->_frame->Length() - _inBufIdx;
        _inBufIdx = _frameToDecode->_frame->Length();
        return size;
    }
    _inBufIdx += mtu;
    return mtu;
}

int PacketLossTest::ByteLoss(int size, unsigned char *pkg, int bytesToLose)
{
    return size;
}

void PacketLossTest::InsertPacket(VideoFrame *buf, unsigned char *pkg, int size)
{
    if (static_cast<long>(buf->Size()) - _outBufIdx < size)
    {
        printf("InsertPacket error!\n");
        return;
    }
    memcpy(buf->Buffer() + _outBufIdx, pkg, size);
    buf->SetLength(buf->Length() + size);
    _outBufIdx += size;
}
