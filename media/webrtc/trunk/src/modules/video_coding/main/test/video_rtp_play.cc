









#include "receiver_tests.h"
#include "video_coding.h"
#include "rtp_rtcp.h"
#include "trace.h"
#include "../source/event.h"
#include "../source/internal_defines.h"
#include "test_macros.h"
#include "rtp_player.h"
#include "modules/video_coding/main/source/mock/fake_tick_time.h"

#include <stdio.h>
#include <string.h>

using namespace webrtc;

WebRtc_Word32
RtpDataCallback::OnReceivedPayloadData(const WebRtc_UWord8* payloadData,
                                          const WebRtc_UWord16 payloadSize,
                                          const WebRtcRTPHeader* rtpHeader)
{
    return _vcm->IncomingPacket(payloadData, payloadSize, *rtpHeader);
}

FrameReceiveCallback::~FrameReceiveCallback()
{
    if (_timingFile != NULL)
    {
        fclose(_timingFile);
    }
    if (_outFile != NULL)
    {
        fclose(_outFile);
    }
}

WebRtc_Word32
FrameReceiveCallback::FrameToRender(VideoFrame& videoFrame)
{
    if (_timingFile == NULL)
    {
        _timingFile = fopen((test::OutputPath() + "renderTiming.txt").c_str(),
                            "w");
        if (_timingFile == NULL)
        {
            return -1;
        }
    }
    if (_outFile == NULL)
    {
        _outFile = fopen(_outFilename.c_str(), "wb");
        if (_outFile == NULL)
        {
            return -1;
        }
    }
    fprintf(_timingFile, "%u, %u\n",
            videoFrame.TimeStamp(),
            MaskWord64ToUWord32(videoFrame.RenderTimeMs()));
    if (fwrite(videoFrame.Buffer(), 1, videoFrame.Length(),
               _outFile) !=  videoFrame.Length()) {
      return -1;
    }
    return 0;
}

int RtpPlay(CmdArgs& args)
{
    
#if !defined(EVENT_DEBUG)
    return -1;
#endif
    

    bool protectionEnabled = false;
    VCMVideoProtection protectionMethod = kProtectionNack;
    WebRtc_UWord32 rttMS = 0;
    float lossRate = 0.0f;
    bool reordering = false;
    WebRtc_UWord32 renderDelayMs = 0;
    WebRtc_UWord32 minPlayoutDelayMs = 0;
    const WebRtc_Word64 MAX_RUNTIME_MS = -1;
    std::string outFile = args.outputFile;
    if (outFile == "")
        outFile = test::OutputPath() + "RtpPlay_decoded.yuv";
    FrameReceiveCallback receiveCallback(outFile);
    FakeTickTime clock(0);
    VideoCodingModule* vcm = VideoCodingModule::Create(1, &clock);
    RtpDataCallback dataCallback(vcm);
    RTPPlayer rtpStream(args.inputFile.c_str(), &dataCallback, &clock);


    PayloadTypeList payloadTypes;
    payloadTypes.push_front(new PayloadCodecTuple(VCM_VP8_PAYLOAD_TYPE, "VP8",
                                                  kVideoCodecVP8));

    Trace::CreateTrace();
    Trace::SetTraceFile((test::OutputPath() + "receiverTestTrace.txt").c_str());
    Trace::SetLevelFilter(webrtc::kTraceAll);
    

    

    WebRtc_Word32 ret = vcm->InitializeReceiver();
    if (ret < 0)
    {
        return -1;
    }
    vcm->RegisterReceiveCallback(&receiveCallback);
    vcm->RegisterPacketRequestCallback(&rtpStream);

    
    for (PayloadTypeList::iterator it = payloadTypes.begin();
        it != payloadTypes.end(); ++it) {
        PayloadCodecTuple* payloadType = *it;
        if (payloadType != NULL)
        {
            VideoCodec codec;
            if (VideoCodingModule::Codec(payloadType->codecType, &codec) < 0)
            {
                return -1;
            }
            codec.plType = payloadType->payloadType;
            if (vcm->RegisterReceiveCodec(&codec, 1) < 0)
            {
                return -1;
            }
        }
    }

    if (rtpStream.Initialize(&payloadTypes) < 0)
    {
        return -1;
    }
    bool nackEnabled = protectionEnabled &&
        (protectionMethod == kProtectionNack ||
         protectionMethod == kProtectionDualDecoder);
    rtpStream.SimulatePacketLoss(lossRate, nackEnabled, rttMS);
    rtpStream.SetReordering(reordering);
    vcm->SetChannelParameters(0, 0, rttMS);
    vcm->SetVideoProtection(protectionMethod, protectionEnabled);
    vcm->SetRenderDelay(renderDelayMs);
    vcm->SetMinimumPlayoutDelay(minPlayoutDelayMs);

    ret = 0;

    
    while ((ret = rtpStream.NextPacket(clock.MillisecondTimestamp())) == 0)
    {
        if (clock.MillisecondTimestamp() % 5 == 0)
        {
            ret = vcm->Decode();
            if (ret < 0)
            {
                return -1;
            }
        }
        while (vcm->DecodeDualFrame(0) == 1) {
        }
        if (vcm->TimeUntilNextProcess() <= 0)
        {
            vcm->Process();
        }
        if (MAX_RUNTIME_MS > -1 && clock.MillisecondTimestamp() >=
            MAX_RUNTIME_MS)
        {
            break;
        }
        clock.IncrementDebugClock(1);
    }

    switch (ret)
    {
    case 1:
        printf("Success\n");
        break;
    case -1:
        printf("Failed\n");
        break;
    case 0:
        printf("Timeout\n");
        break;
    }

    rtpStream.Print();

    
    while (!payloadTypes.empty())
    {
        delete payloadTypes.front();
        payloadTypes.pop_front();
    }
    delete vcm;
    vcm = NULL;
    Trace::ReturnTrace();
    return 0;
}
