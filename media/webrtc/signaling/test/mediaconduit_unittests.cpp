



#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include <vector>

using namespace std;

#include "mozilla/Scoped.h"
#include <MediaConduitInterface.h>
#include "nsIEventTarget.h"
#include "nsStaticComponents.h"
#include "FakeMediaStreamsImpl.h"

#define GTEST_HAS_RTTI 0
#include "gtest/gtest.h"
#include "gtest_utils.h"

#include "mtransport_test_utils.h"
MtransportTestUtils *test_utils;


const int COLOR = 0x80; 



static int32_t Rz, Rw;
static inline int32_t fast_rand(void)
{
  Rz=36969*(Rz&65535)+(Rz>>16);
  Rw=18000*(Rw&65535)+(Rw>>16);
  return (Rz<<16)+Rw;
}




struct VideoTestStats
{
 int numRawFramesInserted;
 int numFramesRenderedSuccessfully;
 int numFramesRenderedWrongly;
};

VideoTestStats vidStatsGlobal={0,0,0};








class VideoSendAndReceive
{
public:
  VideoSendAndReceive():width(640),
                        height(480)
  {
  }

  ~VideoSendAndReceive()
  {
  }

  void Init(mozilla::RefPtr<mozilla::VideoSessionConduit> aSession)
  {
        mSession = aSession;
  }
  void GenerateAndReadSamples()
  {

    int len = ((width * height) * 3 / 2);
    uint8_t* frame = (uint8_t*) PR_MALLOC(len);
    int numFrames = 121;
    memset(frame, COLOR, len);

    do
    {
      mSession->SendVideoFrame((unsigned char*)frame,
                                len,
                                width,
                                height,
                                mozilla::kVideoI420,
                                0);
      PR_Sleep(PR_MillisecondsToInterval(33));
      vidStatsGlobal.numRawFramesInserted++;
      numFrames--;
    } while(numFrames >= 0);
    PR_Free(frame);
  }

private:
mozilla::RefPtr<mozilla::VideoSessionConduit> mSession;
int width, height;
};











class AudioSendAndReceive
{
public:
  static const unsigned int PLAYOUT_SAMPLE_FREQUENCY; 
  static const unsigned int PLAYOUT_SAMPLE_LENGTH; 

  AudioSendAndReceive()
  {
  }

  ~AudioSendAndReceive()
  {
  }

 void Init(mozilla::RefPtr<mozilla::AudioSessionConduit> aSession,
           mozilla::RefPtr<mozilla::AudioSessionConduit> aOtherSession,
           std::string fileIn, std::string fileOut)
  {

    mSession = aSession;
    mOtherSession = aOtherSession;
    iFile = fileIn;
    oFile = fileOut;
 }

  
  void GenerateAndReadSamples();

private:

  mozilla::RefPtr<mozilla::AudioSessionConduit> mSession;
  mozilla::RefPtr<mozilla::AudioSessionConduit> mOtherSession;
  std::string iFile;
  std::string oFile;

  int WriteWaveHeader(int rate, int channels, FILE* outFile);
  int FinishWaveHeader(FILE* outFile);
  void GenerateMusic(int16_t* buf, int len);
};

const unsigned int AudioSendAndReceive::PLAYOUT_SAMPLE_FREQUENCY = 16000;
const unsigned int AudioSendAndReceive::PLAYOUT_SAMPLE_LENGTH  = 160000;

int AudioSendAndReceive::WriteWaveHeader(int rate, int channels, FILE* outFile)
{
  
  unsigned char header[] = {
    
    0x52, 0x49, 0x46, 0x46, 
    0x00, 0x00, 0x00, 0x00, 
    0x57, 0x41, 0x56, 0x45, 
    
    0x66, 0x6d, 0x74, 0x20, 
    0x10, 0x00, 0x00, 0x00, 
    0x01, 0x00,             
    0xFF, 0xFF,             
    0xFF, 0xFF, 0xFF, 0xFF, 
    0x00, 0x00, 0x00, 0x00, 
    0xFF, 0xFF,             
    0x10, 0x00,             
    
    0x64, 0x61, 0x74, 0x61, 
    0xFE, 0xFF, 0xFF, 0x7F  
  };

#define set_uint16le(buffer, value) \
  (buffer)[0] = (value) & 0xff; \
  (buffer)[1] = (value) >> 8;
#define set_uint32le(buffer, value) \
  set_uint16le( (buffer), (value) & 0xffff ); \
  set_uint16le( (buffer) + 2, (value) >> 16 );

  
  set_uint16le(header + 22, channels);
  set_uint32le(header + 24, rate);
  set_uint16le(header + 32, channels*2);

  size_t written = fwrite(header, 1, sizeof(header), outFile);
  if (written != sizeof(header)) {
    cerr << "Writing WAV header failed" << endl;
    return -1;
  }

  return 0;
}


