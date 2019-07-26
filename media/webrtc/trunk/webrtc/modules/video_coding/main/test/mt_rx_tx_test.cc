















#include <string.h>

#include "../source/event.h"
#include "media_opt_test.h"
#include "mt_test_common.h"
#include "receiver_tests.h" 
#include "rtp_rtcp.h"
#include "test_macros.h"
#include "test_util.h" 
#include "thread_wrapper.h"
#include "video_coding.h"

using namespace webrtc;

bool
MainSenderThread(void* obj)
{
    SendSharedState* state = static_cast<SendSharedState*>(obj);
    EventWrapper& waitEvent = *EventWrapper::Create();
    
    I420VideoFrame sourceFrame;
    WebRtc_Word32 width = state->_args.width;
    WebRtc_Word32 height = state->_args.height;
    float frameRate = state->_args.frameRate;
    WebRtc_Word32 lengthSourceFrame  = 3*width*height/2;
    WebRtc_UWord8* tmpBuffer = new WebRtc_UWord8[lengthSourceFrame];

    if (state->_sourceFile == NULL)
    {
        state->_sourceFile = fopen(state->_args.inputFile.c_str(), "rb");
        if (state->_sourceFile == NULL)
        {
            printf ("Error when opening file \n");
            delete &waitEvent;
            delete [] tmpBuffer;
            return false;
        }
    }
    if (feof(state->_sourceFile) == 0)
    {
        TEST(fread(tmpBuffer, 1, lengthSourceFrame,state->_sourceFile) > 0 ||
             feof(state->_sourceFile));
        state->_frameCnt++;
        int size_y = width * height;
        int half_width = (width + 1) / 2;
        int half_height = (height + 1) / 2;
        int size_uv = half_width * half_height;
        sourceFrame.CreateFrame(size_y, tmpBuffer,
                                size_uv, tmpBuffer + size_y,
                                size_uv, tmpBuffer + size_y + size_uv,
                                width, height,
                                width, half_width, half_width);
        state->_timestamp += (WebRtc_UWord32)(9e4 / frameRate);
        sourceFrame.set_timestamp(state->_timestamp);

        WebRtc_Word32 ret = state->_vcm.AddVideoFrame(sourceFrame);
        if (ret < 0)
        {
            printf("Add Frame error: %d\n", ret);
            delete &waitEvent;
            delete [] tmpBuffer;
            return false;
        }
        waitEvent.Wait(33);
    }

    delete &waitEvent;
    delete [] tmpBuffer;

    return true;
}

bool
IntSenderThread(void* obj)
{
    SendSharedState* state = static_cast<SendSharedState*>(obj);
    state->_vcm.SetChannelParameters(1000,30,0);

    return true;
}