int AudioSendAndReceive::FinishWaveHeader(FILE* outFile)
{
  
  long end = ftell(outFile);
  if (end < 16) {
    cerr << "Couldn't get output file length" << endl;
    return (end < 0) ? end : -1;
  }

  
  unsigned char size[4];
  int err = fseek(outFile, 40, SEEK_SET);
  if (err < 0) {
    cerr << "Couldn't seek to WAV file header." << endl;
    return err;
  }
  set_uint32le(size, (end - 44) & 0xffffffff);
  size_t written = fwrite(size, 1, sizeof(size), outFile);
  if (written != sizeof(size)) {
    cerr << "Couldn't write data size to WAV header" << endl;
    return -1;
  }

  
  err = fseek(outFile, 0, SEEK_END);
  if (err < 0) {
    cerr << "Couldn't seek to WAV file end." << endl;
    return err;
  }

  return 0;
}


void AudioSendAndReceive::GenerateMusic(short* buf, int len)
{
  cerr <<" Generating Input Music " << endl;
  int32_t a1,a2,b1,b2;
  int32_t c1,c2,d1,d2;
  int32_t i,j;
  a1=b1=a2=b2=0;
  c1=c2=d1=d2=0;
  j=0;
  
  for(i=0;i<2880;i++)
  {
    buf[i*2]=buf[(i*2)+1]=0;
  }
  for(i=2880;i<len-1;i+=2)
  {
    int32_t r;
    int32_t v1,v2;
    v1=v2=(((j*((j>>12)^((j>>10|j>>12)&26&j>>7)))&128)+128)<<15;
    r=fast_rand();v1+=r&65535;v1-=r>>16;
    r=fast_rand();v2+=r&65535;v2-=r>>16;
    b1=v1-a1+((b1*61+32)>>6);a1=v1;
    b2=v2-a2+((b2*61+32)>>6);a2=v2;
    c1=(30*(c1+b1+d1)+32)>>6;d1=b1;
    c2=(30*(c2+b2+d2)+32)>>6;d2=b2;
    v1=(c1+128)>>8;
    v2=(c2+128)>>8;
    buf[i]=v1>32767?32767:(v1<-32768?-32768:v1);
    buf[i+1]=v2>32767?32767:(v2<-32768?-32768:v2);
    if(i%6==0)j++;
  }
  cerr << "Generating Input Music Done " << endl;
}