int MTRxTxTest(CmdArgs& args)
{
    
    std::string   inname = args.inputFile;
    std::string outname;
    if (args.outputFile == "")
        outname = test::OutputPath() + "MTRxTxTest_decoded.yuv";
    else
        outname = args.outputFile;

    WebRtc_UWord16  width = args.width;
    WebRtc_UWord16  height = args.height;

    float         frameRate = args.frameRate;
    float         bitRate = args.bitRate;
    WebRtc_Word32   numberOfCores = 1;

    
    
    bool          nackEnabled = false;
    bool          fecEnabled = false;
    WebRtc_UWord8   rttMS = 20;
    float         lossRate = 0.0*255; 
    WebRtc_UWord32  renderDelayMs = 0;
    WebRtc_UWord32  minPlayoutDelayMs = 0;

    

    
    Trace::CreateTrace();
    Trace::SetTraceFile((test::OutputPath() + "MTRxTxTestTrace.txt").c_str());
    Trace::SetLevelFilter(webrtc::kTraceAll);

    FILE* sourceFile;
    FILE* decodedFile;

    if ((sourceFile = fopen(inname.c_str(), "rb")) == NULL)
    {
        printf("Cannot read file %s.\n", inname.c_str());
        return -1;
    }

    if ((decodedFile = fopen(outname.c_str(), "wb")) == NULL)
    {
        printf("Cannot read file %s.\n", outname.c_str());
        return -1;
    }
    TickTimeBase clock;
    VideoCodingModule* vcm = VideoCodingModule::Create(1, &clock);
    RtpDataCallback dataCallback(vcm);

    RTPSendCompleteCallback* outgoingTransport =
        new RTPSendCompleteCallback(&clock, "dump.rtp");

    RtpRtcp::Configuration configuration;
    configuration.id = 1;
    configuration.audio = false;
    configuration.incoming_data = &dataCallback;
    configuration.outgoing_transport = outgoingTransport;
    RtpRtcp* rtp = RtpRtcp::CreateRtpRtcp(configuration);

    
    VideoCodec videoCodec;
    strncpy(videoCodec.plName, "ULPFEC", 32);
    videoCodec.plType = VCM_ULPFEC_PAYLOAD_TYPE;
    TEST(rtp->RegisterReceivePayload(videoCodec) == 0);

    strncpy(videoCodec.plName, "RED", 32);
    videoCodec.plType = VCM_RED_PAYLOAD_TYPE;
    TEST(rtp->RegisterReceivePayload(videoCodec) == 0);

    strncpy(videoCodec.plName, args.codecName.c_str(), 32);
    videoCodec.plType = VCM_VP8_PAYLOAD_TYPE;
    videoCodec.maxBitrate = 10000;
    videoCodec.codecType = args.codecType;
    TEST(rtp->RegisterReceivePayload(videoCodec) == 0);
    TEST(rtp->RegisterSendPayload(videoCodec) == 0);

    
    TEST(rtp->SetGenericFECStatus(fecEnabled, VCM_RED_PAYLOAD_TYPE, VCM_ULPFEC_PAYLOAD_TYPE) == 0);

    
    if (vcm->InitializeReceiver() < 0)
    {
        return -1;
    }
    if (vcm->InitializeSender())
    {
        return -1;
    }
    
    VideoCodec sendCodec;
    vcm->InitializeSender();
    WebRtc_Word32 numberOfCodecs = vcm->NumberOfCodecs();
    if (numberOfCodecs < 1)
    {
        return -1;
    }

    if (vcm->Codec(args.codecType, &sendCodec) != 0)
    {
        
        printf("Codec not registered\n");
        return -1;
    }
    
    sendCodec.startBitrate = (int) bitRate;
    sendCodec.height = height;
    sendCodec.width = width;
    sendCodec.maxFramerate = (WebRtc_UWord8)frameRate;
    vcm->RegisterSendCodec(&sendCodec, numberOfCores, 1440);
    vcm->RegisterReceiveCodec(&sendCodec, numberOfCores); 

    vcm->SetRenderDelay(renderDelayMs);
    vcm->SetMinimumPlayoutDelay(minPlayoutDelayMs);

    

    PacketRequester packetRequester(*rtp);
    vcm->RegisterPacketRequestCallback(&packetRequester);

    VCMRTPEncodeCompleteCallback* encodeCompleteCallback = new VCMRTPEncodeCompleteCallback(rtp);
    vcm->RegisterTransportCallback(encodeCompleteCallback);
    encodeCompleteCallback->SetCodecType(ConvertCodecType(args.codecName.c_str()));
    encodeCompleteCallback->SetFrameDimensions(width, height);
    

    VCMDecodeCompleteCallback receiveCallback(decodedFile);
    vcm->RegisterReceiveCallback(&receiveCallback);

    VideoProtectionCallback protectionCallback;
    vcm->RegisterProtectionCallback(&protectionCallback);

    outgoingTransport->SetLossPct(lossRate);
    
    assert(nackEnabled == false);
    vcm->SetVideoProtection(kProtectionNack, nackEnabled);
    vcm->SetVideoProtection(kProtectionFEC, fecEnabled);

    
    FecProtectionParams delta_params = protectionCallback.DeltaFecParameters();
    FecProtectionParams key_params = protectionCallback.KeyFecParameters();
    rtp->SetFecParameters(&delta_params, &key_params);
    rtp->SetNACKStatus(nackEnabled ? kNackRtcp : kNackOff);

    vcm->SetChannelParameters((WebRtc_UWord32) bitRate,
                              (WebRtc_UWord8) lossRate, rttMS);

    SharedRTPState mtState(*vcm, *rtp); 
    SendSharedState mtSendState(*vcm, *rtp, args); 

    

    
    
    ThreadWrapper* mainSenderThread = ThreadWrapper::CreateThread(MainSenderThread,
            &mtSendState, kNormalPriority, "MainSenderThread");
    ThreadWrapper* intSenderThread = ThreadWrapper::CreateThread(IntSenderThread,
            &mtSendState, kNormalPriority, "IntThread");

    if (mainSenderThread != NULL)
    {
        unsigned int tid;
        mainSenderThread->Start(tid);
    }
    else
    {
        printf("Unable to start main sender thread\n");
        return -1;
    }

    if (intSenderThread != NULL)
    {
        unsigned int tid;
        intSenderThread->Start(tid);
    }
    else
    {
        printf("Unable to start sender interference thread\n");
        return -1;
    }

    
    ThreadWrapper* processingThread = ThreadWrapper::CreateThread(ProcessingThread,
            &mtState, kNormalPriority, "ProcessingThread");
    ThreadWrapper* decodeThread = ThreadWrapper::CreateThread(DecodeThread,
            &mtState, kNormalPriority, "DecodeThread");

    if (processingThread != NULL)
    {
        unsigned int tid;
        processingThread->Start(tid);
    }
    else
    {
        printf("Unable to start processing thread\n");
        return -1;
    }

    if (decodeThread != NULL)
    {
        unsigned int tid;
        decodeThread->Start(tid);
    }
    else
    {
        printf("Unable to start decode thread\n");
        return -1;
    }

    EventWrapper& waitEvent = *EventWrapper::Create();

    
    waitEvent.Wait(30000);

    

    while (!mainSenderThread->Stop())
    {
        ;
    }

    while (!intSenderThread->Stop())
    {
        ;
    }


    while (!processingThread->Stop())
    {
        ;
    }

    while (!decodeThread->Stop())
    {
        ;
    }

    printf("\nVCM MT RX/TX Test: \n\n%i tests completed\n", vcmMacrosTests);
    if (vcmMacrosErrors > 0)
    {
        printf("%i FAILED\n\n", vcmMacrosErrors);
    }
    else
    {
        printf("ALL PASSED\n\n");
    }

    delete &waitEvent;
    delete mainSenderThread;
    delete intSenderThread;
    delete processingThread;
    delete decodeThread;
    delete encodeCompleteCallback;
    delete outgoingTransport;
    VideoCodingModule::Destroy(vcm);
    delete rtp;
    rtp = NULL;
    vcm = NULL;
    Trace::ReturnTrace();
    fclose(decodedFile);
    printf("Multi-Thread test Done: View output file \n");
    return 0;

}