void AudioSendAndReceive::GenerateAndReadSamples()
{
   int16_t audioInput[PLAYOUT_SAMPLE_LENGTH];
   int16_t audioOutput[PLAYOUT_SAMPLE_LENGTH];
   short* inbuf;
   int sampleLengthDecoded = 0;
   int SAMPLES = (PLAYOUT_SAMPLE_FREQUENCY * 10); 
   int CHANNELS = 1; 
   int sampleLengthInBytes = sizeof(audioInput);
   
   inbuf = (short *)moz_xmalloc(sizeof(short)*SAMPLES*CHANNELS);
   memset(audioInput,0,sampleLengthInBytes);
   memset(audioOutput,0,sampleLengthInBytes);
   MOZ_ASSERT(SAMPLES <= PLAYOUT_SAMPLE_LENGTH);

   FILE* inFile = fopen( iFile.c_str(), "wb+");
   if(!inFile) {
     cerr << "Input File Creation Failed " << endl;
     return;
   }

   FILE* outFile = fopen( oFile.c_str(), "wb+");
   if(!outFile) {
     cerr << "Output File Creation Failed " << endl;
     return;
   }

   
   WriteWaveHeader(PLAYOUT_SAMPLE_FREQUENCY, 1, inFile);
   GenerateMusic(inbuf, SAMPLES);
   fwrite(inbuf,1,SAMPLES*sizeof(inbuf[0])*CHANNELS,inFile);
   FinishWaveHeader(inFile);
   fclose(inFile);

   WriteWaveHeader(PLAYOUT_SAMPLE_FREQUENCY, 1, outFile);
   int numSamplesReadFromInput = 0;
   do
   {
    if(!memcpy(audioInput, inbuf, sampleLengthInBytes))
    {
      return;
    }

    numSamplesReadFromInput += PLAYOUT_SAMPLE_LENGTH;
    inbuf += PLAYOUT_SAMPLE_LENGTH;

    mSession->SendAudioFrame(audioInput,
                             PLAYOUT_SAMPLE_LENGTH,
                             PLAYOUT_SAMPLE_FREQUENCY,10);

    PR_Sleep(PR_MillisecondsToInterval(10));
    mOtherSession->GetAudioFrame(audioOutput, PLAYOUT_SAMPLE_FREQUENCY,
                                 10, sampleLengthDecoded);
    if(sampleLengthDecoded == 0)
    {
      cerr << " Zero length Sample " << endl;
    }

    int wrote_  = fwrite (audioOutput, 1 , sampleLengthInBytes, outFile);
    if(wrote_ != sampleLengthInBytes)
    {
      cerr << "Couldn't Write " << sampleLengthInBytes << "bytes" << endl;
      break;
    }
   }while(numSamplesReadFromInput < SAMPLES);

   FinishWaveHeader(outFile);
   fclose(outFile);
}







class DummyVideoTarget: public mozilla::VideoRenderer
{
public:
  DummyVideoTarget()
  {
  }

  virtual ~DummyVideoTarget()
  {
  }


  void RenderVideoFrame(const unsigned char* buffer,
                        unsigned int buffer_size,
                        uint32_t time_stamp,
                        int64_t render_time)
 {
  
  if(VerifyFrame(buffer, buffer_size) == 0)
  {
      vidStatsGlobal.numFramesRenderedSuccessfully++;
  } else
  {
      vidStatsGlobal.numFramesRenderedWrongly++;
  }
 }

 void FrameSizeChange(unsigned int, unsigned int, unsigned int)
 {
    
 }

 
 
 int VerifyFrame(const unsigned char* buffer, unsigned int buffer_size)
 {
    int good = 0;
    for(int i=0; i < (int) buffer_size; i++)
    {
      if(buffer[i] == COLOR)
      {
        ++good;
      }
      else
      {
        --good;
      }
    }
   return 0;
 }

};








class FakeMediaTransport : public mozilla::TransportInterface
{
public:
  FakeMediaTransport():numPkts(0),
                       mAudio(false),
                       mVideo(false)
  {
  }

  ~FakeMediaTransport()
  {
  }

  virtual nsresult SendRtpPacket(const void* data, int len)
  {
    ++numPkts;
    if(mAudio)
    {
      mOtherAudioSession->ReceivedRTPPacket(data,len);
    } else
    {
      mOtherVideoSession->ReceivedRTPPacket(data,len);
    }
    return NS_OK;
  }

  virtual nsresult SendRtcpPacket(const void* data, int len)
  {
    if(mAudio)
    {
      mOtherAudioSession->ReceivedRTCPPacket(data,len);
    } else
    {
      mOtherVideoSession->ReceivedRTCPPacket(data,len);
    }
    return NS_OK;
  }

  
  void SetAudioSession(mozilla::RefPtr<mozilla::AudioSessionConduit> aSession,
                        mozilla::RefPtr<mozilla::AudioSessionConduit>
                        aOtherSession)
  {
    mAudioSession = aSession;
    mOtherAudioSession = aOtherSession;
    mAudio = true;
  }

  
  void SetVideoSession(mozilla::RefPtr<mozilla::VideoSessionConduit> aSession,
                       mozilla::RefPtr<mozilla::VideoSessionConduit>
                       aOtherSession)
  {
    mVideoSession = aSession;
    mOtherVideoSession = aOtherSession;
    mVideo = true;
  }

private:
  mozilla::RefPtr<mozilla::AudioSessionConduit> mAudioSession;
  mozilla::RefPtr<mozilla::VideoSessionConduit> mVideoSession;
  mozilla::RefPtr<mozilla::VideoSessionConduit> mOtherVideoSession;
  mozilla::RefPtr<mozilla::AudioSessionConduit> mOtherAudioSession;
  int numPkts;
  bool mAudio, mVideo;
};


namespace {

class TransportConduitTest : public ::testing::Test
{
 public:
  TransportConduitTest()
  {
    
    iAudiofilename = "input.wav";
    oAudiofilename = "recorded.wav";
  }

  ~TransportConduitTest()
  {
  }

  
  void TestDummyAudioAndTransport()
  {
    
    int err=0;
    mAudioSession = mozilla::AudioSessionConduit::Create(NULL);
    if( !mAudioSession )
      ASSERT_NE(mAudioSession, (void*)NULL);

    mAudioSession2 = mozilla::AudioSessionConduit::Create(NULL);
    if( !mAudioSession2 )
      ASSERT_NE(mAudioSession2, (void*)NULL);

    FakeMediaTransport* xport = new FakeMediaTransport();
    ASSERT_NE(xport, (void*)NULL);
    xport->SetAudioSession(mAudioSession, mAudioSession2);
    mAudioTransport = xport;

    
    err = mAudioSession->AttachTransport(mAudioTransport);
    ASSERT_EQ(mozilla::kMediaConduitNoError, err);
    err = mAudioSession2->AttachTransport(mAudioTransport);
    ASSERT_EQ(mozilla::kMediaConduitNoError, err);

    
    
    mozilla::AudioCodecConfig cinst1(124,"opus",48000,960,1,64000);
    mozilla::AudioCodecConfig cinst2(125,"L16",16000,320,1,256000);


    std::vector<mozilla::AudioCodecConfig*> rcvCodecList;
    rcvCodecList.push_back(&cinst1);
    rcvCodecList.push_back(&cinst2);

    err = mAudioSession->ConfigureSendMediaCodec(&cinst1);
    ASSERT_EQ(mozilla::kMediaConduitNoError, err);
    err = mAudioSession->ConfigureRecvMediaCodecs(rcvCodecList);
    ASSERT_EQ(mozilla::kMediaConduitNoError, err);

    err = mAudioSession2->ConfigureSendMediaCodec(&cinst1);
    ASSERT_EQ(mozilla::kMediaConduitNoError, err);
    err = mAudioSession2->ConfigureRecvMediaCodecs(rcvCodecList);
    ASSERT_EQ(mozilla::kMediaConduitNoError, err);

    
    audioTester.Init(mAudioSession,mAudioSession2, iAudiofilename,oAudiofilename);
    cerr << "   ******************************************************** " << endl;
    cerr << "    Generating Audio Samples " << endl;
    cerr << "   ******************************************************** " << endl;
    PR_Sleep(PR_SecondsToInterval(2));
    audioTester.GenerateAndReadSamples();
    PR_Sleep(PR_SecondsToInterval(2));
    cerr << "   ******************************************************** " << endl;
    cerr << "    Input Audio  File                " << iAudiofilename << endl;
    cerr << "    Output Audio File                " << oAudiofilename << endl;
    cerr << "   ******************************************************** " << endl;
  }

  
  void TestDummyVideoAndTransport()
  {
    int err = 0;
    
    mVideoSession = mozilla::VideoSessionConduit::Create();
    if( !mVideoSession )
      ASSERT_NE(mVideoSession, (void*)NULL);

   
    mVideoSession2 = mozilla::VideoSessionConduit::Create();
    if( !mVideoSession2 )
      ASSERT_NE(mVideoSession2,(void*)NULL);

    mVideoRenderer = new DummyVideoTarget();
    ASSERT_NE(mVideoRenderer, (void*)NULL);

    FakeMediaTransport* xport = new FakeMediaTransport();
    ASSERT_NE(xport, (void*)NULL);
    xport->SetVideoSession(mVideoSession,mVideoSession2);
    mVideoTransport = xport;

    
    err = mVideoSession2->AttachRenderer(mVideoRenderer);
    ASSERT_EQ(mozilla::kMediaConduitNoError, err);
    err = mVideoSession->AttachTransport(mVideoTransport);
    ASSERT_EQ(mozilla::kMediaConduitNoError, err);
    err = mVideoSession2->AttachTransport(mVideoTransport);
    ASSERT_EQ(mozilla::kMediaConduitNoError, err);

    
    mozilla::VideoCodecConfig cinst1(120, "VP8", 640, 480);
    mozilla::VideoCodecConfig cinst2(124, "I420", 640, 480);


    std::vector<mozilla::VideoCodecConfig* > rcvCodecList;
    rcvCodecList.push_back(&cinst1);
    rcvCodecList.push_back(&cinst2);

    err = mVideoSession->ConfigureSendMediaCodec(&cinst1);
    ASSERT_EQ(mozilla::kMediaConduitNoError, err);
    err = mVideoSession->ConfigureRecvMediaCodecs(rcvCodecList);
    ASSERT_EQ(mozilla::kMediaConduitNoError, err);

    err = mVideoSession2->ConfigureSendMediaCodec(&cinst1);
    ASSERT_EQ(mozilla::kMediaConduitNoError, err);
    err = mVideoSession2->ConfigureRecvMediaCodecs(rcvCodecList);
    ASSERT_EQ(mozilla::kMediaConduitNoError, err);

    
    cerr << "   *************************************************" << endl;
    cerr << "    Starting the Video Sample Generation " << endl;
    cerr << "   *************************************************" << endl;
    PR_Sleep(PR_SecondsToInterval(2));
    videoTester.Init(mVideoSession);
    videoTester.GenerateAndReadSamples();
    PR_Sleep(PR_SecondsToInterval(2));
    cerr << "   **************************************************" << endl;
    cerr << "    Done With The Testing  " << endl;
    cerr << "    VIDEO TEST STATS  "  << endl;
    cerr << "    Num Raw Frames Inserted: "<<
                                        vidStatsGlobal.numRawFramesInserted << endl;
    cerr << "    Num Frames Successfully Rendered: "<<
                                        vidStatsGlobal.numFramesRenderedSuccessfully << endl;
    cerr << "    Num Frames Wrongly Rendered: "<<
                                        vidStatsGlobal.numFramesRenderedWrongly << endl;

    cerr << "    Done With The Testing  " << endl;

    cerr << "   **************************************************" << endl;


  }

 void TestVideoConduitCodecAPI()
  {
    int err = 0;
    mozilla::RefPtr<mozilla::VideoSessionConduit> mVideoSession;
    
    mVideoSession = mozilla::VideoSessionConduit::Create();
    if( !mVideoSession )
      ASSERT_NE(mVideoSession, (void*)NULL);

    
    cerr << "   *************************************************" << endl;
    cerr << "    Test Receive Codec Configuration API Now " << endl;
    cerr << "   *************************************************" << endl;

    std::vector<mozilla::VideoCodecConfig* > rcvCodecList;

    
    cerr << "   *************************************************" << endl;
    cerr << "    1. Same Codec (VP8) Repeated Twice " << endl;
    cerr << "   *************************************************" << endl;

    mozilla::VideoCodecConfig cinst1(120, "VP8", 640, 480);
    mozilla::VideoCodecConfig cinst2(120, "VP8", 640, 480);
    rcvCodecList.push_back(&cinst1);
    rcvCodecList.push_back(&cinst2);
    err = mVideoSession->ConfigureRecvMediaCodecs(rcvCodecList);
    EXPECT_NE(err,mozilla::kMediaConduitNoError);
    rcvCodecList.pop_back();
    rcvCodecList.pop_back();


    PR_Sleep(PR_SecondsToInterval(2));
    cerr << "   *************************************************" << endl;
    cerr << "    2. Codec With Invalid Payload Names " << endl;
    cerr << "   *************************************************" << endl;
    cerr << "   Setting payload 1 with name: I4201234tttttthhhyyyy89087987y76t567r7756765rr6u6676" << endl;
    cerr << "   Setting payload 2 with name of zero length" << endl;

    mozilla::VideoCodecConfig cinst3(124, "I4201234tttttthhhyyyy89087987y76t567r7756765rr6u6676", 352, 288);
    mozilla::VideoCodecConfig cinst4(124, "", 352, 288);

    rcvCodecList.push_back(&cinst3);
    rcvCodecList.push_back(&cinst4);

    err = mVideoSession->ConfigureRecvMediaCodecs(rcvCodecList);
    EXPECT_TRUE(err != mozilla::kMediaConduitNoError);
    rcvCodecList.pop_back();
    rcvCodecList.pop_back();


    PR_Sleep(PR_SecondsToInterval(2));
    cerr << "   *************************************************" << endl;
    cerr << "    3. Null Codec Parameter  " << endl;
    cerr << "   *************************************************" << endl;

    rcvCodecList.push_back(0);

    err = mVideoSession->ConfigureRecvMediaCodecs(rcvCodecList);
    EXPECT_TRUE(err != mozilla::kMediaConduitNoError);
    rcvCodecList.pop_back();

    cerr << "   *************************************************" << endl;
    cerr << "    Test Send Codec Configuration API Now " << endl;
    cerr << "   *************************************************" << endl;

    cerr << "   *************************************************" << endl;
    cerr << "    1. Same Codec (VP8) Repeated Twice " << endl;
    cerr << "   *************************************************" << endl;


    err = mVideoSession->ConfigureSendMediaCodec(&cinst1);
    EXPECT_EQ(mozilla::kMediaConduitNoError, err);
    err = mVideoSession->ConfigureSendMediaCodec(&cinst1);
    EXPECT_EQ(mozilla::kMediaConduitCodecInUse, err);


    cerr << "   *************************************************" << endl;
    cerr << "    2. Codec With Invalid Payload Names " << endl;
    cerr << "   *************************************************" << endl;
    cerr << "   Setting payload with name: I4201234tttttthhhyyyy89087987y76t567r7756765rr6u6676" << endl;

    err = mVideoSession->ConfigureSendMediaCodec(&cinst3);
    EXPECT_TRUE(err != mozilla::kMediaConduitNoError);

    cerr << "   *************************************************" << endl;
    cerr << "    3. Null Codec Parameter  " << endl;
    cerr << "   *************************************************" << endl;

    err = mVideoSession->ConfigureSendMediaCodec(NULL);
    EXPECT_TRUE(err != mozilla::kMediaConduitNoError);

  }

private:
  
  mozilla::RefPtr<mozilla::AudioSessionConduit> mAudioSession;
  mozilla::RefPtr<mozilla::AudioSessionConduit> mAudioSession2;
  mozilla::RefPtr<mozilla::TransportInterface> mAudioTransport;
  AudioSendAndReceive audioTester;

  
  mozilla::RefPtr<mozilla::VideoSessionConduit> mVideoSession;
  mozilla::RefPtr<mozilla::VideoSessionConduit> mVideoSession2;
  mozilla::RefPtr<mozilla::VideoRenderer> mVideoRenderer;
  mozilla::RefPtr<mozilla::TransportInterface> mVideoTransport;
  VideoSendAndReceive videoTester;

  std::string fileToPlay;
  std::string fileToRecord;
  std::string iAudiofilename;
  std::string oAudiofilename;
};



TEST_F(TransportConduitTest, TestDummyAudioWithTransport) {
  TestDummyAudioAndTransport();
}


TEST_F(TransportConduitTest, TestDummyVideoWithTransport) {
  TestDummyVideoAndTransport();
 }

TEST_F(TransportConduitTest, TestVideoConduitCodecAPI) {
  TestVideoConduitCodecAPI();
 }

}  

int main(int argc, char **argv)
{
  
  CHECK_ENVIRONMENT_FLAG("MOZ_WEBRTC_TESTS")

  test_utils = new MtransportTestUtils();
  ::testing::InitGoogleTest(&argc, argv);
  int rv = RUN_ALL_TESTS();
  delete test_utils;
  return rv;
}


